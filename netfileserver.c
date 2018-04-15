#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

void * connection_handler(void*);
void * accept_clients();

int main(){
	pthread_t server_thread;

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	if(pthread_sigmask(SIG_BLOCK, &set, NULL) < 0){
		perror("Error, unable to mask SIGPIPE.");
		exit(-1);
	}
	
	if(pthread_create(&server_thread, NULL, &accept_clients, NULL) < 0){
		perror("Error, could not create server thread.");
		exit(-1);
	}

	pthread_join(server_thread, NULL);

	pthread_exit(NULL);
}

void * connection_handler(void * sock){
	int socket = *(int*) sock;
	int bytes;
	char buffer[256];
	while((bytes = read(socket, buffer, 255)) > 0){
		buffer[bytes] = '\0';
		//do handling
	}
	close(socket);
	pthread_exit(NULL);
}

void * accept_clients(){
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		perror("Error, unable to open socket.");
		pthread_exit(NULL);
	}

	int yes = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("Error, unable to set socket options.");
		close(sock);
		pthread_exit(NULL);
	}
	
	struct sockaddr_in server;
	bzero((char*) &server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(14315);
	
	if(bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0){
		perror("Error, unable to bind socket to address.");
		close(sock);
		pthread_exit(NULL);
	}

	if(listen(sock, 5) == -1){
		perror("Error, could not listen on socket.");
		close(sock);
		pthread_exit(NULL);
	}

	struct sockaddr_in clientSocket;
	int client_address_length = sizeof(clientSocket);

	pthread_t thread;
	
	int client;
	while( (client = accept(sock, (struct sockaddr*) &clientSocket, (socklen_t*) &client_address_length)) ){
		if(pthread_create(&thread, NULL, connection_handler, (void*) &client) != 0){
			perror("Error, could not create thread.");
			close(client);
			close(sock);
			pthread_exit(NULL);
		}
	}

	close(sock);
	pthread_exit(NULL);
}

