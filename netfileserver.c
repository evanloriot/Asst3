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
#include <semaphore.h>
#include <time.h>

void * connection_handler(void*);
void * accept_clients();
void * handle_queue();

pthread_mutex_t lock;

typedef struct _file {
	int fd;
	struct _file * next;
} file;

typedef struct _client {
	char * ip;
	file * files;
	struct _client * next;
} client;

typedef struct _accessType {
	int fd;
	char mode;
	int isWrite;
	struct _accessType * next;
} accessType;

typedef struct _commandNode{
	char mode;
	int isWrite;
	time_t * timer;
	int isValid;
	struct _commandNode * next;
} commandNode;

typedef struct _fileParam {
	sem_t * binarySem;
	char * filePath;
	accessType * modes;
	commandNode * queue;
	struct _fileParam * next;
} fileParam;

typedef struct _clientAccessParam {
	char * ip;
	char param;
	struct _clientAccessParam * next;
} clientAccessParam;

client * clients = NULL;
fileParam * filesOpen = NULL;
clientAccessParam * cParams = NULL;

int main(){
	pthread_t server_thread;
	pthread_t qWatch;

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

	if(pthread_create(&qWatch, NULL, &handle_queue, NULL) < 0){
		perror("Error, could not create watcher thread.");
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
					if(255 - (i+1) < datasize){
						memcpy(data, &buffer[i+1], 255 - (i+1));
					}
					else{
						memcpy(data, &buffer[i+1], datasize);
					}
					
					if(datasize > 255 - (i+1)){
						r = 255-(i+1);
					}
					else{
						r = datasize;
					}

                                        isFirst = 0;
                                        break;
				case 'r':{
					command = buffer[0];

					int i = 3;
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
				}
				case 'w':{
					command = buffer[0];
					
					int i = 3;
					while(isdigit(buffer[i])){
						i++;
					}
					char * fd = calloc((i-3) + 1, sizeof(char));
					memcpy(fd, &buffer[3], i-3);
					fd[i-3] = '\0';
					fileDescriptor = atoi(fd);
					fileDescriptor--;
					free(fd);

					int j = i+1;
					while(isdigit(buffer[j])){
						j++;
					}
					char * num = calloc(j - (i+1) + 1, sizeof(char));
					memcpy(num, &buffer[i+1], j - i);
					num[j-1] = '\0';
					datasize = atoi(num);
					free(num);

					data = calloc(datasize, sizeof(char));
					if(datasize > 255 - (j+1)){
						memcpy(data, &buffer[j+1], 255 - (j + 1));
						r = 255 - (j+1);
					}
					else{
						memcpy(data, &buffer[j+1], datasize);
						r = datasize;
					}
					isFirst = 0;
					break;
				}
				case 'c':{
					command = 'c';

					fileDescriptor = atoi(&buffer[3]);
					fileDescriptor--;
					
					doBreak = 1;
					break;
				}
				case 'u':{
					command = 'u';
					break;
				}
				case 'e':{
					command = 'e';
					break;
				}
				case 't':{
					command = 't';
					break;
				}
				default:
					break;
                        }
                }
		else{
			if(255 + r < datasize){
				memcpy(data + r, buffer, 255);
				r += 255;
			}
			else{
				memcpy(data + r, buffer, datasize - r);
				r = datasize;
			}
		}
		if(r >= datasize || doBreak == 1){
			break;
		}
        }
	switch(command){
		case 'o':{
			fileParam * fo; 
			clientAccessParam * p;
			do{
			p = cParams;
			while(p != NULL){
				if(strcmp(p->ip, clientip) == 0){
					break;
				}
				p = p->next;
			}
			if(p == NULL){
				//something fucked
				close(socket);
				pthread_exit(NULL);
			}
			fo = filesOpen;
			while(fo != NULL){
				if(strcmp(fo->filePath, data) == 0){
					break;
				}
				fo = fo->next;
			}
			if(fo != NULL && p->param == 't'){
				pthread_mutex_lock(&lock);
				commandNode * com = malloc(sizeof(commandNode));
				com->mode = p->param;
				if(strcmp(flags, "rw") == 0 || strcmp(flags, "wo") == 0){
					com->isWrite = 1;
				}
				else{
					com->isWrite = 0;
				}
				com->next = fo->queue;
				com->isValid = 1;
				com->timer = malloc(sizeof(time_t));
				time(com->timer);
				fo->queue = com;
				pthread_mutex_unlock(&lock);

				sem_wait(fo->binarySem);
				if(com->isValid == 0){
					if(send(socket, "-1/7/timeout", strlen("-1/7/timeout"), 0) < 0){
						perror("Error");
					}
					pthread_exit(NULL);
				}
				continue;
				//send(socket, "-1/38/File already open in Transaction mode.", 44, 0);
				break;
			}
			if(fo != NULL){
				if(p->param == 'e' && (strcmp(flags, "rw") == 0 || strcmp(flags, "wo") == 0)){
					accessType * modes = fo->modes;
					while(modes != NULL){
						if(modes->isWrite == 1){
							break;	
						}
						modes = modes->next;
					}
					if(modes != NULL){
						pthread_mutex_lock(&lock);
						commandNode * com = malloc(sizeof(commandNode));
						com->mode = p->param;
						if(strcmp(flags, "rw") == 0 || strcmp(flags, "wo") == 0){
							com->isWrite = 1;
						}
						else{
							com->isWrite = 0;
						}
						com->next = fo->queue;
						com->isValid = 1;
						com->timer = malloc(sizeof(time_t));
						time(com->timer);
						fo->queue = com;
						pthread_mutex_unlock(&lock);
						sem_wait(fo->binarySem);
						if(com->isValid == 0){
							if(send(socket, "-1/7/timeout", strlen("-1/7/timeout"), 0) < 0){
								perror("Error");
							}
							pthread_exit(NULL);
						}
						continue;
						//send(socket, "-1/32/File already open in write mode.", 38, 0);
						//pthread_exit(NULL);
					}
				}
				else if(p->param == 'u'){
					accessType * modes = fo->modes;
					while(modes != NULL){
						if(modes->mode == 'e' && modes->isWrite == 1){
							pthread_mutex_lock(&lock);
							commandNode * com = malloc(sizeof(commandNode));
							com->mode = p->param;
							if(strcmp(flags, "rw") == 0 || strcmp(flags, "wo") == 0){
								com->isWrite = 1;
							}
							else{
								com->isWrite = 0;
							}
							com->next = fo->queue;
							com->isValid = 1;
							com->timer = malloc(sizeof(time_t));
							time(com->timer);
							fo->queue = com;
							pthread_mutex_unlock(&lock);
							sem_wait(fo->binarySem);
							if(com->isValid == 0){
								if(send(socket, "-1/7/timeout", strlen("-1/7/timeout"), 0) < 0){
									perror("Error");
								}
								pthread_exit(NULL);
							}
							continue;
							//char * msg = "-1/54/File already open in write mode with Exclusive access.";
							//send(socket, msg, strlen(msg), 0);
							//break;
						}
						else if(modes->mode == 't'){
							pthread_mutex_lock(&lock);
							commandNode * com = malloc(sizeof(commandNode));
							com->mode = p->param;
							if(strcmp(flags, "rw") == 0 || strcmp(flags, "wo") == 0){
								com->isWrite = 1;
							}
							else{
								com->isWrite = 0;
							}
							com->next = fo->queue;
							com->isValid = 1;
							com->timer = malloc(sizeof(time_t));
							time(com->timer);
							fo->queue = com;
							pthread_mutex_unlock(&lock);
							sem_wait(fo->binarySem);
							if(com->isValid == 0){
								if(send(socket, "-1/7/timeout", strlen("-1/7/timeout"), 0) < 0){
									perror("Error");
								}
								pthread_exit(NULL);
							}
							continue;
							//send(socket, "-1/38/File already open in Transaction mode.", 44, 0);
							//break;
						}
						modes = modes->next;
					}
					if(modes != NULL){
						pthread_exit(NULL);
					}
				}
				accessType * mode = malloc(sizeof(accessType));
				mode->mode = p->param;
				if(strcmp(flags, "rw") == 0 || strcmp(flags, "wo") == 0){
					mode->isWrite = 1;
				}
				else{
					mode->isWrite = 0;
				}
				mode->next = fo->modes;
				fo->modes = mode;
			}
			break;
			}
			while(1);

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
				strcpy(c->ip, clientip);
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
			if(fo == NULL){
				pthread_mutex_lock(&lock);
				fo = malloc(sizeof(fileParam));
				fo->binarySem = malloc(sizeof(sem_t));
				sem_init(fo->binarySem, 0, 0);
				fo->filePath = calloc(strlen(data)+1, sizeof(char));
				strcat(fo->filePath, data);
				fo->filePath[strlen(data)] = '\0';
				fo->modes = malloc(sizeof(accessType));
				fo->modes->mode = p->param;
				if(strcmp(flags, "rw") == 0 || strcmp(flags, "wo") == 0){
					fo->modes->isWrite = 1;
				}
				else{
					fo->modes->isWrite = 0;
				}
				fo->modes->next = NULL;
				fo->modes->fd = fd;
				fo->queue = NULL;
				fo->next = filesOpen;
				filesOpen = fo;
				pthread_mutex_unlock(&lock);
			}
			else{
				pthread_mutex_lock(&lock);
				accessType * m = malloc(sizeof(accessType));
				m->fd = fd;
				m->mode = p->param;
				m->next = fo->modes;
				fo->modes = m;
				pthread_mutex_unlock(&lock);
			}
			
			file * f = malloc(sizeof(file));
			f->fd = fd;
			f->next = c->files;
			c->files = f;
			
			fd = fd + 1;

			char * s = calloc((int)floor(log10((double)fd))+3, sizeof(char));
			sprintf(s, "-%d\n", fd);
			if(send(socket, s, strlen(s), 0) < 0){
				perror("Error, unable to send file descriptor to client.");
				free(s);
				pthread_exit(NULL);
			}
			free(s);
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
			free(dd);
			free(d);
			break;
		}
		case 'w':{
			client * c = clients;
			while(c != NULL){
				if(strcmp(c->ip, clientip) == 0){
					break;
				}
				c = c->next;
			}
			if(c == NULL){
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
				if(send(socket, "-1/", 3, 0) < 0){
					perror("Error, response did not send");
				}
				break;
			}
			int wr = write(fileDescriptor, data, datasize);
			if(wr < 0){
				if(errno == EBADF){
					if(send(socket, "-1/", 3, 0) < 0){
						perror("Error, response did not send");
					}
					break;
				}
				else{
					perror("Unexpected Error");
					close(socket);
					if(data != NULL) free(data);
					pthread_exit(NULL);
				}
			}
			int length = (int)floor(log10((double)wr)) + 1;
			char * m = calloc(length+1, sizeof(char));
			sprintf(m, "%d", wr);
			if(send(socket, m, length, 0) < 0){
				perror("Error");
				free(m);
				close(socket);
				if(data != NULL) free(data);
				pthread_exit(NULL);
			}
			free(m);
			break;
		}
		case 'c':{
			client * prevClient = NULL;
			client * c = clients;
			while(c != NULL){
				if(strcmp(c->ip, clientip) == 0){
					break;
				}
				prevClient = c;
				c = c->next;
			}
			if(c == NULL){
				if(send(socket, "fail", 4, 0) < 0){
					perror("Error");
				}
				break;
			}
			file * prevFile = NULL;
			file * f = c->files;
			while(f != NULL){
				if(f->fd == fileDescriptor){
					break;
				}
				prevFile = f;
				f = f->next;
			}
			if(f == NULL){
				if(send(socket, "fail", 4, 0) < 0){
					perror("Error");
				}
				break;
			}
			
			int result = close(fileDescriptor);
			if(result == 0){
				//update filesOpen
				pthread_mutex_lock(&lock);
				fileParam * fo = filesOpen;
				fileParam * prevfo = NULL;
				while(fo != NULL){
					int doBreak = 0;
					accessType * p = fo->modes;
					accessType * pprev = NULL;
					while(p != NULL){
						if(p->fd == fileDescriptor){
							if(pprev == NULL){
								fo->modes = p->next;
							}
							else{
								pprev = p->next;
							}
							free(p);
							doBreak = 1;
							break;
						}
						pprev = p;
						p = p->next;
					}
					if(doBreak == 1){
						break;
					}
					prevfo = fo;
					fo = fo->next;
				}
				sem_post(fo->binarySem);
				if(fo->modes == NULL){
					if(fo->queue == NULL){
						free(fo->binarySem);
						free(fo->filePath);
						if(prevfo == NULL){
							filesOpen = fo->next;
						}
						else{
							prevfo->next = fo->next;
						}
						free(fo);
						fo = NULL;
					}
					else{
						fo->filePath = "";
					}
				}
				pthread_mutex_unlock(&lock);

				//remove file
				if(prevFile == NULL){
					c->files = f->next;
				}
				else{
					prevFile->next = f->next;
				}
				free(f);
				//remove client if necessary
				if(c->files == NULL){
					if(prevClient == NULL){
						clients = c->next;
					}
					else{
						prevClient->next = c->next;
					}
					free(c->ip);
					free(c);
				}
				if(send(socket, "zero", 4, 0) < 0){
					perror("Error");
				}
			}
			else{
				//errno
				if(send(socket, "fail", 4, 0) < 0){
					perror("Error");
				}
			}			

			break;
		}
		case 'u':{
			clientAccessParam * p = cParams;
			while(p != NULL){
				if(strcmp(p->ip, clientip) == 0){
					break;
				}
				p = p->next;
			}
			if(p == NULL){
				p = malloc(sizeof(clientAccessParam));
				p->ip = calloc(strlen(clientip), sizeof(char));
				strcpy(p->ip, clientip);
				p->param = 'u';
				p->next = cParams;
				cParams = p;
			}
			else{
				p->param = 'u';
			}
			if((send(socket, "success", 7, 0)) < 0){
				perror("Error");
			}
			break;
		}
		case 'e':{
			clientAccessParam * p = cParams;
			while(p != NULL){
				if(strcmp(p->ip, clientip) == 0){
					break;
				}
				p = p->next;
			}
			if(p == NULL){
				p = malloc(sizeof(clientAccessParam));
				p->ip = calloc(strlen(clientip), sizeof(char));
				strcpy(p->ip, clientip);
				p->param = 'e';
				p->next = cParams;
				cParams = p;
			}
			else{
				p->param = 'e';
			}
			if((send(socket, "success", 7, 0)) < 0){
				perror("Error");
			}
			break;
		}
		case 't':{
			clientAccessParam * p = cParams;
			while(p != NULL){
				if(strcmp(p->ip, clientip) == 0){
					break;
				}
				p = p->next;
			}
			if(p == NULL){
				p = malloc(sizeof(clientAccessParam));
				p->ip = calloc(strlen(clientip), sizeof(char));
				strcpy(p->ip, clientip);
				p->param = 't';
				p->next = cParams;
				cParams = p;
			}
			else{
				p->param = 't';
			}
			if((send(socket, "success", 7, 0)) < 0){
				perror("Error");
			}
			break;
		}
	}
	if(data != NULL) free(data);
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
	server.sin_port = htons(14314);
	
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

void * handle_queue(){
	if(pthread_mutex_init(&lock, NULL) != 0){
		printf("Mutex initialization failed.");
		pthread_exit(NULL);
	}
	time_t timer;
	while(1){
		time(&timer);
		pthread_mutex_lock(&lock);
		fileParam * f = filesOpen;
		while(f != NULL){
			int doPost = 0;
			commandNode * c = f->queue;
			while(c != NULL){
				if(difftime(timer, *(c->timer)) >= 2){
					c->isValid = 0;
					doPost = 1;
				}
				c = c->next;
			}
			if(doPost == 1){
				sem_post(f->binarySem);
			}
			f = f->next;
		}
		pthread_mutex_unlock(&lock);
		sleep(3);
	}
}

