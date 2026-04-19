/* This page contains a client program that can request a file from the server program
 * on the next page. The server responds by sending the whole file.
 */

#include "file-server.h"

int main(int argc, char **argv)
{
  int c, net_socket, bytes;
  char buf[BUF_SIZE];          /* buffer for incoming file */
  struct hostent *h;           /* info about server */
  struct sockaddr_in channel;  /* holds IP address */

  /* ---------------------------------------------------------------
   * PHASE 4: BYTE RANGE VARIABLES
   * Default 0 means "no range specified — send whole file"
   * --------------------------------------------------------------- */
  int start_byte = 0;
  int end_byte   = 0;

  /* ---------------------------------------------------------------
   * ARGUMENT CHECK
   * Minimum: ./client server-name file-name          (argc == 3)
   * With range: ./client server-name file-name -s 3 -e 11 (argc == 7)
   * --------------------------------------------------------------- */
  if (argc < 3) fatal("Usage: client server-name file-name [-s START -e END]");

  /*
    Use DNS to get the IP address of the desired remote host.
  */
  h = gethostbyname(argv[1]);     /* look up host's IP address */
  if (!h) fatal("gethostbyname DNS-query failed. No host present.");

  /* ---------------------------------------------------------------
   * PHASE 4: PARSE -s AND -e FLAGS
   * Loop through argv starting at index 3 (after server and filename).
   * atoi() converts a string like "3" to the integer 3.
   * ++i advances past the flag to grab its value.
   * --------------------------------------------------------------- */
  for (int i = 3; i < argc; i++) {
    if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
      start_byte = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
      end_byte = atoi(argv[++i]);
    }
  }

  /* ---------------------------------------------------------------
   * PHASE 4: VALIDATE BYTE RANGE
   * Catch bad input before we even connect to the server.
   * --------------------------------------------------------------- */
  if (start_byte < 0 || end_byte < 0) {
    fatal("Error: byte range values must be positive integers");
  }
  if ((start_byte > 0 || end_byte > 0) && end_byte < start_byte) {
    fatal("Error: END_BYTE must be greater than or equal to START_BYTE");
  }

  /* ---------------------------------------------------------------
   * socket() - Socket creation, TCP
   * --------------------------------------------------------------- */
  net_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (net_socket < 0) fatal("socket");

  /* Remote socket to connect to */
  memset(&channel, 0, sizeof(channel));
  channel.sin_family = AF_INET;            /* specify family of the address socket */
  memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
  channel.sin_port = htons(SERVER_PORT);   /* uses port value provided by file-server.h */

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
   * PHASE 4: Send byte range to server.
   * We send start and end as raw integers after the filename.
   * If no range was given both are 0 — server sends whole file.
   * --------------------------------------------------------------- */
  write(net_socket, &start_byte, sizeof(int));
  write(net_socket, &end_byte, sizeof(int));

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