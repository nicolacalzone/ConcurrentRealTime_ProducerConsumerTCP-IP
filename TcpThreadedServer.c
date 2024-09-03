/* Libraries */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
/* For shared memory*/
#include "sharedMem.h"

/* Global variables */
int exit_status = 0;
int client_id = 0;
pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;
struct BufferDataMonitorServer *sharedBuf;

/* Definition of functions */
static void *connectionHandler(void *arg);
static void handleConnection(int currSd, int client_id);
int getUpdatedData();
int generateId();
void handle_signal(int signal);
uint64_t htonll(uint64_t value);
uint64_t ntohll(uint64_t value);
void convertUpdatedData(uint64_t *first, uint64_t *second);

/* MAIN */
int main(int argc, char *argv[])
{
  int sd, *currSd;
  int port, sharedMemId;;
  struct sockaddr_in sin, retSin;
  pthread_t threads[MAX_THREADS];
  //pthread_t serverReader;

  /* Set-up shared memory */
  sharedMemId = shmget(SHM_KEY_MONITOR_SERVER, sizeof(struct BufferDataMonitorServer), SHM_R);
  if(sharedMemId == -1)
  {
    perror("Error in shmget");
    exit(0);
  }
  /* Shared memory attach b/w sharedBuf and sharedMem segment */
  sharedBuf = shmat(sharedMemId, NULL, 0);
  if(sharedBuf == (void *)-1)
  {
    perror("Error in shmat");
    exit(0);
  }  

  /* Initialize buffer indexes */
  sharedBuf->producedMessages = 0;
  sharedBuf->queueSize = 0;

  /* Create serverReader thread */
  //pthread_create(&serverReader, NULL, readingProcess, NULL);

  // Check for correct usage
  if (argc < 2)
  {
    printf("Usage: server <port>\n");
    exit(0);
  }
  sscanf(argv[1], "%d", &port);

  // Create a new socket
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

/* set socket options REUSE ADDRESS */
  int reuse = 1;
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
      perror("setsockopt(SO_REUSEADDR) failed");
  #ifdef SO_REUSEPORT
      if (setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
          perror("setsockopt(SO_REUSEPORT) failed");
  #endif


  // Initialize the address (struct sockaddr_in) fields
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);

  // Bind the socket to the specified port number
  if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1)
  {
    perror("bind");
    exit(1);
  }

  // Set the maximum queue length for clients requesting connection to MAX_CLIENTS
  if (listen(sd, MAX_THREADS) == -1)
  {
    perror("listen");
    exit(1);
  }

  // Accept and serve all incoming connections in a loop
  for (int i = 0; i < MAX_THREADS; ++i)
  {
    socklen_t sAddrLen = sizeof(retSin);

    // Allocate the current socket and client ID.
    currSd = (int *)malloc(2 * sizeof(int));
    if ((*currSd = accept(sd, (struct sockaddr *)&retSin, &sAddrLen)) == -1)
    {
      perror("accept");
      exit(1);
    }

    /* Generate a unique client ID */
    //pthread_mutex_lock(&client_id_mutex);
    //currSd[1] = client_id_counter++;
    //pthread_mutex_unlock(&client_id_mutex);

    currSd[1] = generateId();

    printf("Connection received from: %s with clientId: %d\n", inet_ntoa(retSin.sin_addr), currSd[1]);

    // Connection received, start a new thread serving the connection
    pthread_create(&threads[i], NULL, connectionHandler, currSd);

    //signal(SIGINT, handle_signal);
    //signal(SIGTSTP, handle_signal);
  }

  return 0; // never reached
}


// ---------------------------------------------------------------------------------------------------

/* Description of Functions */

// Thread routine to handle the connection
static void *connectionHandler(void *arg)
{
  int currSock = ((int *)arg)[0];
  int client_id = ((int *)arg)[1];
  free(arg);
  handleConnection(currSock, client_id);
  pthread_exit(0);
  return NULL;
}

// Function to handle the connection
static void handleConnection(int currSd, int client_id)
{
  int consumersAmtByte = sharedBuf->consumersAmt;
  //send the num of consumers
  if (send(currSd, &consumersAmtByte, sizeof(consumersAmtByte), 0) == -1)
  {
    perror("Failed to send number");
  }
  else
  {
    printf("num of consumers: %d to client %d\n", consumersAmtByte, client_id);
  }  

  for (;;)
  {
    /* Gets update data from the shared buffer 
       and converts it to network byte order */
    uint64_t producedMessagesNBO, queueSizeNBO;
    convertUpdatedData(&producedMessagesNBO, &queueSizeNBO); 


  /* Send Produced Messages */
    if (send(currSd, &producedMessagesNBO, sizeof(producedMessagesNBO), 0) == -1)
    {
      perror("Failed to send number");
      break;
    }
    else
    {
      printf("produced: %ld to client %d\n", producedMessagesNBO, client_id);
    }
  /* Send Queue Size */
    if (send(currSd, &queueSizeNBO, sizeof(queueSizeNBO), 0) == -1)
    {
      perror("Failed to send number");
      break;
    }
    else
    {
      printf("Queue size [byte]: %ld to client %d\n", queueSizeNBO, client_id);
    }


    for(int i = 0;i<sharedBuf->consumersAmt;++i){
      
      uint64_t receivedMessagesByte = htonll(sharedBuf->receivedMessagesPerConsumer[i]);
        if (send(currSd, &receivedMessagesByte, sizeof(receivedMessagesByte), 0) == -1)
      {
        perror("Failed to send number");
        break;
      }
      else
      {
        printf("Messages byte: %ld to client %d\n", receivedMessagesByte, client_id);
      }     
    }
  }

  // Close the connection after sending the number
  printf("Connection terminated for client %d\n", client_id);
  close(currSd);
}


void convertUpdatedData(uint64_t *producedMessagesByte, uint64_t *queueSizeByte) {
    producedMessagesByte = htonll(sharedBuf->producedMessages);  
    queueSizeByte = htonll(sharedBuf->queueSize); 
}


int generateId(){
  pthread_mutex_lock(&client_id_mutex);
  client_id++;
  pthread_mutex_unlock(&client_id_mutex);
  return client_id;
}

uint64_t htonll(uint64_t value) {
    return (((uint64_t)htonl(value)) << 32) + htonl(value >> 32);
}

uint64_t ntohll(uint64_t value) {
    return (((uint64_t)ntohl(value)) << 32) + ntohl(value >> 32);
}


/*static void *readingProcess(void *arg){
  while(1){
    
    printf("Number of messages in the queue: %lu\n", sharedBuf->queueSize);  
    printf("Number of produced elements so far: %d\n",sharedBuf->producedMessages);

    for (int i = 0; i<sharedBuf->consumersAmt; i++)
      printf("consumer:%d, received: %d\n",i+1,sharedBuf->receivedMessagesPerConsumer[i]);

    sleep(10);
  }
}*/
