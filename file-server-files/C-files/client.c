/* This page contains a client program that can request a file from the server program
 * on the next page. The server responds by sending the whole file.
 */

#include "file-server.h"

int main(int argc, char **argv)
{
  int c, net_socket, bytes;
  char buf[BUF_SIZE];		/* buffer for incoming file */
  struct hostent *h;		/* info about server */
  struct sockaddr_in channel;		/* holds IP address */

  if (argc != 3) fatal("Usage: client <server-name> <file-name>");
  
  h = gethostbyname(argv[1]);		/* look up host's IP address */
  if (!h) fatal("gethostbyname failed");

  // Socket Creation, TCP
  net_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (net_socket <0) fatal("socket");
  
  /*  -------
      socket() - Socket creation.
      -------
  */  
  // Remote socket to connect to
  memset(&channel, 0, sizeof(channel));
  channel.sin_family= AF_INET;  // specify family of the address socket

  // Providing port # for both ends
  memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
  channel.sin_port= htons(SERVER_PORT); // uses port value provided by file-server.h

  /*  -------
      connect() - TCP Handshake with server.
      -------
  */  
  c = connect(net_socket, (struct sockaddr *) &channel, sizeof(channel));
  if (c < 0) fatal("Connection Error to the remote socket.\n\n"); // Check connection process connect() for error

  /*  -------
      Start by sending a filename to request file sending to the server.
      -------
  */  

  /* Connection is now established. Send file name including 0 byte at end.
  
        Write in the socket a request from the server the fileName at argv[2]
  */
  write(net_socket, argv[2], strlen(argv[2])+1);

  /* EXAMPLE: ./client <serverName> <fileName>
            ./client eustis.eecs.ucf.edu fileName.txt
  */
  FILE *outfile = fopen(argv[2], "wb");
    if (!outfile) fatal("fopen() failed — cannot create output file");

    while (1) {
        bytes = read(net_socket, buf, BUF_SIZE); /* read a chunk from socket  */
        if (bytes <= 0) break;                   /* End if no bytes were observed in the socket */
        fwrite(buf, 1, bytes, outfile);          /* write exactly those bytes into a file */
    }
 
    /* ---------------------------------------------------------------
     * CLEANUP
     * Close both the file and the socket when done.
     * --------------------------------------------------------------- */
    fclose(outfile);
    close(net_socket);
}
