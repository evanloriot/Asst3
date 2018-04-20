#include "libnetfiles.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

char *hst;
int globalfd;

//check if hostname is available
int netserverinit(char *hostname, int filemode) {
	hst = hostname;
	struct hostent *hstnm;
	hstnm = gethostbyname(hostname);
	if(hstnm != NULL) {
		char *mode = calloc(1, sizeof(char));
		char *response = calloc(256, sizeof(char));
		switch(filemode) {
			case UNRESTRICTED:
				mode = "u";
			break;
			case EXCLUSIVE:
				mode = "e";
			break;
			case TRANSACTION:
				mode = "t";
			break;
			default: mode = "u";
		}
		int fd = connectToServer(hst);
		if(fd < 0)
			printf("Error connecting to server\n");
		if(write(fd, mode, strlen(mode)) < 0)
			printf("Error writing to server\n");
		if(recv(fd, &response, 256, 0) < 0) 
			printf("Error receiving message\n");

		if(response[0] != 0) {
			printf("%s", response);
			return -1;
		}
		else printf("Successfully connected to server\n");
		return 0;
	}
	else {
		herror("gethostbyname");
		return -1;
	}
}

//makes a connection to the server and returns file descriptor
int connectToServer(char* hostname) {
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		perror("Error, could not open socket.\n");
		return -1;
	}

	struct hostent * server;
	server = gethostbyname(hostname);
	if(server == NULL) {
		printf("Error, could not get host server\n");
		return 1;
	}

	struct sockaddr_in serverAddress;
	bzero((char*) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serverAddress.sin_addr.s_addr, server->h_length);
	serverAddress.sin_port = htons(14315);
	if(connect(sock, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0){
		perror("Error, could not connect to server.\n");
		close(sock);
		return -1;
	}
	return sock;
}


//tells server to open file and reports file descriptor
int netopen(char *pathname, int flags) {
	char *perm = calloc(2, sizeof(char));
	switch(flags) {
		case O_RDONLY:
		perm = "ro";
		break;
		case O_WRONLY:
		perm = "wo";
		break;
		case O_RDWR:
		perm = "rw";
		break;
	}	
	int serverfd = connectToServer(hst);
	char *msgrecv = calloc(256, sizeof(char));
	char *msg = calloc(256, sizeof(char));
	int pnlen = strlen(pathname);
	char *pathlen = calloc(pnlen, sizeof(char));
	sprintf(pathlen, "%d", pnlen);
	strcat(msg, "o/");
	strcat(msg, perm);
	strcat(msg, "/");
	strcat(msg, pathlen);
	strcat(msg, "/");
	strcat(msg, pathname);
	if(send(serverfd, msg, strlen(msg), 0) < 0) //Sends message to server
		printf("Send failed\n");
	if(recv(serverfd, msgrecv, 256, 0) < 0) //Receives message from server
		printf("Receive failed\n");
	int fd = atoi(msgrecv);
	if(fd == -1)
		printf("File not found\n");
	globalfd = fd;
	close(serverfd);
	return fd;
}

//tells server to read file and reports number of bytes read
ssize_t netread(int fildes, void *buf, size_t nbyte) {
	unsigned int size = (unsigned int) nbyte;
	int serverfd = connectToServer(hst); //connect to server
	char *msg = calloc(256, sizeof(char));
	char *msgrecv = calloc(256, sizeof(char));
	globalfd = -1 * globalfd;
	int fdlen = (int)ceil(log10((double)globalfd)) + 1;
	globalfd = -1 * globalfd;
	char *fdstr = calloc(fdlen, sizeof(char));
	int nbytelen = (int)ceil(log10((double)size));
	char *nbytestr = calloc(nbytelen, sizeof(char));
	sprintf(nbytestr, "%d", size);	
	strcat(msg, "r/");
	strcat(msg, fdstr);
	strcat(msg, "/");
	strcat(msg, nbytestr);
	if(write(serverfd, msg, strlen(msg)) < 0) //send message to server
		printf("Send failed\n");
	while(recv(serverfd, &msgrecv, 256, 0) < 0) //receive message
		printf("Receive failed\n");
	strcat(buf, msgrecv);
	int bytesread = atoi(msgrecv);
	if(bytesread == -1)
		printf("Read error\n");
	return (ssize_t) bytesread;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte) {
	unsigned int size = (unsigned int) nbyte;	
	char *text = calloc(nbyte, sizeof(char));
	memcpy(text, buf, nbyte);
	int serverfd = connectToServer(hst);
	globalfd = -1 * globalfd;
	int fdlen = (int)ceil(log10((double)globalfd)) + 1;
	globalfd = -1 * globalfd;
	char *fdstr = calloc(fdlen, sizeof(char));
	int nbytelen = (int)ceil(log10((double)size));
	char *nbytestr = calloc(nbytelen, sizeof(char));
	sprintf(nbytestr, "%d", size);	
	char msg[256];
	char msgrecv[256];
	strcat(msg, "w/");
	strcat(msg, fdstr);
	strcat(msg, "/");
	strcat(msg, nbytestr);
	strcat(msg, "/");
	strcat(msg, text);
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
	globalfd = -1 * globalfd;
	int fdlen = (int)ceil(log10((double)globalfd)) + 1;
	globalfd = -1 * globalfd;
	char *fdstr = calloc(fdlen, sizeof(char));
	char  msg[256];
	char msgrecv[256];
	strcat(msg, "c/");
	strcat(msg, fdstr);
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

