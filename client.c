#include "client.h"

int *dataSize;
#define SERVER_TCP_PORT 7000 // Default port
#define BUFLEN 1024          // Buffer length
#define TIMES 100

int main(int argc, char *argv[])
{
    int n, bytes_to_read;
    int sd, port;
    struct hostent *hp;
    struct sockaddr_in server;
    char *host, *bp, rbuf[BUFLEN], sbuf[BUFLEN], **pptr;
    char str[16];
    int times;
    int clntNum;
    int totalRcvd = 0, totalSent = 0;
    port = SERVER_TCP_PORT;
    times = TIMES;
    clntNum = 0;

    switch (argc)
    {
    // case 2:
    //     host = argv[1]; // Host name
    //     times = TIMES;
    //     clntNum = 0;
    //     break;
    // case 3:
    //     host = argv[1];
    //     times = atoi(argv[2]); // Number of sends
    //     clntNum = 0;
    //     break;
    case 4:
        host = argv[1];
        times = atoi(argv[2]); // Number of sends
        clntNum = atoi(argv[3]);
        break;
    default:
        fprintf(stderr, "Usage: %s host [Times Sent] [Client Number]\n", argv[0]);
        exit(1);
    }

    fprintf(stdout, "\nClient: %d", clntNum);

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

    // printf("Connected    Server Name: %s\n", hp->h_name);
    pptr = hp->h_addr_list;
    printf("\nConnected IP Address: %s\n", inet_ntop(hp->h_addrtype, *pptr, str, sizeof(str)));
    // printf("Transmit:\n");

    // get user's text
    // fgets(sbuf, BUFLEN, stdin);
    memset(sbuf, 'A', sizeof(sbuf));
    sbuf[sizeof(sbuf)] = '\0';

    int rcved = 0;
    while (rcved < times)
    {
        // Transmit data through the socket

        send(sd, sbuf, BUFLEN, 0);
        totalSent += 1;

        // printf("Receive:\n");
        bp = rbuf;
        bytes_to_read = BUFLEN;

        // client makes repeated calls to recv until no more data is expected to arrive.
        n = 0;
        while ((n = recv(sd, bp, bytes_to_read, 0)) < BUFLEN)
        {
            bp += n;
            bytes_to_read -= n;
        }
        totalRcvd += 1;

        fprintf(stdout, "\nReceived: %s", rbuf);
        fflush(stdout);
        rcved += 1;
    }

    fprintf(stdout, "\nClient(%d) Total Sent: %d", clntNum, totalSent);
    fprintf(stdout, "\nClient(%d) Total Received: %d", clntNum, totalRcvd);

    close(sd);
    return (0);
    return 0;
}