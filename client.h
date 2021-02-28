#ifndef CLIENT_H
#define CLIENT_H

#include "constants.h"
#define SERVER_TCP_PORT 7000 // Default port
#define BUFLEN 1024          // Buffer length
#define THREAD_COUNT 15000
#define NUM_LEN 6
#define FILE_DIR "clientLog.cvs"

struct ServerInfo
{
    char *host;
    int port;
    int msgLen;
    int transfers;
    int clientNum;
    int clientSent;
    int clientRcvd;
    FILE *cvs;
};

int setup_client(struct ServerInfo svr);
void *client_work(void *arg);
void write_init_msg(struct ServerInfo svr, char *buf);
void write_log(struct ServerInfo svr);
static void SystemFatal(const char *message);
#endif