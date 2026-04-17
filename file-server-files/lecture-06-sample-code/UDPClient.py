from socket import *
serverName = 'monroe.cs.ucf.edu'
serverPort = 9119
clientSocket = socket(AF_INET, SOCK_DGRAM)
message = input('Input a sentence: ')
clientSocket.sendto(message.encode(), (serverName, serverPort))
modifiedMessage, serverAddress = clientSocket.recvfrom(2048)
print(modifiedMessage.decode())
clientSocket.close()
