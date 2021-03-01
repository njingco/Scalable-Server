#ifndef SERVER_H
#define SERVER_H

#include "constants.h"

#define IP_LEN 13
#define TRUE 1
#define FALSE 0
#define EPOLL_QUEUE_LEN 250
#define SERVER_PORT 7000
#define THREAD_COUNT 3000
#define SVR_LOG_DIR "serverLog.csv"

struct ServerStats
{
    char *ip;
    int *client;
    int *sent;
    int *rcvd;
    FILE *file;
};

void signal_handle(struct sigaction *act);
int setup_listener_socket();
void *event_handler(void *arg);
int setup_epoll(struct epoll_event *event);
int accept_connection(int epoll_fd, struct epoll_event *event, struct ServerStats *svr);
int echo_message(int fd, struct ServerStats *svr);
static void SystemFatal(const char *message);
void close_fd(int signo);
void reset_stats(struct ServerStats *svr);

#endif