#ifndef UTILS_H_
#define UTILS_H_

#define MAX_THREADS 50
#define SHM_KEY 1414                // token to identify the shared memory segment b/w Prod and Con
#define SHM_KEY_MONITOR_SERVER 9999 // token to identify the shared memory b/w Server and Monitor
#define BUFFER_SIZE 256

struct BufferDataMonitorServer {
  int receivedMessagesPerConsumer[MAX_THREADS]; // every index is a consumer. every vec[index] is the amt of receivedMessages
  int producedMessages;
  int queueSize;
  int consumersAmt;
};


#endif // UTILS_H_