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
	int fd = netopen("../test/test.c", O_RDWR);
	printf("%d\n", fd);
	char buff[256];
	int r = netread(fd, &buff, 25);
	printf("%s\n", buff);
	return 0;
}
