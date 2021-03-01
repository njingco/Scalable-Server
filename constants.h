#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <semaphore.h>

#define SERVER_PORT 7000
#define BUFLEN 1024 // Buffer length
#define NUM_LEN 6

#endif