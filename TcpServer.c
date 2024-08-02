#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static void handleConnection(int currSd)
{
  int numberToSend = 42;               // The number to send to the client
  int netNumber = htonl(numberToSend); // Convert to network byte order

  // Send the number to the client
  if (send(currSd, &netNumber, sizeof(netNumber), 0) == -1)
  {
    perror("Failed to send number");
  }
  else
  {
    printf("Sent number %d to client\n", numberToSend);
  }

  // Close the connection after sending the number
  printf("Connection terminated\n");
  close(currSd);
}

/* Main Program */
int main(int argc, char *argv[])
{
  int sd, currSd;
  int port;
  struct sockaddr_in sin, retSin;

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
  for (;;)
  {
    socklen_t sAddrLen = sizeof(retSin);
    if ((currSd = accept(sd, (struct sockaddr *)&retSin, &sAddrLen)) == -1)
    {
      perror("accept");
      exit(1);
    }

    // When execution reaches this point, a client has established the connection.
    printf("Connection received from %s\n", inet_ntoa(retSin.sin_addr));
    handleConnection(currSd);
  }

  return 0;
}
