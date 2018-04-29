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

char *hst = NULL;

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
			printf("Only file modes unrestricted, exclusive, and transaction are supported.\n");
			return -1;
		}
		mode[1] = '\0';
		int fd = connectToServer(hst);
		if(fd < 0){
			perror("Error");
			return -1;
		}
		if(write(fd, mode, strlen(mode)) < 0)
			perror("Error");			
		if(recv(fd, response, 256, 0) < 0) 
			perror("Error");

		if(strcmp(response, "success") != 0) {
			perror("Error");
			return -1;
		}
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
	serverAddress.sin_port = htons(14314);
	if(connect(sock, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0){
		perror("Error");
		close(sock);
		return -1;
	}
	return sock;
}


//tells server to open file and reports file descriptor
int netopen(char *pathname, int flags) {
	if(hst == NULL){
		printf("Run netserverinit before opening.");
		return -1;
	}
	char *perm;
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
		default: {
			printf("O_RDONLY/O_WRONLY/O_RDWR only are supported.\n");
			return -1;	
		}
	}	
	int serverfd = connectToServer(hst);
	if(serverfd < 0){
		perror("Error");
		return -1;
	}
	int pnlen = strlen(pathname);
	char *pathlen = calloc(pnlen, sizeof(char));
	sprintf(pathlen, "%d", pnlen);
	int totallen = 2 + 2 + 1 + strlen(pathlen) + 1 + strlen(pathname);
	char msgrecv[256];
	char *msg = calloc(totallen+1, sizeof(char));
	strcat(msg, "o/");
	strcat(msg, perm);
	strcat(msg, "/");
	strcat(msg, pathlen);
	strcat(msg, "/");
	strcat(msg, pathname);
	free(pathlen);
	if(send(serverfd, msg, strlen(msg), 0) < 0) //Sends message to server
		perror("Error");
	free(msg);
	if(recv(serverfd, msgrecv, 255, 0) < 0) //Receives message from server
		perror("Error");
	msgrecv[255] = '\0';
	int i = 1;
	while(isdigit(msgrecv[i])){
		i++;
	}
	char * fdes = calloc(i+1, sizeof(char));
	memcpy(fdes, msgrecv, i);
	fdes[i] = '\0';
	int fd = atoi(fdes);
	free(fdes);
	if(fd == -1){
		i = 3;
		while(isdigit(msgrecv[i])){
			i++;
		}
		char * num = calloc(i - 3 + 1, sizeof(char));
		
		memcpy(num, &msgrecv[3], i - 3);
		num[i-3] = '\0';
		int length = atoi(num);
		free(num);
		switch(length){
			case 5:{
				char * err = calloc(6, sizeof(char));
				memcpy(err, &msgrecv[i+1], 5);
				err[5] = '\0';
				if(strcmp(err, "eintr") == 0){
					errno = EINTR;
				}
				else if(strcmp(err, "erofs") == 0){
					errno = EROFS;
				}
				break;
			}
			case 6:{ 
				char * err = calloc(7, sizeof(char));
				memcpy(err, &msgrecv[i+1], 6);
				err[6] = '\0';
				if(strcmp(err, "eacces") == 0){
					errno = EACCES;
				}
				else if(strcmp(err, "eisdir") == 0){
					errno = EISDIR;
				}
				else if(strcmp(err, "enoent") == 0){
					errno = ENOENT;
				}
				free(err);
				break;
			}
			default: {
				char * error = calloc(length+1, sizeof(char));
				memcpy(error, &msgrecv[i+1], length);
				error[length] = '\0';
				printf("Error: %s\n", error);
				free(error);
				close(serverfd);
				return fd;
			}
		}
	}
	if(fd == 0){
		//atoi didnt work...
		return -1;
	}
	close(serverfd);
	return fd;
}

//tells server to read file and reports number of bytes read
ssize_t netread(int fildes, void *buf, size_t nbyte) {
	if(fildes >= -1){
		errno = EBADF;
		return -1;
	}
	unsigned int size = (unsigned int) nbyte;
	int serverfd = connectToServer(hst); //connect to server
	char *msg = calloc(256, sizeof(char));
	char msgrecv[256];
	fildes = -1 * fildes;
	int fdlen = (int)floor(log10((double)fildes)) + 2;
	fildes = -1 * fildes;
	char *fdstr = calloc(fdlen+1, sizeof(char));
	sprintf(fdstr, "-%d", fildes * -1);
	int nbytelen = (int)floor(log10((double)size)) + 1;
	char *nbytestr = calloc(nbytelen+1, sizeof(char));
	sprintf(nbytestr, "%d", size);	
	strcat(msg, "r/");
	strcat(msg, fdstr);
	strcat(msg, "/");
	strcat(msg, nbytestr);
	free(fdstr);
	free(nbytestr);
	if(send(serverfd, msg, strlen(msg), 0) < 0) //send message to server
		perror("Error");
	free(msg);
	int bytesRead = 0, bytesToRead, isFirst = 1, bytes;
	char * data = calloc(nbyte, sizeof(char));
	while((bytes = recv(serverfd, msgrecv, 255, 0)) > 0){ //receive message
		msgrecv[bytes] = '\0';
		if(isFirst == 1 && strcmp(msgrecv, "-1/") == 0){
			errno = EBADF;
			free(data);
			return -1;
			break;
		}
		else if(strcmp(msgrecv, "000") == 0){
			if(isFirst == 1){
				free(data);
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
			char * len = calloc(i+1, sizeof(char));
			memcpy(len, msgrecv, i);
			len[i] = '\0';
			bytesToRead = atoi(len);
			free(len);
			memcpy(data, &msgrecv[i+1], bytes - (i + 1));
			bytesRead += bytes - (i+1);
			isFirst = 0;
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
	free(data);
	return bytesRead;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte) {
	if(fildes >= -1){
		errno = EBADF;
		return -1;
	}
	unsigned int size = (unsigned int) nbyte;	
	int serverfd = connectToServer(hst);
	fildes = -1 * fildes;
	int fdlen = (int)floor(log10((double)fildes)) + 2;
	fildes = -1 * fildes;
	char *fdstr = calloc(fdlen+1, sizeof(char));
	sprintf(fdstr, "-%d", fildes * -1);
	int nbytelen = (int)floor(log10((double)size)) + 1;
	char *nbytestr = calloc(nbytelen+1, sizeof(char));
	sprintf(nbytestr, "%d", size);	
	int totallen = 2 + strlen(fdstr) + 1 + strlen(nbytestr) + 1 + size;
	char * msg = calloc(totallen + 1, sizeof(char));
	char msgrecv[256];
	strcat(msg, "w/");
	strcat(msg, fdstr);
	strcat(msg, "/");
	strcat(msg,nbytestr);
	strcat(msg, "/");
	memcpy(msg + (2 + strlen(fdstr) + 1 + strlen(nbytestr) + 1), buf, size);
	free(fdstr);
	free(nbytestr);
	if(send(serverfd, msg, strlen(msg), 0) < 0)
		perror("Error");
	free(msg);
	if(recv(serverfd, msgrecv, 255, 0) < 0)
		perror("Error");
	msgrecv[255] = '\0';
		
	int byteswritten = atoi(msgrecv);
	if(byteswritten == -1){
		errno = EBADF;
	}
	return (ssize_t) byteswritten;
}

int netclose(int fd) {
	if(fd >= -1){
		errno = EBADF;
		return -1;
	}
	int output = -1;
	int serverfd = connectToServer(hst);
	fd = -1 * fd;
	int fdlen = (int)floor(log10((double)fd)) + 2;
	fd = -1 * fd;
	char *fdstr = calloc(fdlen+1, sizeof(char));
	sprintf(fdstr, "-%d", fd * -1);
	char * msg = calloc(2 + strlen(fdstr) + 2, sizeof(char));
	char msgrecv[256];
	strcat(msg, "c/");
	strcat(msg, fdstr);
	free(fdstr);
	if(send(serverfd, msg, strlen(msg), 0) < 0)
		perror("Error");
	free(msg);
	if(recv(serverfd, msgrecv, 255, 0) < 0)
		perror("Error");
	msgrecv[255] = '\0';
	if(strcmp(msgrecv, "zero") == 0){
		output = 0;
	}
	else{
		output = -1;
		errno = EBADF;
	}
	return output;
}

