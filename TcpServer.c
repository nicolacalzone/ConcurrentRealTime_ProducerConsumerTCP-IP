#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "utils.h"

// Global variables
int EXIT = 0;

struct BufferDataMonitorServer *sharedBuf;

// Function to handle the connection
static void handleConnection(int currSd, int client_id)
{


    int numOfConsumersByte = sharedBuf->numOfConsumers;
  //send the num of consumers
    if (send(currSd, &numOfConsumersByte, sizeof(numOfConsumersByte), 0) == -1)
    {
      perror("Failed to send number");
    }

  while(1)
  {

    int producedMessagesByte = htonl(sharedBuf->producedMessages);
    int queueLengthByte = htonl(sharedBuf->queueLength);

    // Send the number to the client
    if (send(currSd, &producedMessagesByte, sizeof(producedMessagesByte), 0) == -1)
    {
      perror("Failed to send number");
      break;
    }

    if (send(currSd, &queueLengthByte, sizeof(queueLengthByte), 0) == -1)
    {
      perror("Failed to send number");
      break;
    }

    for(int i = 0;i<sharedBuf->numOfConsumers;++i){
      
      int receivedMessagesByte = htonl(sharedBuf->receivedMessagesPerConsumer[i]);
        if (send(currSd, &receivedMessagesByte, sizeof(receivedMessagesByte), 0) == -1)
      {
        perror("Failed to send number");
        break;
      }    
    }

    // Sleep for a short period to simulate time delay between updates
    sleep(0.5);
  }

  // Close the connection after sending the number
  printf("Connection terminated for client %d\n", client_id);
  close(currSd);
}

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

// Main Program
int main(int argc, char *argv[])
{
  int sd, *arg;
  int port, sharedMemId;;
  struct sockaddr_in sin, retSin;
  pthread_t threads[MAX_THREADS];


  key_t keyForMonitor = ftok("tmp",SHM_KEY_MONITOR_SERVER);
  /* Set-up shared memory */
  sharedMemId = shmget(keyForMonitor, sizeof(struct BufferDataMonitorServer), SHM_R);
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
  sharedBuf->queueLength = 0;

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

  // Set socket options REUSE ADDRESS
  int reuse = 1;
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
  {
    perror("setsockopt(SO_REUSEADDR) failed");
  }

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

  // Set the maximum queue length for clients requesting connection to 5
  if (listen(sd, 5) == -1)
  {
    perror("listen");
    exit(1);
  }

  // Accept and serve all incoming connections in a loop
  for (int i = 0; i < MAX_THREADS; ++i)
  {
    socklen_t sAddrLen = sizeof(retSin);

    // Allocate the current socket and client ID.
    arg = (int *)malloc(2 * sizeof(int));
    if ((arg[0] = accept(sd, (struct sockaddr *)&retSin, &sAddrLen)) == -1)
    {
      perror("accept");
      exit(1);
    }

    arg[1] = i;

    // Connection received, start a new thread serving the connection
    pthread_create(&threads[i], NULL, connectionHandler, arg);
  }

  return 0; // never reached
}
