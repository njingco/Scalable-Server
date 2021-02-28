#ifndef SERVER_H
#define SERVER_H

#include "constants.h"

#define TRUE 1
#define FALSE 0
#define EPOLL_QUEUE_LEN 250
#define BUFLEN 1024
#define SERVER_PORT 7000
#define THREAD_COUNT 10000

void signal_handle(struct sigaction *act);
int setup_listener_socket();
void *event_handler(void *arg);
int setup_epoll(struct epoll_event *event);
int accept_connection(int epoll_fd, struct epoll_event *event);
int echo_message(int fd, int *msgLen, int *client);

static void SystemFatal(const char *message);
void close_fd(int signo);

struct Stats
{
    int x;
};

#endif