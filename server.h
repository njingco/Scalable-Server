#ifndef SERVER_H
#define SERVER_H

#include "constants.h"

#define IP_LEN 13
#define TRUE 1
#define FALSE 0
#define EPOLL_QUEUE_LEN 250
#define SERVER_PORT 7000
#define THREAD_COUNT 10000
#define SVR_LOG_DIR "serverLog.csv"

struct ServerStats
{
    char *ip;
    char *client;
    int *sent;
    int *rcvd;
};

void *event_handler(void *arg);
int echo_message(int fd, struct ServerStats *svr);
void reset_stats(struct ServerStats *svr);
int accept_connection(int epoll_fd, struct epoll_event *event);
int setup_epoll(struct epoll_event *event);
int setup_listener_socket();
void signal_handle(struct sigaction *act);
void close_fd(int signo);
static void SystemFatal(const char *message);

#endif