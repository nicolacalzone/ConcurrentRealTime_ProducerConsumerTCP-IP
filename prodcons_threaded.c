#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <mqueue.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_THREADS 250

#define SHM_KEY 5678

#define SHM_KEY_MONITOR_SERVER 1234


#define BUFFER_SIZE 264


/* Shared Buffer, indexes and semaphores are held in shared memory
   readIdx is the index in the buffer of the next item to be retrieved
   writeIdx is the index in the buffer of the next item to be inserted
   Buffer empty condition corresponds to readIdx == writeIdx
   Buffer full condition corresponds to
   (writeIdx + 1)%BUFFER_SIZE == readIdx)
   Semaphores used for synchronization:
   mutexSem is used to protect the critical section
   dataAvailableSem is used to wait for data avilability
   roomAvailableSem is used to wait for room abailable in the buffer */

struct BufferData {
  int buffer[BUFFER_SIZE];
  int receivedMessagesPerConsumer[MAX_THREADS];
  int producedMessages;
  
  /* The mutex used to protect shared data */
  pthread_mutex_t mutex;
  
  /* Condition variables to signal availability
   of room and data in the buffer */
  pthread_cond_t roomAvailable;
  pthread_cond_t dataAvailable;

  /* readIdx is the index in the buffer of the next item to be retrieved */
  int readIdx;
  
  /* writeIdx is the index in the buffer of the next item to be inserted */
  int writeIdx;

  int numOfConsumers;
   
};


struct BufferDataMonitorServer {
  int receivedMessagesPerConsumer[MAX_THREADS];
  int producedMessages;
  int queueLength;
  int numOfConsumers;
};


//structure of shared buffer between monitor and server


struct BufferData *sharedBuf;

struct BufferDataMonitorServer *sharedBufMonitorServer;


int numOfProducedMessages = 0;


static void *monitor(void *arg)
{

  sharedBufMonitorServer->numOfConsumers = sharedBuf->numOfConsumers;
  while(1){

      //  Buffer empty condition corresponds to readIdx == writeIdx
  //  Buffer full condition corresponds to
  //  (writeIdx + 1)%BUFFER_SIZE == readIdx)
    size_t num_elements = 0;
    if(sharedBuf->readIdx == sharedBuf->writeIdx){
      num_elements = 0;
    }
    else if ((sharedBuf->writeIdx + 1)%BUFFER_SIZE == sharedBuf->readIdx){
      num_elements = BUFFER_SIZE;
    }
    else{
      if(sharedBuf->writeIdx > sharedBuf->readIdx){
        num_elements = sharedBuf->writeIdx - sharedBuf->readIdx;
      }
      else if(sharedBuf->readIdx > sharedBuf->writeIdx){
        int diff = abs(sharedBuf->writeIdx - sharedBuf->readIdx);
        num_elements = BUFFER_SIZE - diff;
      }
    }

    sharedBufMonitorServer->queueLength = num_elements;
    sharedBufMonitorServer->producedMessages = sharedBuf->producedMessages;
    
    printf("Number of messages in the queue: %lu\n", num_elements);  


    printf("the number of produced elements so far:%d\n",sharedBuf->producedMessages);

    for (int i = 0;i<sharedBuf->numOfConsumers;i++){
      sharedBufMonitorServer->receivedMessagesPerConsumer[i] = sharedBuf->receivedMessagesPerConsumer[i];
      printf("consumer:%d, received: %d\n",i+1,sharedBuf->receivedMessagesPerConsumer[i]);
    
    }

    sleep(10);
  }
}



/* Consumer Code: the passed argument is not used */
static void *consumer(void *arg)
{
  //ID of current thread
  int ID = (__intptr_t ) arg;
  int numOfReceivedMessages = 0;
  int item;

  while(1)
  {
    // printf("fuck you!\n");
/* Enter critical section */
    pthread_mutex_lock(&sharedBuf->mutex);
/* If the buffer is empty, wait for new data */
    while(sharedBuf->readIdx == sharedBuf->writeIdx)
    {
      pthread_cond_wait(&sharedBuf->dataAvailable, &sharedBuf->mutex);
    }

    // printf("fuck you After lock!\n");
/* At this point data are available
   Get the item from the buffer */

    int item;

    /* Get data item */
    item = sharedBuf->buffer[sharedBuf->readIdx];

    // printf("the item is: %d from consumer:%d Num of received messages: %d\n",item,ID,numOfReceivedMessages);
    
    numOfReceivedMessages += 1;
    sharedBuf->receivedMessagesPerConsumer[ID-1] = numOfReceivedMessages;

    // printf("fuck you After !\n");
    // printf("received messages:%d\n",sharedBuf->receivedMessagesPerConsumer[ID-1]);
    // sleep(4);

    // item = sharedBuf->buffer[readIdx];
    sharedBuf->readIdx = (sharedBuf->readIdx + 1)%BUFFER_SIZE;
/* Signal availability of room in the buffer */
    pthread_cond_signal(&sharedBuf->roomAvailable);
/* Exit critical section */
    pthread_mutex_unlock(&sharedBuf->mutex);

 /* Consume the item and take actions (e.g. return)*/
   // ...
  }
  return NULL;
}
/* Producer code. Passed argument is not used */
static void *producer(void *arg)
{
  
  static int rateOfchange = 1;
  int item = 20;
  while(1)
  {
/* Produce a new item and take actions (e.g. return) */
    //  ...
/* Enter critical section */
    pthread_mutex_lock(&sharedBuf->mutex);
/* Wait for room availability */
    while((sharedBuf->writeIdx + 1)%BUFFER_SIZE == sharedBuf->readIdx)
    {
      pthread_cond_wait(&sharedBuf->roomAvailable, &sharedBuf->mutex);
    }
/* At this point room is available
   Put the item in the buffer */

/* Write data item */
    item += rateOfchange;

    sharedBuf->buffer[sharedBuf->writeIdx] = item;
    
    numOfProducedMessages += 1;
    // printf("We produced: %d\n",numOfProducedMessages);

    sharedBuf->producedMessages = numOfProducedMessages;

    // sleep(1);
    
    sharedBuf->writeIdx = (sharedBuf->writeIdx + 1)%BUFFER_SIZE;
/* Signal data avilability */
    pthread_cond_signal(&sharedBuf->dataAvailable);
/* Exit critical section */
    pthread_mutex_unlock(&sharedBuf->mutex);
  }
  return NULL;
}

int main(int argc, char *args[])
{
  pthread_t threads[MAX_THREADS];
  int nConsumers;
  int i;
  int sharedMemId;
  int sharedMemMonServerId;

/* The number of consumer is passed as argument */
  if(argc != 2)
  {
    printf("Usage: prod_cons <numConsumers>\n");
    exit(0);
  }
  sscanf(args[1], "%d", &nConsumers);


/* Set-up shared memory */
  sharedMemId = shmget(SHM_KEY, sizeof(struct BufferData),IPC_CREAT | SHM_R | SHM_W);
  if(sharedMemId == -1)
  {
    perror("Error in shmget");
    exit(0);
  }
  sharedBuf = shmat(sharedMemId, NULL, 0);
  if(sharedBuf == (void *)-1)
  {
    perror("Error in shmat");
    exit(0);
  }  


/* Set-up shared memory for monitor and server */
  sharedMemMonServerId = shmget(SHM_KEY_MONITOR_SERVER, sizeof(struct BufferDataMonitorServer),IPC_CREAT | SHM_R | SHM_W);
  if(sharedMemMonServerId == -1)
  {
    perror("Error in shmget");
    exit(0);
  }
  sharedBufMonitorServer = shmat(sharedMemMonServerId, NULL, 0);
  if(sharedBuf == (void *)-1)
  {
    perror("Error in shmat");
    exit(0);
  }  

/* Initialize buffer indexes */
  sharedBuf->readIdx = 0;
  sharedBuf->writeIdx = 0;
  sharedBuf->numOfConsumers = nConsumers;


/* Initialize mutex and condition variables */
  pthread_mutex_init(&sharedBuf->mutex, NULL);
  pthread_cond_init(&sharedBuf->dataAvailable, NULL);
  pthread_cond_init(&sharedBuf->roomAvailable, NULL);



/* Create producer thread */
  pthread_create(&threads[0], NULL, producer, NULL);

  /* Create monitor thread */
  
  pthread_create(&threads[1], NULL, monitor, NULL);
/* Create consumer threads */
  for(i = 0; i < nConsumers; i++)
    pthread_create(&threads[i+2], NULL, consumer, ( void * )( __intptr_t ) i+1);



/* Wait termination of all threads */
  for(i = 0; i < nConsumers + 2; i++)
  {
    pthread_join(threads[i], NULL);
  }
  return 0;
}