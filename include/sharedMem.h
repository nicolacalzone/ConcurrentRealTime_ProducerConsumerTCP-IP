#ifndef UTILS_H_
#define UTILS_H_

#define MAX_THREADS 250
#define SHM_KEY 5678
#define SHM_KEY_MONITOR_SERVER 1234
#define BUFFER_SIZE 264

struct BufferDataMonitorServer {
  int receivedMessagesPerConsumer[MAX_THREADS]; // every index is a consumer. every vec[index] is the amt of receivedMessages
  int producedMessages;
  int queueSize;
  int consumersAmt;
};


#endif // UTILS_H_