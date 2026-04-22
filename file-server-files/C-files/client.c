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
  int startByte = 0, finByte = 0;     /* To track byte reading */
  char fileRequest[BUF_SIZE]; // Buffer to hold requested fileName
  char *fileNameHolder = NULL; // temporary storage for the requested file
  int writeFlag = 0; /* Monitoring for client-write requests */
  
// Check the arguments and parameters
  if (argc < 3) fatal("Usage: client <server-name> <file-name> \n\nOptions:\n     Request a section of a file: \"-s <startingByte> -e <endingByte>\"\n     Upload a file to the server: \"client <server-name> [-w] <file-name>\"\n");

 /* Check for write. Once checked, swap -w and the fileName because it's easier to work with*/
  if(strcmp(argv[2], "-w") == 0){
     writeFlag = 1;
     fileNameHolder = argv[3];  /* filename is after -w */
 } else {
     fileNameHolder = argv[2];  /* no -w, filename is here */
 }
  
  /* Check for possible byte range request */
  if(argc > 3){
    for(int i = 3; i < argc; i++){
        if(strcmp(argv[i], "-s") == 0){
            startByte = atoi(argv[++i]);
        } else if(strcmp(argv[i], "-e") == 0){
            finByte = atoi(argv[++i]);
        }
    }
  }
    
/*
  Validating Byte range. Error check valid range
*/
 if(startByte < 0 || finByte < 0){
    fatal("Error: byte range values must be positive integers");
 }
 if((startByte == 0 && finByte > 0) || (startByte > 0 && finByte == 0)){
     fatal("Error: must provide both -s and -e flags together");
 }
 if(startByte > 0 && finByte < startByte){
     fatal("Error: END_BYTE must be greater than or equal to START_BYTE");
 }

  h = gethostbyname(argv[1]);		/* look up host's IP address */
  if (!h) fatal("gethostbyname failed");


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

  /*  -------
      Start by sending a filename to request file sending to the server.
      -------
  */  

  /* Connection is now established. Send file name including 0 byte at end.

        Write in the socket a request from the server the fileName at argv[2]
        Added:
            > snprintf to request specific byte range via string imposed onto buffer.
  */
  if(writeFlag){
    snprintf(fileRequest, BUF_SIZE, "%s %d %d WRITE\n", fileNameHolder, startByte, finByte);
  } else {
    snprintf(fileRequest, BUF_SIZE, "%s %d %d READ\n", fileNameHolder, startByte, finByte);
  }
  write(net_socket, fileRequest, strlen(fileRequest));

  /* EXAMPLE: ./client <serverName> <fileName>
            ./client eustis.eecs.ucf.edu fileName.txt
  */
  if(writeFlag){
    /* -------
        PUT - Upload local file to server.
        Open the local file, read it, and send it over the socket.
       -------
    */
    FILE *infile = fopen(fileNameHolder, "rb");
    if(!infile) fatal("Error: file not found locally");
    
    while(1){
        bytes = fread(buf, 1, BUF_SIZE, infile);  /* read from local file */
        if(bytes <= 0) break;
        write(net_socket, buf, bytes);             /* send bytes to server */
    }
    fclose(infile);

  } else {
    /* -------
        GET - Download file from server.
        Receive file data from server and save it locally.
       -------
    */
    FILE *outfile = fopen(fileNameHolder, "wb");
    if(!outfile) fatal("fopen() failed — cannot create output file");

    while(1){
        bytes = read(net_socket, buf, BUF_SIZE);   /* read a chunk from socket */
        if(bytes <= 0) break;                      /* End if no bytes were observed in the socket */

        if(strncmp(buf, "ERROR:", 6) == 0){        /* checks for the first few words in the error message */
            printf("%s", buf);                     /* print the error message */
            fclose(outfile);
            close(net_socket);
            exit(1);
        }

        fwrite(buf, 1, bytes, outfile);            /* write exactly those bytes into a file */
    }
    fclose(outfile);
  }

  /* ---------------------------------------------------------------
   * CLEANUP
   * Close both the file and the socket when done.
   * --------------------------------------------------------------- */
  close(net_socket);
}
