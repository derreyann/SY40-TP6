//=============================================================================//
//										tri.c 
//===============================================================================//

/* tri alpha-numerique en utilisant la fonction qsort */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#define NBMAXNB 20
#define TRI_MSG_TYPE 2 // Specify a unique message type for communication with the server

typedef struct
{
    long type;
    int tab[NBMAXNB];
} tri_message;


/*-----------------------------------------------------------
                      char*
-------------------------------------------------------------*/    

int ordreAscendantCharEtoile(const void* s1, const void* s2)
{
  return strcmp(*(const char**)s1, *(const char**)s2);
}

  
void trierChaines(char **t, int nb)   
{
  qsort(t, nb, sizeof(char*), ordreAscendantCharEtoile);
}


/*-----------------------------------------------------------
                          int
-------------------------------------------------------------*/    

int ordreAscendantInt(const void* v1, const void* v2)
{
  return (*(const int*)v1 - *(const int*)v2);
}



int main(int argc, char *argv[])
{

    int tailleRep = sizeof(tri_message) - sizeof(long);
    int tri_msgid; // Message queue ID for communication with the server
    int cle=1234;
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
    //printf("Sorted numbers sent back to the server\n");
    return 0;
}