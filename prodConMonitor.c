#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "utils.h"
#include <signal.h>


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

struct BufferDataProdCon {
  char *messageBuffer[MESSAGE_BUFFER_SIZE];
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

//structure of shared buffer between monitor and server


struct BufferDataProdCon *sharedBufProdCon;

struct BufferDataMonitorServer *sharedBufMonitorServer;



int numOfProducedMessages = 0;


static void *monitor(void *arg)
{

  sharedBufMonitorServer->numOfConsumers = sharedBufProdCon->numOfConsumers;
  while(sharedBufProdCon->receivedMessagesPerConsumer[0]<1000000){

      //  Buffer empty condition corresponds to readIdx == writeIdx
  //  Buffer full condition corresponds to
  //  (writeIdx + 1)%BUFFER_SIZE == readIdx)
    size_t num_elements = 0;
    if(sharedBufProdCon->readIdx == sharedBufProdCon->writeIdx){
      num_elements = 0;
    }
    else if ((sharedBufProdCon->writeIdx + 1)%MESSAGE_BUFFER_SIZE == sharedBufProdCon->readIdx){
      num_elements = MESSAGE_BUFFER_SIZE;
    }
    else{
      if(sharedBufProdCon->writeIdx > sharedBufProdCon->readIdx){
        num_elements = sharedBufProdCon->writeIdx - sharedBufProdCon->readIdx;
      }
      else if(sharedBufProdCon->readIdx > sharedBufProdCon->writeIdx){
        int diff = abs(sharedBufProdCon->writeIdx - sharedBufProdCon->readIdx);
        num_elements = MESSAGE_BUFFER_SIZE - diff;
      }
    }

    sharedBufMonitorServer->queueLength = num_elements;
    sharedBufMonitorServer->producedMessages = sharedBufProdCon->producedMessages;
    
    printf("Number of messages in the queue: %lu\n", num_elements);  


    printf("the number of produced elements so far:%d\n",sharedBufProdCon->producedMessages);

    for (int i = 0;i<sharedBufProdCon->numOfConsumers;i++){
      sharedBufMonitorServer->receivedMessagesPerConsumer[i] = sharedBufProdCon->receivedMessagesPerConsumer[i];
      printf("consumer:%d, received: %d\n",i+1,sharedBufProdCon->receivedMessagesPerConsumer[i]);
    
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
  char* item;

  while(numOfReceivedMessages < 1000000)
  {
/* Enter critical section */
    pthread_mutex_lock(&sharedBufProdCon->mutex);
/* If the buffer is empty, wait for new data */
    while(sharedBufProdCon->readIdx == sharedBufProdCon->writeIdx)
    {
      pthread_cond_wait(&sharedBufProdCon->dataAvailable, &sharedBufProdCon->mutex);
    }

    /* Get data item */
    item = sharedBufProdCon->messageBuffer[sharedBufProdCon->readIdx];
    
    numOfReceivedMessages += 1;
    sharedBufProdCon->receivedMessagesPerConsumer[ID-1] = numOfReceivedMessages;

    sharedBufProdCon->readIdx = (sharedBufProdCon->readIdx + 1)%MESSAGE_BUFFER_SIZE;
/* Signal availability of room in the buffer */
    pthread_cond_signal(&sharedBufProdCon->roomAvailable);
/* Exit critical section */
    pthread_mutex_unlock(&sharedBufProdCon->mutex);

  }
  return NULL;
}
/* Producer code. Passed argument is not used */
static void *producer(void *arg)
{
  
  char* item = "hello world!";
  while(1)
  {
/* Produce a new item and take actions (e.g. return) */
    //  ...
/* Enter critical section */
    pthread_mutex_lock(&sharedBufProdCon->mutex);
/* Wait for room availability */
    while((sharedBufProdCon->writeIdx + 1)%MESSAGE_BUFFER_SIZE == sharedBufProdCon->readIdx)
    {
      pthread_cond_wait(&sharedBufProdCon->roomAvailable, &sharedBufProdCon->mutex);
    }
/* At this point room is available
   Put the item in the buffer */

/* Write data item */

    sharedBufProdCon->messageBuffer[sharedBufProdCon->writeIdx] = item;
    
    numOfProducedMessages += 1;

    sharedBufProdCon->producedMessages = numOfProducedMessages;
    
    sharedBufProdCon->writeIdx = (sharedBufProdCon->writeIdx + 1)%MESSAGE_BUFFER_SIZE;
/* Signal data avilability */
    pthread_cond_signal(&sharedBufProdCon->dataAvailable);
/* Exit critical section */
    pthread_mutex_unlock(&sharedBufProdCon->mutex);
  }
  return NULL;
}



// Signal handler for SIGINT
void sigint_handler(int signal)
{
 if (signal == SIGINT)
  printf("\nRemocing all shared memory segments!\n");

  system("ipcs -m | awk 'NR>3 {print $2}' | xargs -n 1 ipcrm -m");
  exit(0);
}

void set_signal_action(void)
{
 // Declare the sigaction structure
 struct sigaction act;

 // Set all of the structure's bits to 0 to avoid errors
 // relating to uninitialized variables...
 bzero(&act, sizeof(act));
 // Set the signal handler as the default action
 act.sa_handler = &sigint_handler;
 // Apply the action in the structure to the
 // SIGINT signal (ctrl-c)
 sigaction(SIGINT, &act, NULL);
}


int main(int argc, char *args[])
{

  set_signal_action();


  pthread_t threads[MAX_THREADS];
  int nConsumers;
  int i;
  int sharedMemProdConId;
  int sharedMemMonServerId;

/* The number of consumer is passed as argument */
  if(argc != 2)
  {
    printf("Usage: prod_cons <numConsumers>\n");
    exit(0);
  }
  sscanf(args[1], "%d", &nConsumers);

  key_t key = ftok("/tmp",SHM_KEY_PROD_CON);


/* Set-up shared memory */
  sharedMemProdConId = shmget(key, sizeof(struct BufferDataProdCon), IPC_CREAT | SHM_R | SHM_W);
  if(sharedMemProdConId == -1)
  {
    perror("Error in shmget");
    exit(0);
  }
  sharedBufProdCon = shmat(sharedMemProdConId, NULL, 0);
  if(sharedBufProdCon == (void *)-1)
  {
    perror("Error in shmat");
    exit(0);
  }  




  key_t keyForMonitor = ftok("tmp",SHM_KEY_MONITOR_SERVER);
/* Set-up shared memory for monitor and server */
  sharedMemMonServerId = shmget(keyForMonitor, sizeof(struct BufferDataMonitorServer),IPC_CREAT | SHM_R | SHM_W);
  if(sharedMemMonServerId == -1)
  {
    perror("Error in shmget");
    exit(0);
  }
  sharedBufMonitorServer = shmat(sharedMemMonServerId, NULL, 0);
  if(sharedBufMonitorServer == (void *)-1)
  {
    perror("Error in shmat");
    exit(0);
  }  

/* Initialize buffer indexes */
  sharedBufProdCon->readIdx = 0;
  sharedBufProdCon->writeIdx = 0;
  sharedBufProdCon->numOfConsumers = nConsumers;


/* Initialize mutex and condition variables */
  pthread_mutex_init(&sharedBufProdCon->mutex, NULL);
  pthread_cond_init(&sharedBufProdCon->dataAvailable, NULL);
  pthread_cond_init(&sharedBufProdCon->roomAvailable, NULL);



/* Create producer thread */
  pthread_create(&threads[0], NULL, producer, NULL);

  /* Create monitor thread */
  
  pthread_create(&threads[1], NULL, monitor, NULL);
/* Create consumer threads */
  for(i = 0; i < nConsumers; i++)
    pthread_create(&threads[i+2], NULL, consumer, ( void * )( __intptr_t ) i+1);



/* Wait termination of all threads */
  for(i = 1; i < nConsumers + 2; i++)
  {
    pthread_join(threads[i], NULL);
    printf("thread %d finished\n",i);
  }

  printf("clearing all shared memory segments!");
  shmdt(sharedBufProdCon);
  shmdt(sharedBufMonitorServer);

  shmctl(sharedMemProdConId,IPC_RMID,NULL);
  shmctl(sharedMemMonServerId,IPC_RMID,NULL);

  // system("ipcs -m | awk 'NR>3 {print $2}' | xargs -n 1 ipcrm -m");
  return 0;
}