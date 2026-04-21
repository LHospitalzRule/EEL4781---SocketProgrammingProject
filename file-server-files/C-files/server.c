/* This is the server code */
#include "file-server.h"
#include <sys/fcntl.h>
#include <arpa/inet.h>

#define QUEUE_SIZE 10






int main(int argc, char *argv[])
{	
  int net_socket, b, l, fd, sa, bytes, on = 1;
  int debugFlag = 0; /* monitoring DEBUG mode*/
  char buf[BUF_SIZE];		/* buffer for outgoing file */
  struct sockaddr_in channel;		/* holds IP address for*/
  struct sockaddr_in clientAddress;  /* Holds clientIP address*/
  

  /* Check for DEBUG flag */
  if(argc==2 && strcmp(argv[1], "DEBUG=1") == 0){
    debugFlag = 1;
  }

  /* Build address structure to bind to socket. */
  memset(&channel, 0, sizeof(channel));	/* zero channel */
  channel.sin_family = AF_INET;
  channel.sin_addr.s_addr = htonl(INADDR_ANY);
  channel.sin_port = htons(SERVER_PORT);

  /* Passive open. Wait for connection. */
  net_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);    /* create socket */
  if (net_socket < 0) fatal("socket failed");
  setsockopt(net_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));

  b = bind(net_socket, (struct sockaddr *) &channel, sizeof(channel));
  if (b < 0) fatal("bind failed");

  l = listen(net_socket, QUEUE_SIZE);		/* specify queue size */
  if (l < 0) fatal("listen failed");

  /* Socket is now set up and bound. Wait for connection and process it. */
  while (1) {
        socklen_t clientLength = sizeof(clientAddress); /* observe address length */
        sa = accept(net_socket, (struct sockaddr *) &clientAddress, &clientLength);		/* Save a section for distinct connection request */
        if (sa < 0) fatal("accept failed");

        char *client_IP = inet_ntoa(clientAddress.sin_addr);

        read(sa, buf, BUF_SIZE);		/* read file name from socket */

        if (debugFlag) printf("Sending %s to %s\n", buf, client_IP); 

        /* Get and return the file. */
        fd = open(buf, O_RDONLY);	/* open the file to be sent back */
        if (fd < 0) fatal("open failed");

        while (1) {
                bytes = read(fd, buf, BUF_SIZE);	/* read from file */
                if (bytes <= 0) break;		/* check for end of file */
                write(sa, buf, bytes);		/* write bytes to socket */
        }

        if (debugFlag) printf("Finished sending %s to %s\n", buf, client_IP);
        
        close(fd);			/* close file */
        close(sa);			/* close connection */
  }
}
