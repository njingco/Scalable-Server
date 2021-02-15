#ifndef SERVER_H
#define SERVER_H

#include "constants.h"

#define TRUE 1
#define FALSE 0
#define EPOLL_QUEUE_LEN 256
#define BUFLEN 80
#define SERVER_PORT 7000

static void SystemFatal(const char *message);
void close_fd(int signo);

struct Stats
{
    int x;
};

#endif