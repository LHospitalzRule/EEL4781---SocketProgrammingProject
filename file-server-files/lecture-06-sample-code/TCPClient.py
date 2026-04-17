from socket import *
serverName = 'CECSHNJ9PQV56R'
serverPort = 6789
clientSocket = socket(AF_INET, SOCK_STREAM)
clientSocket.connect((serverName,serverPort))
sentence = input('Input a sentence: ')
sentence = sentence + '\n';
clientSocket.send(sentence.encode())
receivedData = clientSocket.recv(1024)
modifiedSentence = receivedData
print ('From server:', modifiedSentence.decode())
clientSocket.close()
