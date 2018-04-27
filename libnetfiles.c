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
			perror("Error");
		if(write(fd, mode, strlen(mode)) < 0)
			perror("Error");			
		if(recv(fd, response, 256, 0) < 0) 
			perror("Error");

		if(response[0] != 0) {
			//this may not work... fix this
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
	int pnlen = strlen(pathname);
	char *pathlen = calloc(pnlen, sizeof(char));
	sprintf(pathlen, "%d", pnlen);
	int totallen = 2 + 2 + 1 + strlen(pathlen) + 1 + strlen(pathname);
	char *msgrecv = calloc(totallen, sizeof(char));
	char *msg = calloc(totallen, sizeof(char));
	strcat(msg, "o/");
	strcat(msg, perm);
	strcat(msg, "/");
	strcat(msg, pathlen);
	strcat(msg, "/");
	strcat(msg, pathname);
	if(send(serverfd, msg, strlen(msg), 0) < 0) //Sends message to server
		perror("Error");
	if(recv(serverfd, msgrecv, 256, 0) < 0) //Receives message from server
		perror("Error");
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
	int fdlen = (int)floor(log10((double)fildes)) + 2;
	fildes = -1 * fildes;
	char *fdstr = calloc(fdlen, sizeof(char));
	sprintf(fdstr, "-%d", fildes * -1);
	int nbytelen = (int)floor(log10((double)size)) + 1;
	char *nbytestr = calloc(nbytelen, sizeof(char));
	sprintf(nbytestr, "%d", size);	
	strcat(msg, "r/");
	strcat(msg, fdstr);
	strcat(msg, "/");
	strcat(msg, nbytestr);
	if(send(serverfd, msg, strlen(msg), 0) < 0) //send message to server
		perror("Error");
	int bytesRead, bytesToRead, isFirst = 1, bytes;
	char * data = calloc(nbyte, sizeof(char));
	while((bytes = recv(serverfd, msgrecv, 255, 0)) > 0){ //receive message
		msgrecv[255] = '\0';
		if(isFirst == 1 && strcmp(msgrecv, "-1/") == 0){
			errno = EBADF;
			return -1;
			break;
		}
		else if(strcmp(msgrecv, "000") == 0){
			if(isFirst == 1){
				return 0;
			}
			else{
				break;
			}
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
		if(bytesRead >= bytesToRead){
			break;
		}
		
	}
	if(bytes < 0){
		return -1;
	}
	memcpy(buf, data, nbyte);
	return bytesRead;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte) {
	if(fildes == 0){
		printf("File descriptor cannot be zero.");
		return -1;
	}
	unsigned int size = (unsigned int) nbyte;	
	int serverfd = connectToServer(hst);
	fildes = -1 * fildes;
	int fdlen = (int)floor(log10((double)fildes)) + 2;
	fildes = -1 * fildes;
	char *fdstr = calloc(fdlen, sizeof(char));
	sprintf(fdstr, "-%d", fildes * -1);
	int nbytelen = (int)floor(log10((double)size)) + 1;
	char *nbytestr = calloc(nbytelen, sizeof(char));
	sprintf(nbytestr, "%d", size);	
	int totallen = 2 + strlen(fdstr) + 1 + strlen(nbytestr) + 1 + size;
	char * msg = calloc(totallen + 1, sizeof(char));
	char * msgrecv = calloc(totallen + 1, sizeof(char));
	strcat(msg, "w/");
	strcat(msg, fdstr);
	strcat(msg, "/");
	strcat(msg,nbytestr);
	strcat(msg, "/");
	memcpy(msg + (2 + strlen(fdstr) + 1 + strlen(nbytestr) + 1), buf, size);
	if(send(serverfd, msg, strlen(msg), 0) < 0)
		perror("Error");
	if(recv(serverfd, msgrecv, 256, 0) < 0)
		perror("Error");
		
	int byteswritten = atoi(msgrecv);
	return (ssize_t) byteswritten;
}

int netclose(int fd) {
	int output = -1;
	int serverfd = connectToServer(hst);
	fd = -1 * fd;
	int fdlen = (int)floor(log10((double)fd)) + 2;
	fd = -1 * fd;
	char *fdstr = calloc(fdlen, sizeof(char));
	sprintf(fdstr, "-%d", fd * -1);
	char * msg = calloc(2 + strlen(fdstr) + 1, sizeof(char));
	char * msgrecv = calloc(256, sizeof(char));
	strcat(msg, "c/");
	strcat(msg, fdstr);
	if(send(serverfd, msg, 256, strlen(msg)) < 0)
		perror("Error");
	if(recv(serverfd, msgrecv, 256, 0) < 0)
		perror("Error");
	if(strcmp(msgrecv, "zero") == 0){
		output = 0;
	}
	else{
		output = -1;
		errno = EBADF;
	}
	return output;
}

