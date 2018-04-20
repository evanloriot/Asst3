#include <stdlib.h>
#ifndef LIBNETFILES_H
#define LIBNETFILES_H

int netserverinit(char *hostname);

int connectToServer(char * hostname);

int netopen(char *pathname, int flags);

ssize_t netread(int fildes, const void *buf, size_t nbyte);

ssize_t netwrite(int fildes, const void *buf, size_t nbyte);

int netclose(int fd);


#endif
