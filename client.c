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
	//netserverinit("ls.cs.rutgers.edu", "exclusive");
	netserverinit("ls.cs.rutgers.edu", "unrestricted");
	int fd = netopen("./netfileserver.c", O_RDONLY);
	char buff[256];
	int r;
	while((r = netread(fd, &buff, 255)) > 0){
		buff[r] = '\0';
		printf("%s", buff);
	}
	netclose(fd);
	return 1;
}
