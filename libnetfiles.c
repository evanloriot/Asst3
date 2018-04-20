#include "libnetfiles.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#define READ_ONLY 0
char *hst;

//check if hostname is available
int netserverinit(char *hostname) {
	hst = hostname;
	struct hostent *hstnm;
	hstnm = gethostbyname(hostname);
	if(hstnm != NULL)
		return 0;
	else {
		herror("gethostbyname");
		return -1;
	}
}

//makes a connection to the server and returns file descriptor
int connectToServer(char* hostname) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		perror("Error, could not open socket.");
		return -1;
	}

	struct hostent * server;
	server = gethostbyname(hostname);
	if(server == NULL) {
		printf("Error, could not get host server");
		return 1;
	}

	struct sockaddr_in serverAddress;
	bzero((char*) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serverAddress.sin_addr.s_addr, server->h_length);
	serverAddress.sin_port = htons(14315);
	if(connect(sock, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0){
		perror("Error, could not connect to server.");
		close(sock);
		return -1;
	}
	return sock;
}


//tells server to open file and reports file descriptor
int netopen(char *pathname, int flags) {
	int serverfd = connectToServer(hst);
	char *msgrecv;
	char msg[256];
	strcpy(msg, "open ");
	strcpy(msg, pathname);
	strcpy(msg, " ");
	if(flags == 0); //Read only
		strcpy(msg, "ro ");
	if(send(serverfd, msg, 256, strlen(msg)) < 0) //Sends message to server
		printf("Send failed\n");
	if(recv(serverfd, &msgrecv, 256, 0) < 0) //Receives message from server
		printf("Receive failed\n");

	int fd = atoi(msgrecv);
	if(fd == -1)
		printf("File not found\n");
	return fd;
}

//tells server to read file and reports number of bytes read
ssize_t netread(int fildes, void *buf, size_t nbyte) {
	unsigned int size = (unsigned int) nbyte;
	int serverfd = connectToServer(hst); //connect to server
	char msg[256];
	char msgrecv[256];
	strcpy(msg, "read ");
	sprintf(msg, "%d", size);
	strcpy(msg, " ");
	if(send(serverfd, msg, 256, strlen(msg)) < 0) //send message to server
		printf("Send failed\n");
	if(recv(serverfd, &msgrecv, 256, 0) < 0) //receive message
		printf("Receive failed\n");
	strcpy(buf, msgrecv);
	int bytesread = atoi(msgrecv);
	if(bytesread == -1)
		printf("Read error\n");
	return (ssize_t) bytesread;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte) {
	int serverfd = connectToServer(hst);
	char msg[256];
	char msgrecv[256];
	strcpy(msg, "write ");
	strcpy(msg, buf);
	strcpy(msg, " ");
	sprintf(msg, "%d", (unsigned int)nbyte);
	if(send(serverfd, msg, 256, strlen(msg)) < 0)
		printf("Send failed\n");
	if(recv(serverfd, &msgrecv, 256, 0) < 0)
		printf("Receive failed\n");
	int byteswritten = atoi(msgrecv);
	if(byteswritten == -1)
		printf("Write error\n");
	return (ssize_t) byteswritten;
}

int netclose(int fd) {
	int serverfd = connectToServer(hst);
	char  msg[256];
	char msgrecv[256];
	strcpy(msg, "close ");
	sprintf(msg, "%d", fd);
	if(send(serverfd, msg, 256, strlen(msg)) < 0)
		printf("Send failed\n");
	if(recv(serverfd, msgrecv, 256, 0) < 0)
		printf("Receive failed\n");
	int result = atoi(msgrecv);
	if(result == 0)
		printf("Close success\n");
	else printf("Close failed\n");
	return result;
}
	















