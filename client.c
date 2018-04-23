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
	netserverinit("design.cs.rutgers.edu", "unrestricted");
	char * b = "asdlkjfhaslkjdfhlaksjdhflkjashdflkjashdlfkjhasdlkjfhaslkjdhflkjashdflkjashdlkjfhaslkjdfhaslkjdhflaksjdhflkjashdflkjashdflkjahsdlkfjhaslkdjfhasljkdhfljkashdfljkashdfjlkhasljkdfhlaskjdhflkjashdflkjashdlkfjhasldkjfhaslkjdhflkjashdflkjahsdlkfjhaslkdjfhasl.txt";
	int fd = netopen("../test/test.c", O_RDWR);
	int w = netwrite(fd, b, strlen(b));
	printf("Written: %d\n", w);
	char buff[256];
	int r = netread(fd, buff, 12);
	buff[12] = '\0';
	printf("R: %d, Buff: %s\n", r, buff); 
	int close = netclose(-1);
	printf("Close: %d\n", close);
	if(close < 0) perror("Error");
	return 0;
}
