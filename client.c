#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

int main(){
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		perror("Error, could not open socket.");
		exit(-1);
	}
	
	struct hostent * server;
	server = gethostbyname("ls.cs.rutgers.edu");
	if(server == NULL){
		printf("Error, could not get host server.\n");
		exit(-1);
	}
	
	struct sockaddr_in serverAddress;
	bzero((char*) &serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serverAddress.sin_addr.s_addr, server->h_length);
	serverAddress.sin_port = htons(14315);
	
	if(connect(sock, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0){
		perror("Error, could not connect to server.");
		close(sock);
		exit(-1);
	}

	char buffer[256];
	fgets(buffer, 256, stdin);
	int bytes;
	bytes = write(sock, buffer, strlen(buffer));
	close(sock);
	return 0;
}
