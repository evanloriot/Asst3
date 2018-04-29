#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "libnetfiles.h"

int main(){
	netserverinit("ls.cs.rutgers.edu", "unrestricted");
	//netserverinit("ls.cs.rutgers.edu", "transaction");
	//netserverinit("ls.cs.rutgers.edu", "exclusive");
	int fd = netopen("./test", O_RDWR);
	//int fd = netopen("../../Desktop/test/test.c", O_RDONLY);
	printf("FD: %d\n", fd);
	perror("ERRORFD");
	//char buff[256];
	//int r;
	//while((r = netread(-9, &buff, 255)) > 0){
	//	buff[r] = '\0';
	//	printf("%s", buff);
	//}
	//perror("ErrorREAD");
	//printf("\n");
	//int w = netwrite(-9, "123", strlen("123"));
	//perror("ErrorWRITE");
	//printf("W: %d\n", w);
	//netclose(-9);
	//perror("ErrorCLOSE");
	
	return 1;
}
