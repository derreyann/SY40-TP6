//=============================================================================//
//										P3.c 
//===============================================================================//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define cle 314

void erreur(const char* msg)      {perror(msg);exit(1);}

/* structure du message */
typedef struct 
{
        long  type;
        pid_t numPID;
} tMessage;


int main(int argc,char* argv[])
{
  int  msgid, tailleMsg;
  tMessage req, rep;
  
  tailleMsg    = sizeof(tMessage) - sizeof(long);
 
  if (argc-1 != 0) 
  {
       fprintf(stderr,"Appel %s <desc MSG>",argv[0]);
       exit(1);
  };

  if ((msgid = msgget(cle, 0)) == -1) 
     erreur("Pb msgget dans P3");
     
    req.type = 2; 
    req.numPID = getpid(); 
  
    /* envoi message req */
    if (msgsnd(msgid, &req, tailleMsg, 0) == -1) {
        erreur("Pb msgsnd dans P3");
    }
  
    /* reponse message rep */
    if (msgrcv(msgid, &rep, tailleMsg, 3, 0) == -1) {
        erreur("Pb msgrcv dans P3");
    }

  printf("P3 : mon pid %d, son pid %d \n", getpid(), rep.numPID);
  exit(0);
  }
     
