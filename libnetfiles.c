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
#include <ctype.h>
#include <errno.h>

char *hst;
int globalfd;

//check if hostname is available
int netserverinit(char *hostname, char * filemode) {
	hst = hostname;
	struct hostent *hstnm;
	hstnm = gethostbyname(hostname);
	if(hstnm != NULL) {
		char mode[2];
		char response[256];
		if(strcmp(filemode, "unrestricted") == 0) {
			mode[0] = 'u';
		}	
		else if(strcmp(filemode, "exclusive") == 0){
			mode[0] = 'e';
		}
		else if(strcmp(filemode, "transaction") == 0){
			mode[0] = 't';
		}
		else{
			mode[0] = 'u';
		}
		mode[1] = '\0';
		int fd = connectToServer(hst);
		if(fd < 0)
			printf("Error connecting to server\n");
		if(write(fd, mode, strlen(mode)) < 0)
			printf("Error writing to server\n");
		if(recv(fd, response, 256, 0) < 0) 
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
	char *msg = calloc(256, sizeof(char));
	int pnlen = strlen(pathname);
	char *pathlen = calloc(pnlen, sizeof(char));
	sprintf(pathlen, "%d", pnlen);
	int totallen = 2 + 2 + 1 + strlen(pathlen) + 1 + strlen(pathname);
	char *msgrecv = calloc(totallen, sizeof(char));
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
	int i = 1;
	while(isdigit(msgrecv[i])){
		i++;
	}
	char * fdes = calloc(i+1, sizeof(char));
	memcpy(fdes, &msgrecv[0], i);
	fdes[i] = '\0';
	int fd = atoi(fdes);
	if(fd == -1){
		int length = msgrecv[i+1] - '0';
		switch(length){
			case 5:
				if(strcmp(&msgrecv[i+3], "eintr") == 0){
					errno = EINTR;
				}
				else if(strcmp(&msgrecv[i+3], "erofs") == 0){
					errno = EROFS;
				}
				break;
			case 6: 
				if(strcmp(&msgrecv[i+3], "eacces") == 0){
					errno = EACCES;
				}
				else if(strcmp(&msgrecv[i+3], "eisdir") == 0){
					errno = EISDIR;
				}
				else if(strcmp(&msgrecv[i+3], "enoent") == 0){
					errno = ENOENT;
				}
				break;
			default: break;
		}
		perror("Error, file not found");
	}
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
	fildes = -1 * fildes;
	int fdlen = (int)ceil(log10((double)fildes)) + 1;
	fildes = -1 * fildes;
	char *fdstr = calloc(fdlen, sizeof(char));
	sprintf(fdstr, "-%d", fildes * -1);
	int nbytelen = (int)ceil(log10((double)size));
	char *nbytestr = calloc(nbytelen, sizeof(char));
	sprintf(nbytestr, "%d", size);	
	strcat(msg, "r/");
	strcat(msg, fdstr);
	strcat(msg, "/");
	strcat(msg, nbytestr);
	if(send(serverfd, msg, strlen(msg), 0) < 0) //send message to server
		printf("Send failed\n");
	int bytesRead, bytesToRead, isFirst = 1, bytes;
	char * data = calloc(nbyte, sizeof(char));
	while((bytes = recv(serverfd, msgrecv, 255, 0)) > 0){ //receive message
		msgrecv[255] = '\0';
		if(isFirst == 1 && strcmp(msgrecv, "-1/") == 0){
			return -1;
			break;
		}
		if(isFirst == 1){
			int i = 0;
			while(isdigit(msgrecv[i])){
				i++;
			}
			char * len = calloc(i+2, sizeof(char));
			memcpy(len, msgrecv, i+1);
			len[i+1] = '\0';
			bytesToRead = atoi(len);
			free(len);
			memcpy(data, &msgrecv[i+1], bytes - (i+1));
			bytesRead += bytes - (i+1);
		}
		else{
			memcpy(&data[bytesRead], msgrecv, bytes);
			bytesRead += bytes;
		}
		if(bytesRead == bytesToRead){
			break;
		}
		
	}
	memcpy(buf, data, nbyte);
	return bytesRead;
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

