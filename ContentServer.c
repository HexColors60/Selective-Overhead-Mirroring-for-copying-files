#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>			/* For sockets */
#include <sys/socket.h>			/* For sockets */
#include <netinet/in.h>			/* For Internet sockets */
#include <netdb.h>			/* For gethostbyname */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tools.h"
#include "ContentServer.h"

void read_args(int argc, char * argv[], int * port, char ** dirorfile);
void * ContentChild(void * ptr);

char * dirorfile=NULL;



pthread_mutex_t mutex;
int counter =0;
delayID * delays;
int buffersize =2;
int reader = 0;
int writer = 0;
pthread_cond_t readers_cond;
pthread_cond_t writers_cond;

int main(int argc, char * argv[])
{
  int * connections;
  int i=0;
  connections = malloc(256*sizeof(int));
  int size = 256;
  int port=-1;

  delays = malloc(buffersize*sizeof(delayID));


  read_args(argc, argv, &port, &dirorfile);
  printf("%s \n", dirorfile );

  pthread_mutex_init(&mutex, 0);
  pthread_cond_init(&writers_cond, 0);
  pthread_cond_init(&readers_cond, 0);

  unsigned int serverlen, clientlen;	/* Server - client variables */
	struct sockaddr_in server, client;
	struct sockaddr *serverptr, *clientptr;
  int sock, newsock;

  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1){	/* Create socket */
    perror_exit("socket");
  }
  server.sin_family = AF_INET;	/* Internet domain */
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);	/* The given port */
	serverptr = (struct sockaddr *) &server;
	serverlen = sizeof server;

  if (bind(sock, serverptr, serverlen) < 0){
    perror_exit("bind");
  }

  if (listen(sock, 100) < 0){
    perror_exit("listen");
  }
  clientptr = (struct sockaddr *) &client;
  clientlen = sizeof(client);
  pthread_t service;
  while((newsock = accept(sock, clientptr, &clientlen)) != -1){
    connections[i] = newsock;
    pthread_create(&service, 0, ContentChild, &connections[i]);
    //close(newsock);
    i++;
    if (i == size)
    {
      size*=2;
      connections = realloc(connections, size*sizeof(int));
    }
  }
  pthread_cond_destroy(&writers_cond);
  pthread_cond_destroy(&readers_cond);
  pthread_mutex_destroy(&mutex);
  free(connections);
  free(dirorfile);
  exit(EXIT_SUCCESS);
}

void * ContentChild(void * ptr)
{
  int socket;
  socket = *(int *)ptr;
  char *request;
  read_data(socket, &request);
  //printf("%s\n", request );
  char * type = strtok(request, " ");
  if(strcmp(type, "LIST")==0)
  {

    writer_lock();
    if (buffersize == counter)
    {
      buffersize *=2;
      delays = realloc(delays, buffersize*sizeof(delayID));
    }
    delays[counter].ID = atoi(strtok(NULL, " "));
    delays[counter].delay = atoi(strtok(NULL, " "));
    counter++;
    writer_release();

    char command [1024];
    char command2 [1024];
    sprintf(command, "find %s -type f", dirorfile);
    FILE * fp, *temp;
    fp = popen(command, "r");
    sprintf(command2, "%s | wc -l", command);
    temp = popen(command2, "r");
    char *line = NULL;
    size_t len = 0;
    int count = 0;
    if(getline(&line, &len, temp)!=0)
      count =atoi(line);
    //printf("Count is %d \n", count);
    write_data(socket, line);
    char * pos;
    char * substring;
    while( getline(&line, &len, fp) !=-1)
    {
      substring = strstr(line, dirorfile);
      printf("%s", substring);
      if ((pos=strchr(line, '\n')) != NULL)
	    		*pos = '\0';	//we don't want to include the \n in our strings
      write_data(socket, line);
    }
    pclose(temp);
    pclose(fp);
    // printf("Got list request\n");
  }
  else if(strcmp(type, "FETCH")==0)
  {
    int i;
    int id = atoi(strtok(NULL, " "));
    reader_lock();
    int delay;
    for(i =0; i < counter; i++)
    {
      if (delays[i].ID == id)
       break;
    }
    delay = delays[i].delay;
    reader_release();
    sleep(delay);
    char * filename =strtok(NULL, " ");
    char data[1024];
    int filedesc = open(filename, O_RDONLY);
    while((i = read(filedesc, data, 1024)) >0)
    {
      printf("i is: %d\n",i );
      send_data(socket, data, i);
    }
    printf("Sent file: %s\n", filename);
  }
//  write_data(socket, "0");
  free(request);
  close(socket);
  pthread_detach(pthread_self());
  pthread_exit((void *)0);
}



void read_args(int argc, char * argv[], int * port, char ** dirorfile)
{
  int i;

  for (i = 1; i < argc; i+=2)
  {
    if(strcmp(argv[i], "-p") == 0)
    {
      *port =atoi(argv[i+1]);
    }
    else if(strcmp(argv[i], "-d") == 0)
    {
      *dirorfile = malloc((strlen(argv[i+1])+1)*sizeof(char));
      strcpy(*dirorfile, argv[i+1]);
    }
  }

  if((*port < 0) || (*dirorfile == NULL))
  {
    printf("Usage: ./ContentServer -p <port> -d <dirorfilename> \n");
    exit(EXIT_FAILURE);
  }
}
