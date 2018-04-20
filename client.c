#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include "libnetfiles.h"

int main(){
	printf("%d", netserverinit("ls.cs.rutgers.edu"));
	int sock = connectToServer("ls.cs.rutgers.edu");


	char buffer[256];
	fgets(buffer, 256, stdin);
	write(sock, buffer, strlen(buffer));
	close(sock);
	return 0;
}
