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
	int fd = netopen("../test/test.c", O_RDWR);
	//int fd = netopen("../../Desktop/test/test.c", O_RDONLY);
	printf("FD: %d\n", fd);
	//char buff[256];
	//int r;
	//while((r = netread(fd, &buff, 255)) > 0){
	//	buff[r] = '\0';
	//	printf("%s", buff);
	//}
	//printf("\n");
	//int w = netwrite(fd, "123", strlen("123"));
	//printf("W: %d\n", w);
	//sleep(1);
	//netclose(fd);
	
	return 1;
}
