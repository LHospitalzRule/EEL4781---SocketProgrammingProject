/* This is the server code */
#include "file-server.h"
#include <sys/fcntl.h>
#include <arpa/inet.h>

#define QUEUE_SIZE 10






int main(int argc, char *argv[])
{	
  int net_socket, b, l, fd, sa, bytes, on = 1;
  int debugFlag = 0; /* monitoring DEBUG mode*/

  char tempByte; /* Contain byte data for check and buffer storage */
  char buf[BUF_SIZE];		/* buffer for outgoing file */
  char mode[8];
  struct sockaddr_in channel;		/* holds IP address for*/
  struct sockaddr_in clientAddress;  /* Holds clientIP address*/
  

  /* Check for DEBUG flag */
  if(argc==2 && strcmp(argv[1], "DEBUG=1") == 0){
    debugFlag = 1;
    printf("DEBUG Mode ON. Displaying info...\n");
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
        int bufCtr  = 0; /* For tracking buffer size read-in requests per client */
        int startByte = 0, finByte = 0; /* Storage space for byte-range requests */

        socklen_t clientLength = sizeof(clientAddress); /* observe address length */
        sa = accept(net_socket, (struct sockaddr *) &clientAddress, &clientLength);		/* Save a socket space for distinct connection request */
        if (sa < 0) fatal("accept failed");

        char *client_IP = inet_ntoa(clientAddress.sin_addr);

        /*
            Replace read so that it can read the incoming byte range from the fileName in the buffer.
            Loop that continuously reads data into buffer until end-condition met.
            Our client-side : snprintf(fileRequest, BUF_SIZE, "%s %d %d\n", argv[2], startByte, finByte);
            > We need to match "%s %d %d\n and stop at \n"
        */
        //read(sa, buf, BUF_SIZE); /* Read file name from buffer */
        while(bufCtr < BUF_SIZE - 1){
          read(sa, &tempByte, 1);     // read in byte
          if(tempByte == '\n') break; // Check if it's the end of the args
          buf[bufCtr++] = tempByte;   // Add valid byte to buffer
        }

        buf[bufCtr] = '\0';


        char fileName[256]; /* Store the file request title */
        /* Read the info we stored in buf - store into their respective variables 
             "%s %d %d\n", argv[2], startByte, finByte =>>  "%s %d %d\n", fileName, startByte, finByte
        */
        sscanf(buf, "%s %d %d %s", fileName, &startByte, &finByte, mode);

        if(strcmp(mode, "WRITE") == 0){

          /* Check for file existencew */
          if(access(fileName, F_OK) == 0){
            char *existingFileError = "ERROR: File already exists on server - overwriting not allowed\n";
            printf("%s", existingFileError);
            write(sa, existingFileError, strlen(existingFileError));
            close(sa);
            continue;
          }

          /* [WRITE] - Server receeiving file from client */
          FILE *outfile = fopen(fileName, "wb");
          if(!outfile){
            char *fileNotFound = "ERROR: Cannot write file\n"; // error message
            printf("%s", fileNotFound);
            write(sa, fileNotFound, strlen(fileNotFound)); 
            close(sa);
            continue;  /* Close the socket, keep the server runnig */
          }
          while(1){
              bytes = read(sa, buf, BUF_SIZE);  /* read from client */
              if(bytes <= 0) break;
              fwrite(buf, 1, bytes, outfile);   /* write to local file */
          }

          if(debugFlag) printf("A file has been uploaded. File-name: %s\n", fileName);
          fclose(outfile);


        } else {

              /* Get and return the file. */
          fd = open(fileName, O_RDONLY);	/* open the file to be sent back */
          
          /* File not found */
          if (fd < 0){ 
            char *fileNotFound = "ERROR: File not found on server\n"; // error message
            printf("%s", fileNotFound);
            write(sa, fileNotFound, strlen(fileNotFound)); 
            close(sa);
            continue;  /* Close the socket, keep the server runnig */
            //fatal("Open Failed. File not detected... ending server activity.");
          }

          if (debugFlag) printf("Sending %s to %s\n", fileName, client_IP);

          /*
              Checking for byte range or not
          */
          int byteRangeCHK = -1; /* -1 means send whole file, else check range*/
          int totalSent = 0;

          /* Preparing the range of bytes */
          if(startByte > 0 && finByte >= startByte){
              printf("Byte Range request detected. Sending the data range requested.\n");
              lseek(fd, startByte - 1, SEEK_SET);  /* jump to start position (1-indexed to 0-indexed) */
              byteRangeCHK = finByte - startByte + 1; /* calculate how many bytes to send */
          }

          /* Reading the range of bytes */
          while (1) {
            int toRead = BUF_SIZE; /* Count of bytes to read */
            if(byteRangeCHK != -1){
                int remaining = byteRangeCHK - totalSent;
                if(remaining <= 0) break;
                if(remaining < BUF_SIZE) toRead = remaining;
            }

            bytes = read(fd, buf, toRead);	/* read from file */
            if (bytes <= 0) break;		/* check for end of file */
            write(sa, buf, bytes);		/* write bytes to socket */
            totalSent += bytes;
          }
        
          close(fd);			/* close file */
          if (debugFlag) printf("Finished sending %s to %s\n", fileName, client_IP);
        }

        close(sa);			/* close connection */
  }
}
