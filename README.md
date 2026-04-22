[project-file-server.pdf](https://github.com/user-attachments/files/26826603/project-file-server.pdf)


Description: TCP-Based Client-Server Communication with dual-direction read and writing.

Instructions to run:
1) Run [make] to compile the client and server. It should show up in the current directory.
2) If the hostname of the server is not know - the following command can be used to determine the hostname:
	- "hostname"

3) Start server first - there are two options:
	- ./server		>> This will run the server but now activity displays
	- ./server DEBUG=1 	>> This runs the server while also displaying messages of its activities

4) Start client next - there are 3 options:
	- ./client '<server-name>' <file-name>	  >> This is the base format, it will find the file in the server and receive it
	- ./client '<server-name>' -w '<file-name>'   >> This starts write-mode, allowing the user to upload files to the server
	- ./client '<server-name> <file-name>' -s '<starting value>' -e '<final byte>' 
		>> This will allow the user to request a specific section of a desired file. It can be write -e first follow by -s, but being consistent with the pre-determined format is recommended.

NOTE(s):
- DEBUG does not display the progress of the server's progress but can be done given the current structure of the byte range.
- The PORT read by both ends is hardcoded as 23455 but can be changed via the fileheader.h
- Arbitrary PORTS can be used or even randomized too; however, it was not implemented.
- Error detection is present but full error detection cannot be guaranteed. Edge cases mayu exist.
