#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
/* For shared memory*/
#include "sharedMem.h"

/* Definition of Functions */
static int receive(int sd, void *buffer, int size);

/* MAIN */
int main(int argc, char **argv)
{
  /* Tcp/Ip needed variables */
  char hostname[100];
  int sd;
  int port;
  struct sockaddr_in sin;
  struct hostent *hp;
  
  /* Prod/Con needed variables */
  int receivedNumber;
  int counter=0;
  int consumersAmt = 0;
  int consumer = 0;
  int producedMessage;
  int queueSize;
  int receivedmessage;
  int len = sizeof(struct BufferDataMonitorServer);

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

  // Receive num of consumers
  if (receive(sd, &consumersAmt, sizeof(consumersAmt)) == -1)
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
    if (receive(sd, &queueSize, sizeof(queueSize)) == -1)
    {
      perror("recv");
      close(sd);
      exit(1);
    }
    else{
      printf("Queue Length %d\n",ntohl(queueSize));
    }

    for(int i=0; i<consumersAmt; i++){
      
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
  }
  
  // Close the socket (never reached in this loop)
  close(sd);
  return 0;
}

// ---------------------------------------------------------------------------------------------------

/* Description of Functions */

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