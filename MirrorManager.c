#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>			/* For sockets */
#include <sys/socket.h>			/* For sockets */
#include <netinet/in.h>			/* For Internet sockets */
#include <netdb.h>			/* For gethostbyname */

#include "MirrorServer.h"
#include "DataStructures.h"
#include "tools.h"

void * mirrorManager(void *ptr)
{
  char * str = (char *)ptr;
  char * addr = strtok(str, ":");
  int port = atoi(strtok(NULL, ":"));
  char * dirorfile = strtok(NULL, ":");
  int delay = atoi(strtok(NULL, ":"));
  ContentServer * contentserver;
  contentserver = createContentServer(addr, port, dirorfile, delay);

  int sock;
  unsigned int serverlen;
  struct sockaddr_in server;
  struct sockaddr *serverptr;
  struct hostent *rem;

  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    perror_exit("socket");
  }
  if ((rem = gethostbyname(contentserver->Address)) == NULL){	/* Find server address */
    perror_exit("gethostbyname");
  }
  server.sin_family = AF_INET;
  bcopy((char *) rem -> h_addr, (char *) &server.sin_addr, rem -> h_length);
  server.sin_port = htons(contentserver->Port);
  serverptr = (struct sockaddr *) &server;
  serverlen = sizeof server;
  if (connect(sock, serverptr, serverlen) < 0){
    perror_exit("connect");
  }
  char localbuffer[127];
  sprintf(localbuffer, "LIST %d", contentserver->delay);
  write_data(sock, localbuffer);
  char * answer;
  read_data(sock, &answer);
  printf("Answer is %s\n", answer );


  /*connection, send request*/
  int i;
  for(i=0 ; i <5 ; i++)
  {

    ServerBuffer * sb;
    acquire_manager();
    sb = createServerBuffer("aaaaa", "bbbbb", 420);
    buffer[counter] = sb;
    counter++;
    //printf("Wrote on buffer\n" );
    //printf("Buffer size is: %d\n", counter );
    sb = NULL;
    release_manager();
  }

  acquire_manager();
  windowsmanagersfinished++;
  release_manager();

  deleteContentServer(contentserver);
  free(contentserver);
  pthread_exit((void *)0);
}

void acquire_manager()
{
  pthread_mutex_lock(&mutex);
  while (worker || manager || (counter==BUFFERSIZE))
  {
    pthread_cond_wait(&managers_cond, &mutex);
  }
  manager++;
  pthread_mutex_unlock(&mutex);
}

void release_manager()
{
  pthread_mutex_lock(&mutex);
  manager--;
  pthread_cond_signal(&workers_cond);
  pthread_cond_signal(&managers_cond);
  pthread_mutex_unlock(&mutex);
}
