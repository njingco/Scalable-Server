#ifndef CLIENT_H
#define CLIENT_H

#include "constants.h"
#define SERVER_TCP_PORT 7000 // Default port
#define THREAD_COUNT 15000
#define CLNT_LOG_DIR "clientLog.csv"

struct ServerInfo
{
    char *host;
    int port;
    int transfers;
    int clientNum;
    int clientSent;
    int clientRcvd;
};

int client_work(struct ServerInfo info);
long get_duration(struct timeval start, struct timeval end);
void log_data(struct ServerInfo svr, long time);
void write_init_msg(struct ServerInfo svr, char *buf);
void write_log(struct ServerInfo svr);
int setup_client(int port, char *host);

#endif