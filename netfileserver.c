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
#include <ctype.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <math.h>

void * connection_handler(void*);
void * accept_clients();

typedef struct _file {
	int fd;
	struct _file * next;
} file;

typedef struct _client {
	char * ip;
	file * files;
	struct _client * next;
} client;

client * clients;

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

	struct sockaddr_in addr;
    	socklen_t addr_size = sizeof(struct sockaddr_in);
    	int res = getpeername(socket, (struct sockaddr *)&addr, &addr_size);
	if(res == -1){
		perror("Error, unable to determine IP address of client.");
		pthread_exit(NULL);
	}
    	char clientip[20];
    	strcpy(clientip, inet_ntoa(addr.sin_addr));

	int bytes;
	char buffer[256];
	bzero(&buffer, 256);
	int isFirst = 1;
        char command;
        char flags[3];
        char * data = NULL;
	int datasize;
	int r = 0;
	int doBreak = 0;
	int fileDescriptor = -1;
        while((bytes = recv(socket, buffer, 255, 0)) > 0){
		buffer[255] = '\0';
                if(isFirst == 1){
                        switch(buffer[0]){
                                case 'o':
                                        command = buffer[0];
                                        flags[0] = buffer[2];
                                        flags[1] = buffer[3];
                                        flags[2] = '\0';

                                        int i = 5;
                                        while(isdigit(buffer[i])){
                                                i++;
                                        }
                                        char * num = calloc((i-5) + 1, sizeof(char));
                                        memcpy(num, &buffer[5], i-5);
					num[i-1] = '\0';
                                        datasize = atoi(num);
                                        free(num);

                                        data = calloc(datasize, sizeof(char));
                                        sprintf(data, "%s", &buffer[i+1]);
					
					r = strlen(&buffer[i+1]);

                                        isFirst = 0;
                                        break;
				case 'r':
					command = buffer[0];

					i = 3;
					while(isdigit(buffer[i])){
						i++;
					}
					char * fd = calloc((i - 3) + 1, sizeof(char));
					memcpy(fd, &buffer[3], i-3);
					fd[i-1] = '\0';
					fileDescriptor = atoi(fd);
					fileDescriptor--;
					free(fd);

					int c = i+1;
					while(isdigit(buffer[c])){
						c++;
					}
					char * n = calloc((c - (i + 1) + 1), sizeof(char));
					memcpy(n, &buffer[i + 1], c - (i + 1));
					n[c-1] = '\0';
					datasize = atoi(n);
					free(n);
					doBreak = 1;
					break;
				default:
					break;
                        }
                }
		else{
			if(datasize - bytes < 256){
				buffer[datasize - bytes] = '\0';
			}
			sprintf(data + bytes, "%s", buffer);
			r += bytes;
		}
		if(r == datasize || doBreak == 1){
			break;
		}
        }
	switch(command){
		case 'o':{
			client * c = clients;
			while(c != NULL){
				if(strcmp(c->ip, clientip) == 0){
					break;
				}
				c = c->next;
			}
			if(c == NULL){
				c = malloc(sizeof(client));
				c->ip = calloc(strlen(clientip), sizeof(char));
				sprintf(c->ip, "%s", clientip);
				c->next = clients;
				c->files = NULL;
				clients = c;
			}
			int fd;
			if(strcmp(flags, "rw") == 0){
				fd = open(data, O_RDWR);
			}
			else if(strcmp(flags, "wo") == 0){
				fd = open(data, O_WRONLY);
			}
			else{
				fd = open(data, O_RDONLY);
			}
			if(fd == -1){
				switch(errno){
					case EACCES:
						send(socket, "-1/6/eacces", 11, 0);
						break;
					case EINTR:
						send(socket, "-1/5/eintr", 10, 0);
						break;
					case EISDIR:
						send(socket, "-1/6/eisdir", 11, 0);
						break;
					case ENOENT:
						send(socket, "-1/6/enoent", 11, 0);
						break;
					case EROFS:
						send(socket, "-1/5/erofs", 10, 0);
						break;
					default: perror("Unexpected error occurred\n"); pthread_exit(NULL); break;
				}
				break;
			}

			//consider handling fd already open...
			file * f = malloc(sizeof(file));
			f->fd = fd;
			f->next = c->files;
			c->files = f;
			
			fd = fd + 1;

			char * s = calloc((int)floor(log10((double)fd))+3, sizeof(char));
			sprintf(s, "-%d\n", fd);
			if(send(socket, s, strlen(s), 0) < 0){
				perror("Error, unable to send file descriptor to client.");
				pthread_exit(NULL);
			}
			break;
		}
		case 'r':{
			client * c = clients;
			while(c != NULL){
				if(strcmp(c->ip, clientip) == 0){
					break;
				}
				c = c->next;
			}
			if(c == NULL){
				//something bad is about to happen...
				if(send(socket, "-1/", 3, 0) < 0){
					perror("Error, response did not send");
				}
				break;
			}
			file * f = c->files;
			while(f != NULL){
				if(f->fd == fileDescriptor){
					break;
				}
				f = f->next;
			}
			if(f == NULL){
				//again... something bad is about to happen...
				if(send(socket, "-1/", 3, 0) < 0){
					perror("Error, response did not send");
				}
				break;
			}
			char * d = calloc(datasize+1, sizeof(char));
			int rd = read(fileDescriptor, d, datasize);
			if(rd == 0){
				if(send(socket, "000", 3, 0) < 0){
					perror("Error");
				}
				free(d);
				close(socket);
				pthread_exit(NULL);
			}
			if(rd < 0){
				//no read
				if(errno == EBADF){
					if(send(socket, "-1/", 3, 0) < 0){
						perror("Error, response did not send");
					}
				}
				else{
					perror("Error");
				}
				free(d);
				close(socket);
				pthread_exit(NULL);
			}
			int length = (int)floor(log10((double)rd)) + 1;
			char * dd = calloc(datasize + length + 2, sizeof(char));
			sprintf(dd, "%d/", rd);
			sprintf(dd + length + 1, "%s", d);

			if(send(socket, dd, datasize + length + 1, 0) < 0){
				perror("Error, response did not send");
				free(d);
				close(socket);
				pthread_exit(NULL);
			}
			free(d);
			break;
		}
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
	while((client = accept(sock, (struct sockaddr*) &clientSocket, (socklen_t*) &client_address_length)) != -1){
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

