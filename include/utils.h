#ifndef UTILS_H_
#define UTILS_H_

#define MAX_THREADS 250
#define SHM_KEY 5678
#define SHM_KEY_MONITOR_SERVER 1234
#define BUFFER_SIZE 264

struct BufferDataMonitorServer {
  int receivedMessagesPerConsumer[MAX_THREADS];
  int producedMessages;
  int queueLength;
  int numOfConsumers;
};


#endif // UTILS_H_