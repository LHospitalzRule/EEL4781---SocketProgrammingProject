/* This is the server code */
#include "file-server.h"
#include <sys/fcntl.h>
#include <arpa/inet.h>   /* inet_ntoa() — converts binary IP to readable string */

#define QUEUE_SIZE 10

int main(int argc, char *argv[])
{	
  /* ---------------------------------------------------------------
   * VARIABLE DECLARATIONS
   * --------------------------------------------------------------- */
  int net_socket;          /* welcome socket — listens for new connections  */
  int sa;                  /* connection socket — one per accepted client    */
  int b, l;                /* return values of bind() and listen()          */
  int fd;                  /* file descriptor for the file being sent       */
  int bytes;               /* bytes read from file each iteration           */
  int on = 1;              /* used by setsockopt() to enable SO_REUSEADDR   */
  int debug = 0;           /* DEBUG flag — 0 by default, 1 if DEBUG=1 given */
  char buf[BUF_SIZE];      /* buffer for outgoing file                      */

  /* ---------------------------------------------------------------
   * PHASE 4: BYTE RANGE VARIABLES
   * --------------------------------------------------------------- */
  int start_byte = 0;      /* start of requested byte range                 */
  int end_byte   = 0;      /* end of requested byte range                   */

  struct sockaddr_in channel;      /* server's own address struct            */
  struct sockaddr_in client_addr;  /* will hold the client's IP + port       */
  socklen_t client_len;            /* required by accept() — size of above   */
  char *client_ip;                 /* human-readable client IP string        */

  /* ---------------------------------------------------------------
   * PHASE 3: PARSE DEBUG FLAG
   * argv[1] will be the string "DEBUG=1" if provided.
   * strcmp returns 0 when strings match.
   * --------------------------------------------------------------- */
  if (argc == 2 && strcmp(argv[1], "DEBUG=1") == 0) {
    debug = 1;
    printf("Debug mode enabled\n");
  } else if (argc > 2) {
    fatal("Usage: server [DEBUG=1]");
  }

  /* Build address structure to bind to socket. */
  memset(&channel, 0, sizeof(channel));  /* zero channel */
  channel.sin_family = AF_INET;
  channel.sin_addr.s_addr = htonl(INADDR_ANY);
  channel.sin_port = htons(SERVER_PORT);

  /* Passive open. Wait for connection. */
  net_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);    /* create socket */
  if (net_socket < 0) fatal("socket failed");
  setsockopt(net_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));

  b = bind(net_socket, (struct sockaddr *) &channel, sizeof(channel));
  if (b < 0) fatal("bind failed");

  l = listen(net_socket, QUEUE_SIZE);    /* specify queue size */
  if (l < 0) fatal("listen failed");

  printf("Server is ready on port %d\n", SERVER_PORT);

  /* Socket is now set up and bound. Wait for connection and process it. */
  while (1) {

        /* ---------------------------------------------------------------
         * PHASE 3: ACCEPT WITH CLIENT ADDRESS
         * We pass &client_addr so accept() fills in the client's IP.
         * client_len must be set to the struct size before calling.
         * --------------------------------------------------------------- */
        client_len = sizeof(client_addr);
        sa = accept(net_socket, (struct sockaddr *) &client_addr, &client_len);
        if (sa < 0) fatal("accept failed");

        /* Convert binary IP to readable string e.g. "132.170.1.5"         */
        client_ip = inet_ntoa(client_addr.sin_addr);

        /* Read filename from socket                                         */
        read(sa, buf, BUF_SIZE);

        /* ---------------------------------------------------------------
         * PHASE 4: READ BYTE RANGE FROM CLIENT
         * Client sends start and end as raw integers right after filename.
         * We read them back in the same order they were sent.
         * --------------------------------------------------------------- */
        read(sa, &start_byte, sizeof(int));
        read(sa, &end_byte, sizeof(int));

        /* ---------------------------------------------------------------
         * PHASE 3: DEBUG — print before transfer starts
         * --------------------------------------------------------------- */
        if (debug) printf("Sending %s to %s\n", buf, client_ip);

        /* Get and return the file. */
        fd = open(buf, O_RDONLY);   /* open the file to be sent back */
        if (fd < 0) fatal("open failed");

        /* ---------------------------------------------------------------
         * PHASE 4: BYTE RANGE — lseek()
         * If start_byte is non-zero, jump to that position in the file.
         * Spec says bytes are 1-indexed so we seek to start_byte - 1.
         * bytes_to_send tracks how many bytes remain to send.
         * If both are 0, skip this and send the whole file.
         * --------------------------------------------------------------- */
        int bytes_to_send = -1;   /* -1 means send entire file              */

        if (start_byte > 0 && end_byte >= start_byte) {
          lseek(fd, start_byte - 1, SEEK_SET);
          bytes_to_send = end_byte - start_byte + 1;
        }

        /* ---------------------------------------------------------------
         * SEND FILE DATA
         * If bytes_to_send is -1, send until EOF.
         * Otherwise track total sent and stop at the requested range.
         * --------------------------------------------------------------- */
        int total_sent = 0;

        while (1) {
          int to_read = BUF_SIZE;

          /* Limit the final read to not overshoot the end byte             */
          if (bytes_to_send != -1) {
            int remaining = bytes_to_send - total_sent;
            if (remaining <= 0) break;
            if (remaining < BUF_SIZE) to_read = remaining;
          }

          bytes = read(fd, buf, to_read);   /* read from file               */
          if (bytes <= 0) break;            /* check for end of file        */
          write(sa, buf, bytes);            /* write bytes to socket        */
          total_sent += bytes;
        }

        /* ---------------------------------------------------------------
         * PHASE 3: DEBUG — print after transfer completes
         * --------------------------------------------------------------- */
        if (debug) printf("Finished sending %s to %s\n", buf, client_ip);

        close(fd);   /* close file       */
        close(sa);   /* close connection */
  }
}