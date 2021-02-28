#include "client.h"
#include "semaphore.h"

sem_t sem;

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
    svr.msgLen = BUFLEN;

    switch (argc)
    {
    case 5:
        clients = atoi(argv[1]);       // Number of clients
        svr.host = argv[2];            // IP
        svr.transfers = atoi(argv[3]); // Number of Transfers
        svr.msgLen = atoi(argv[4]);    // Message Length
        break;
    default:
        fprintf(stderr, "\nUsage: [Number of Clients] [IP] [Transfer Times] [Message Length]\n");
        exit(1);
    }

    // initialize Semaphore
    sem_init(&sem, 0, 0);

    // Open the logfile
    svr.file = fopen(CLNT_LOG_DIR, "w+");
    fprintf(svr.file, "Client,Sent,Received,Transfer Time\n");
    fflush(svr.file);

    // Make client processes
    for (int i = 1; i < clients; i++)
    {
        pid_t id;
        sem_post(&sem);

        if ((id = fork()) < 0)
        {
            perror("\nERROR making child");
            exit(1);
        }
        if (id == 0)
        {
            sem_wait(&sem);
            svr.clientNum = (i);
            break;
        }
        if (id != 0 && i == (clients - 1))
        {
            // sem_post(&sem);
            svr.clientNum = (i + 1);
        }
    }

    client_work(svr);

    return 0;
}

void client_work(struct ServerInfo info)
{
    struct ServerInfo svr = info;
    int sd;
    struct timeval start, end;

    // Setup Socket
    sd = setup_client(svr.port, svr.host, svr.clientNum);
    // fprintf(stdout, "\nConnected : #%d               \n", svr.clientNum);
    fflush(stdout);

    // Send Messages
    char initBuff[BUFLEN], rbuf[svr.msgLen], sbuf[svr.msgLen];
    char *rp, *sp;
    int n, msgLen, to_read;

    // Set up Buffer with Client Number and length of message
    memset(initBuff, 0, sizeof(initBuff));
    memset(sbuf, 0, sizeof(sbuf));

    write_init_msg(svr, initBuff);
    memset(sbuf, 'A', sizeof(sbuf));

    // Start time
    gettimeofday(&start, NULL);

    while (svr.clientRcvd < (svr.transfers)) // First message is message length
    {
        n = 0;

        if (svr.clientRcvd == 0)
        {
            sp = initBuff;
            rp = initBuff;
            msgLen = BUFLEN;
            to_read = BUFLEN;
        }
        else
        {
            sp = sbuf;
            rp = rbuf;
            msgLen = svr.msgLen;
            to_read = svr.msgLen;
        }

        // Senda Data
        send(sd, sp, msgLen, 0); // Messages
        svr.clientSent += 1;     // Messages Client Sent
        // Wait for Server Echo
        while ((n = recv(sd, rp, to_read, 0)) < msgLen)
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
    double t_time = (e_time - s_time) / 1000;

    // Close socket
    close(sd);

    // P(svr.sid);
    fprintf(svr.file, "%d,%d,%d,%f\n", svr.clientNum, svr.clientSent, svr.clientRcvd, t_time);
    fflush(svr.file);
    // V(svr.sid);
}

void write_init_msg(struct ServerInfo svr, char *buf)
{
    char client[NUM_LEN];
    char length[NUM_LEN];
    char *split = "|";

    sprintf(client, "%d", svr.clientNum);
    sprintf(length, "%d", svr.msgLen);

    strncat(buf, client, strlen(client));
    strncat(buf, split, 1);

    strncat(buf, length, strlen(length));
    strncat(buf, split, 1);
}

int setup_client(int port, char *host, int clientNum)
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
        exit(1);
    }

    return sd;
}
