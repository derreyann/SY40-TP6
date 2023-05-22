//=============================================================================//
//										prg2.c
//===============================================================================//

/*------------------------------------------------------------------

    Modele client-serveur : generation de nombres aleatoires

    communication par files de messages, terminaison par Ctrl-C

    le serveur : le pere
    un client  : un des processus fils (leur nombre est argv[1])

    un client emet un message de type 1 contenant :
        (int pidEmetteur, int nbNombreDemandes)

    le serveur emet un message de type pidClient contenant :
        (int tableau[nbNombreDemandes])

    un client n'extrait que les types de messages ayant son pid

    le serveur n'extrait que les messages de type 1

    Remarque : version simplifiee -> pas de recouvrement
               numero interne connu avant creation fils


-------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#define NBMAXNB 6
#define TRI_MSG_TYPE 2

typedef struct
{
  long type; /* impose, type du msg */
  pid_t pidEmetteur;
  int nbNombreDemandes;
  int nMax;

} trequete;

typedef struct
{
  long type;
  int tab[NBMAXNB]; /* on dimensionne au msg de taille max */

} treponse;

typedef struct
{
  long type;
  int tab[NBMAXNB];
} tri_message;

int tri_msgid; // Message queue ID for communication with "tri.c"

int msgid; /* connu de tous les processus */

void initRand() /* init generateur nb aleatoires */
{
  srand(getpid());
}

void erreurFin(const char *msg)
{
  perror(msg);
  exit(1);
}

/*---------------- CODE CLIENT -----------------------*/

void constructionRequete(trequete *preq)
{
  preq->pidEmetteur = getpid();
  preq->nbNombreDemandes = rand() % NBMAXNB;
  preq->nMax = rand() + 1; /* evite le 0 comme nb max */
}

void affichageRequete(trequete *preq)
{
  printf("%d nombres de 1 a %d demandes par le processus %d\n",
         preq->nbNombreDemandes, preq->nMax, preq->pidEmetteur);
}

void affichageReponse(trequete *preq, treponse *prep)
{
  int i, lg;
  char msg[1000];
  lg = sprintf(msg, "\nNombres recus par le processus %d : \n", preq->pidEmetteur);

  for (i = 0; i < preq->nbNombreDemandes; i++)
    lg += sprintf(msg + lg, "%d  ", prep->tab[i]);

  sprintf(msg + lg, "\n");
  write(1, msg, lg + 1); /* assure l'indivisibilite de l'affichage */
}

void client()
{
  trequete req;
  treponse rep;
  int tailleReq, tailleRep;

  tailleReq = sizeof(trequete) - sizeof(long);
  initRand();

  while (1)
  {
    constructionRequete(&req);
    affichageRequete(&req);
    req.type = 1; /* num de processus utilisateur impossible */
    msgsnd(msgid, &req, tailleReq, 0);
    tailleRep = req.nbNombreDemandes * sizeof(int);
    msgrcv(msgid, &rep, tailleRep, getpid(), 0);
    affichageReponse(&req, &rep);
  }
}

/*---------------- CODE SERVEUR -----------------------*/

void constructionReponse(trequete *preq, treponse *prep)
{
  int i;
  for (i = 0; i < preq->nbNombreDemandes; i++)
    prep->tab[i] = rand() % preq->nMax;
}

int ordreAscendantInt(const void *v1, const void *v2)
{
  return (*(const int *)v1 - *(const int *)v2);
}

void tri(int tailleRep)
{
  int tri_msgid; // Message queue ID for communication with the server
  int cle = 1234;
  tri_message tri_msg;

  // Connect to the message queue created by the server
  if ((tri_msgid = msgget(cle, 0)) == -1)
  {
    perror("msgget");
    exit(1);
  }

  // Receive the numbers from the server through the message queue
  if (msgrcv(tri_msgid, &tri_msg, tailleRep, TRI_MSG_TYPE, 0) == -1)
  {
    perror("msgrcv");
    exit(1);
  }

  printf("Numbers received from the server\n");

  // Sort the numbers
  qsort(tri_msg.tab, NBMAXNB, sizeof(int), ordreAscendantInt);

  // Send the sorted numbers back to the server through the message queue
  if (msgsnd(tri_msgid, &tri_msg, tailleRep, 0) == -1)
  {
    perror("msgsnd");
    exit(1);
  }
}

void serveur()
{
  trequete req;
  treponse rep;
  int tailleReq, tailleRep;

  tailleReq = sizeof(trequete) - sizeof(long);
  initRand();

  while (1)
  {
    msgrcv(msgid, &req, tailleReq, 1, 0);
    constructionReponse(&req, &rep);

    // Sending the numbers to "tri.c" through the message queue
    tri_message tri_msg;
    tri_msg.type = TRI_MSG_TYPE;
    tailleRep = req.nbNombreDemandes * sizeof(int);

    memcpy(tri_msg.tab, rep.tab, req.nbNombreDemandes);

    int taillemess = sizeof(tri_msg) - sizeof(long);

    // Sending the message to "tri.c"
    msgsnd(tri_msgid, &tri_msg, taillemess, 0);

    if (fork() == 0)
    {
      // Child process - execute "tri" process
      tri(taillemess);
      exit(0);
    }
    else
    {
      wait(NULL);
    }

    // Receiving the sorted numbers from "tri.c"
    msgrcv(tri_msgid, &tri_msg, taillemess, TRI_MSG_TYPE, 0);
    //qsort(tri_msg.tab, NBMAXNB, sizeof(int), ordreAscendantInt);

    memcpy(rep.tab, tri_msg.tab, req.nbNombreDemandes * sizeof(int));

    rep.type = req.pidEmetteur;
    msgsnd(msgid, &rep, tailleRep, 0);
  }
}



/*------------- CODE GESTION PROCESSUS -----------------------*/

/* fonction permettant de creer nbFils qui executent la meme fonction */

void forkn(int nbFils, void (*pauBoulot)())
{
  int i;
  for (i = 0; i < nbFils; i++)
    if (fork() == 0)
    {
      (*pauBoulot)();
      exit(0); /* assure terminaison du fils */
    }
}

void traitantSIGINT(int s)
{
  msgctl(msgid, IPC_RMID, NULL);
  msgctl(tri_msgid, IPC_RMID, NULL);
  exit(0);
}

int main(int argc, char *argv[])
{
  int nbClients;
  key_t cle;
  key_t cle_tri = 1234;

  if (argc - 1 != 1)
  {
    fprintf(stderr, "Appel : %s <nb clients>\n", argv[0]);
    exit(2);
  }
  nbClients = atoi(argv[1]);
  if ((cle = ftok(argv[0], '0')) == -1)
    erreurFin("Pb ftok");
  if ((msgid = msgget(cle, IPC_CREAT | IPC_EXCL | 0600)) == -1)
    erreurFin("Pb msgget 1");
  if ((tri_msgid = msgget(cle_tri, IPC_CREAT | IPC_EXCL | 0600)) == -1)
    erreurFin("Pb msgget tri");
  forkn(nbClients, client);
  signal(SIGINT, traitantSIGINT);
  // Cleanup the message queues
  serveur();
}
