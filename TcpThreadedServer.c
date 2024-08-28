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

#define MAX_THREADS 10
#define SHM_KEY_MONITOR_SERVER 1234

// Global variables
int exit_status = 0;
int client_id_counter = 0;
pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;

struct BufferData {
  int receivedMessagesPerConsumer[MAX_THREADS];
  int producedMessages;
  int queueLenght;
  int numOfConsumers;
}; 

struct BufferData *sharedBuf;

// Simulate receiving an updated number
int getUpdatedNumber()
{
  static int number = 42; // Initial number
  return number++;
}

// Function to handle the connection
static void handleConnection(int currSd, int client_id)
{
  for (;;)
  {
    int numberToSend = getUpdatedNumber(); // Get the updated number
    int netNumber = htonl(numberToSend);   // Convert to network byte order

    // Send the number to the client
    if (send(currSd, &netNumber, sizeof(netNumber), 0) == -1)
    {
      perror("Failed to send number");
      break;
    }
    else
    {
      printf("Sent number %d to client %d\n", numberToSend, client_id);
    }

    // Sleep for a short period to simulate time delay between updates
    sleep(3);
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

static void *readingProcess(void *arg){
  while(1){
    
    printf("Number of messages in the queue: %lu\n", sharedBuf->queueLenght);  
    printf("Number of produced elements so far: %d\n",sharedBuf->producedMessages);

    for (int i = 0; i<sharedBuf->numOfConsumers; i++)
      printf("consumer:%d, received: %d\n",i+1,sharedBuf->receivedMessagesPerConsumer[i]);

    sleep(10);
  }
}

// Main Program
int main(int argc, char *argv[])
{
  int sd, *arg;
  int port, sharedMemId;;
  struct sockaddr_in sin, retSin;
  pthread_t threads[MAX_THREADS];
  pthread_t serverReader;

  /* Set-up shared memory */
  sharedMemId = shmget(SHM_KEY_MONITOR_SERVER, sizeof(struct BufferData), SHM_R);
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
  sharedBuf->queueLenght = 0;

  /* Create serverReader thread */
  pthread_create(&serverReader, NULL, readingProcess, NULL);

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

    // Generate a unique client ID
    pthread_mutex_lock(&client_id_mutex);
    arg[1] = client_id_counter++;
    pthread_mutex_unlock(&client_id_mutex);

    printf("Connection received from %s with client ID %d\n", inet_ntoa(retSin.sin_addr), arg[1]);

    // Connection received, start a new thread serving the connection
    pthread_create(&threads[i], NULL, connectionHandler, arg);
  }

  return 0; // never reached
}
