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
	int z = connectToServer("flyweight.cs.rutgers.edu");
	printf("%d\n", z);
	int i = netserverinit("flyweight.cs.rutgers.edu", "unrestricted");	
	printf("%d\n", i);
	char * b = "asdfshjshshshbfkjfjakjskskdhaksdhk";
	int fd = netopen("../test/test.c", O_RDWR);	
//	fd = netopen("../test/test.c", O_RDONLY);
//	int x;
	int w = netwrite(fd, b, strlen(b));
	printf("Written: %d\n", w);
	char buff[256];
//	for(x = 0; x < 100; x++) {	
	int r = netread(fd, buff, 12);
	buff[12] = '\0';
	printf("R: %d, Buff: %s\n", r, buff);
//	} 
	int close = netclose(fd);
	printf("Close: %d\n", close);
	if(close < 0) perror("Error");
	return 0;
}
