#ifndef SERVER_H
#define SERVER_H

#include "constants.h"

#define TRUE 1
#define FALSE 0
#define EPOLL_QUEUE_LEN 5000
#define BUFLEN 1024
#define SERVER_PORT 7000

static void SystemFatal(const char *message);
void close_fd(int signo);
void *epoll_loop(void *arg);
static int read_socket(int fd);

struct Stats
{
    int x;
};

#endif