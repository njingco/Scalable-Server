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
    FILE *file;
};

int client_work(struct ServerInfo info);
void write_init_msg(struct ServerInfo svr, char *buf);
void write_log(struct ServerInfo svr);
int setup_client(int port, char *host);

#endif