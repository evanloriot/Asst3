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
	netserverinit("cd.cs.rutgers.edu");
	int fd = netopen("../test/test.c", O_RDONLY);
	printf("%d\n", fd);
	return 0;
}
