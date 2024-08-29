#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "utils.h"

static int receive(int sd, void *buffer, int size)
{
  int totalSize = 0, currentSize;

  int counter = 0;
  while (totalSize < size)
  {
    currentSize = recv(sd, (char *)buffer + totalSize, size - totalSize, 0);
    if (currentSize <= 0)
    {
      // Error or connection closed
      return -1;
    }

    totalSize += currentSize;
  }
  // Success
  return 0;
}

int main(int argc, char **argv)
{
  char hostname[100];
  int sd;
  int port;
  struct sockaddr_in sin;
  struct hostent *hp;

  // Check number of arguments and get IP address and port
  if (argc < 3)
  {
    printf("Usage: client <hostname> <port>\n");
    exit(0);
  }
  sscanf(argv[1], "%s", hostname);
  sscanf(argv[2], "%d", &port);

  // Resolve the passed name and store the resulting long representation
  if ((hp = gethostbyname(hostname)) == 0)
  {
    perror("gethostbyname");
    exit(1);
  }

  // Fill in the socket structure with host information
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;
  sin.sin_port = htons(port);

  // Create a new socket
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  // Connect the socket to the port and host specified in struct sockaddr_in
  if (connect(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1)
  {
    perror("connect");
    exit(1);
  }

  // Variable to hold the received number
  int receivedNumber;
  int counter=0;
  int numOfConsumers = 0;
  int consumer = 0;

  int producedMessage;
  int queueLength;
  int receivedmessage;



  // Receive num of consumers
  if (receive(sd, &numOfConsumers, sizeof(numOfConsumers)) == -1)
  {
    perror("recv");
    close(sd);
    exit(1);
  }  

  // Loop to continuously receive numbers from the server
  while (1)
  {
    // Receive the number from the server
    if (receive(sd, &producedMessage, sizeof(producedMessage)) == -1)
    {
      perror("recv");
      close(sd);
      exit(1);
    }
    else{
      printf("produced Messages %d\n",ntohl(producedMessage));
    }

    // Receive the number from the server
    if (receive(sd, &queueLength, sizeof(queueLength)) == -1)
    {
      perror("recv");
      close(sd);
      exit(1);
    }
    else{
      printf("Queue Length %d\n",ntohl(queueLength));
    }

    for(int i = 0;i<numOfConsumers;++i){
      
      // int receivedMessagesByte = htonl(sharedBuf->receivedMessagesPerConsumer[i]);
   // Receive the number from the server
      if (receive(sd, &receivedmessage, sizeof(receivedmessage)) == -1)
      {
        perror("recv");
        close(sd);
        exit(1);
      }
      else
        {
          printf("Consumer %d received:%d\n",i,ntohl(receivedmessage));
        }     
    }        


    //print produced messages
    // if(counter == 0){
    //   printf("produced Messages %d\n",ntohl(receivedNumber));
    //   counter++;
    // }

    // //print queue length
    // if(counter == 1){
    //   printf("Queue Length %d\n",ntohl(receivedNumber));
    //   counter++;
    // }

    // //print received messages for every consumer
    // // printf("num of consumers:%d",numOfConsumers);
    // if(counter >= 2){
    //   // for(int i = 0;i<numOfConsumers;++i){
    //   printf("Consumer %d received:%d\n",consumer,ntohl(receivedNumber));
    //   counter++;
    //   consumer++;
    //   // }

    // }

    //cehck if we are at the end of the counter loop
    // if((counter + 1) % (1+numOfConsumers) == 0){
    //   counter = 0;
    //   consumer = 0;
    // }
    // printf("Received number: %d\n", ntohl(receivedNumber));
  }

  // Close the socket (never reached in this loop)
  close(sd);
  return 0;
}
