/* This page contains a client program that can request a file from the server program
 * on the next page. The server responds by sending the whole file.
 */

#include "file-server.h"

int main(int argc, char **argv)
{
  int c, net_socket, bytes;
  char buf[BUF_SIZE];		     /* buffer for incoming file */
  struct hostent *h;		     /* info about server */
  struct sockaddr_in channel;  /* holds IP address */

  if (argc != 3) fatal("Usage: client server-name file-name");
  
  /*
    Use DNS to get the IP address of the desired remote host.
  */
  h = gethostbyname(argv[1]);		/* look up host's IP address */
  if (!h) fatal("gethostbyname DNS-query failed. No host present.");

  /* ---------------------------------------------------------------
   * socket() - Socket creation, TCP
   * --------------------------------------------------------------- */
  net_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (net_socket < 0) fatal("socket");
  
  /* Remote socket to connect to */
  memset(&channel, 0, sizeof(channel));
  channel.sin_family = AF_INET;             /* specify family of the address socket */
  memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
  channel.sin_port = htons(SERVER_PORT);    /* uses port value provided by file-server.h */

  /* ---------------------------------------------------------------
   * connect() - TCP Handshake with server.
   * --------------------------------------------------------------- */
  c = connect(net_socket, (struct sockaddr *) &channel, sizeof(channel));
  if (c < 0) fatal("Connection Error to the remote socket.");

  /* ---------------------------------------------------------------
   * Send filename to server to request the file.
   * strlen(argv[2])+1 includes the null terminator so server knows
   * where the filename ends.
   * --------------------------------------------------------------- */
  write(net_socket, argv[2], strlen(argv[2]) + 1);

  /* ---------------------------------------------------------------
   * PHASE 2: Receive file data and write to a local file.
   * fopen with "wb" (write binary) ensures no byte translation
   * occurs — required for non-text files like images or executables.
   * argv[2] is reused as the local filename.
   * --------------------------------------------------------------- */
  FILE *outfile = fopen(argv[2], "wb");
  if (!outfile) fatal("fopen() failed — cannot create output file");

  while (1) {
    bytes = read(net_socket, buf, BUF_SIZE);  /* read a chunk from socket */
    if (bytes <= 0) break;                    /* 0 = server closed (EOF)
                                                 <0 = error; stop either way */
    fwrite(buf, 1, bytes, outfile);           /* write exactly those bytes
                                                 to the file, no extras */
  }

  /* ---------------------------------------------------------------
   * CLEANUP — close file and socket when done.
   * --------------------------------------------------------------- */
  fclose(outfile);
  close(net_socket);

  return 0;
}