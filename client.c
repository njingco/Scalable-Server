#include "client.h"

int totalRcvd;
int totalSent;
int clientNum;
pthread_mutex_t total_mux;
pthread_mutex_t clientNum_mux;

int main(int argc, char *argv[])
{
    int clients; // Number of Clients Default

    // VAR default
    totalRcvd = 0;
    totalSent = 0;
    clientNum = 0;

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

    // Open the logfile
    svr.cvs = fopen(FILE_DIR, "a+");

    // Create the Client Threads
    pthread_t thread[clients];

    for (int i = 0; i < clients; i++)
    {
        if (pthread_create(&thread[i], NULL, client_work, (void *)&svr))
        {
            SystemFatal("\npthread create");
        }
    }

    // Join threads
    for (int i = 0; i < clients; i++)
    {
        pthread_join(thread[i], NULL);
    }

    fprintf(stdout, "\nEnd Program");
    fflush(stdout);
    return (0);
}

void *client_work(void *arg)
{
    struct ServerInfo svr = *((struct ServerInfo *)arg);
    int sd;

    pthread_mutex_lock(&clientNum_mux);
    svr.clientNum = (clientNum += 1);
    pthread_mutex_unlock(&clientNum_mux);

    // Setup Socket
    sd = setup_client(svr);

    // Send Messages
    char initBuff[BUFLEN], rbuf[svr.msgLen], sbuf[svr.msgLen];
    char *rp, *sp;
    int n, msgLen, to_read;

    // Set up Buffer with Client Number and length of message
    memset(initBuff, 0, sizeof(initBuff));
    memset(sbuf, 0, sizeof(sbuf));

    write_init_msg(svr, initBuff);
    memset(sbuf, 'A', sizeof(sbuf));

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

        fprintf(stdout, "\nClient: %d snd %d", svr.clientNum, svr.clientSent);
        fflush(stdout);

        // Wait for Server Echo
        while ((n = recv(sd, rp, to_read, 0)) < msgLen)
        {
            rp += n;
            to_read -= n;
        }

        // Received Data
        svr.clientRcvd += 1; // Messages Client Received
        fprintf(stdout, "\nClient: %d rcv %d", svr.clientNum, svr.clientRcvd);
        fflush(stdout);
    }

    // Close socket
    close(sd);
    fprintf(stdout, "\n------------------");
    fprintf(stdout, "\nClient: %d Closed", svr.clientNum);
    fprintf(stdout, "\n------------------\n");

    return NULL;
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

void write_log(struct ServerInfo svr)
{
    // Add to total Massages sent and received
    pthread_mutex_lock(&total_mux);
    // Connected to (svr->host)
    // CLient Number (svr->clientNum)
    // Lengh of Message (svr->msgLen)
    totalSent += svr.clientSent;
    totalRcvd += svr.clientRcvd;

    // Write log here

    pthread_mutex_unlock(&total_mux);
}

int setup_client(struct ServerInfo svr)
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
    server.sin_port = htons(svr.port);

    if ((hp = gethostbyname(svr.host)) == NULL)
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
    fprintf(stdout, "\nConnected %d\n", svr.clientNum);

    return sd;
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char *message)
{
    fprintf(stdout, "\n\nSYSTEM FATAL");
    fflush(stdout);

    perror(message);
    exit(EXIT_FAILURE);
}
