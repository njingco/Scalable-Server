#include "client.h"

int main(int argc, char *argv[])
{
    int clients = 1; // Number of Clients Default

    // Set Default
    struct ServerInfo svr;
    svr.port = SERVER_TCP_PORT;
    svr.transfers = 1;
    svr.clientNum = 0;
    svr.clientSent = 0;
    svr.clientRcvd = 0;

    switch (argc)
    {
    case 5:
        clients = atoi(argv[1]);       // Number of clients
        svr.host = argv[2];            // IP
        svr.transfers = atoi(argv[3]); // Number of Transfers
        break;
    default:
        fprintf(stderr, "\nUsage: [Number of Clients] [IP] [Transfer Times] \n");
        exit(1);
    }

    // Open the logfile
    svr.file = fopen(CLNT_LOG_DIR, "w+");
    fprintf(svr.file, "Client,Sent,Received,Transfer Time(ms)\n");
    fflush(svr.file);

    // Make client processes
    int i = 1;
    for (i = 1; i < clients; i++)
    {
        pid_t id;

        if ((id = fork()) < 0)
        {
            perror("\nERROR making child");
            exit(1);
        }
        if (id == 0)
        {
            svr.clientNum = (i);
            break;
        }
        if (id != 0 && i == (clients - 1))
        {
            svr.clientNum = (i + 1);
        }
    }

    if (client_work(svr) < 0)
    {
        return -1;
    }

    return 0;
}

int client_work(struct ServerInfo info)
{
    struct ServerInfo svr = info;
    int sd;
    struct timeval start, end;

    // Setup Socket
    sd = setup_client(svr.port, svr.host);
    if (sd < 0)
        return -1;

    // Send Messages
    char rbuf[BUFLEN], sbuf[BUFLEN];

    char *rp, *sp;
    int n, to_read;

    // Set up Buffer with Client Number and length of message
    memset(sbuf, 'A', sizeof(sbuf));
    write_init_msg(svr, sbuf);

    // Start time
    gettimeofday(&start, NULL);

    while (svr.clientRcvd < (svr.transfers)) // First message is message length
    {
        n = 0;
        sp = sbuf;
        rp = rbuf;

        // Senda Data
        send(sd, sp, BUFLEN, 0); // Messages
        svr.clientSent += 1;     // Messages Client Sent

        // Wait for Server Echo
        while ((n = recv(sd, rp, to_read, 0)) < BUFLEN)
        {
            rp += n;
            to_read -= n;
        }

        // Received Data
        svr.clientRcvd += 1; // Messages Client Received
    }

    // stop time
    gettimeofday(&end, NULL);
    long s_time = (long)(start.tv_sec) * 1000 + (start.tv_usec / 1000);
    long e_time = (long)(end.tv_sec) * 1000 + (end.tv_usec / 1000);
    long t_time = (e_time - s_time);

    // Close socket
    close(sd);

    fprintf(svr.file, "%d,%d,%d,%ld\n", svr.clientNum, svr.clientSent, svr.clientRcvd, t_time);
    fflush(svr.file);
    return 0;
}

void write_init_msg(struct ServerInfo svr, char *buf)
{
    char client[NUM_LEN];
    char *split = "|";

    sprintf(client, "%d", svr.clientNum);

    char temp[strlen(client) + 2];

    strncat(temp, client, strlen(client));
    strncat(temp, split, 1);

    strncpy(buf, temp, strlen(client) + 2);
}

int setup_client(int port, char *host)
{
    int sd;
    struct hostent *hp;
    struct sockaddr_in server;

    // Create the socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Cannot create socket");
        exit(1);
    }

    bzero((char *)&server, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if ((hp = gethostbyname(host)) == NULL)
    {
        fprintf(stderr, "Unknown server address\n");
        exit(1);
    }
    bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);

    // Connecting to the server
    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        fprintf(stderr, "Can't connect to server\n");
        perror("connect");
        return -1;
    }

    return sd;
}
