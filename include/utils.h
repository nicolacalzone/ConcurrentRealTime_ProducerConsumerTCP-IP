#ifndef UTILS_H_
#define UTILS_H_

#define MAX_THREADS 10
#define SHM_KEY_PROD_CON 2222
#define SHM_KEY_MONITOR_SERVER 1111
#define MESSAGE_BUFFER_SIZE 512

struct BufferDataMonitorServer {
  int receivedMessagesPerConsumer[MAX_THREADS];
  int producedMessages;
  int queueLength;
  int numOfConsumers;
};


#endif // UTILS_H_