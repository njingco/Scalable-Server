#include "server.h"

//Globals
int fd_server;
int totalConnected;
int totalSent;

pthread_mutex_t conn_add;
pthread_mutex_t conn_sub;

int main(int argc, char *argv[])
{
    struct sigaction act;
    pthread_t thread[THREAD_COUNT];

    // Log Variables
    totalConnected = 0;
    totalSent = 0;

    // Signal Handler
    signal_handle(&act);

    // Listener Socket
    fd_server = setup_listener_socket();

    // Create Worker Threads
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_create(&thread[i], NULL, event_handler, NULL))
        {
            SystemFatal("pthread create");
        }
    }

    // Join threads
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(thread[i], NULL);
    }

    fprintf(stdout, "\nEnd Program");
    fflush(stdout);

    return 0;
}

void *event_handler(void *arg)
{
    int epoll_fd = 0;
    int num_events = 0;
    struct epoll_event events[EPOLL_QUEUE_LEN];
    struct epoll_event event;
    int *msgLen = (int *)malloc(sizeof(int));
    int *clientNum = (int *)malloc(sizeof(int));
    *msgLen = 0;
    *clientNum = 0;

    epoll_fd = setup_epoll(&event);

    while (1)
    {
        num_events = epoll_wait(epoll_fd, events, EPOLL_QUEUE_LEN, -1);
        if (num_events < 0)
            SystemFatal("\nError in epoll_wait!");

        for (int i = 0; i < num_events; i++)
        {
            // Error Case
            if (events[i].events & (EPOLLHUP | EPOLLERR))
            {
                if (errno != EAGAIN)
                {
                    fprintf(stderr, "EPOLL ERROR %d", errno);
                    // perror("\nEPOLL ERROR");
                    close(events[i].data.fd);
                    continue;
                }
            }
            // assert(events[i].events & EPOLLIN);

            // Connection Request
            if (events[i].data.fd == fd_server)
            {
                if (accept_connection(epoll_fd, events) < 0)
                {
                    if (errno != EAGAIN)
                    {
                        fprintf(stdout, "\nERROR ACCEPTING NEW CONNECTIONS %d", errno);
                        // perror("\nERROR ACCEPTING NEW CONNECTIONS");
                        close_fd(errno);
                    }
                    else
                        continue;
                }
            }
            else
            {
                int echo = 0;
                echo = echo_message(events[i].data.fd, msgLen, clientNum);

                // Closed Connection
                if (echo == 0)
                {
                    close(events[i].data.fd);

                    pthread_mutex_lock(&conn_sub);
                    totalConnected -= 1;
                    fprintf(stdout, "\nTotal Connected: %d", totalConnected);
                    fflush(stdout);
                    pthread_mutex_unlock(&conn_sub);
                }
                else if (echo < 0)
                {
                    if (errno != EAGAIN)
                    {
                        // perror("\nReceive Error");
                        fprintf(stdout, "\nReceive Error %d", errno);
                        close(events[i].data.fd);
                    }
                }
            }
        }
    }

    return NULL;
}

int echo_message(int fd, int *msgLen, int *client)
{
    int length;

    if (*msgLen == 0)
        length = BUFLEN;
    else
        length = *msgLen;

    int n = 1, bytes_to_read;
    char *bp, buf[length];

    while (n != 0)
    {

        bp = buf;
        bytes_to_read = length;

        while ((n = recv(fd, bp, bytes_to_read, 0)) < length)
        {
            if (n == 0)
                break;

            else if (n < 0)
                return n;
            else
            {
                bp += n;
                bytes_to_read -= n;
            }
        }
        // Get the Client number and Message Length
        if (*msgLen == 0)
        {
            // get number
            char *token = strtok(buf, "|");
            *client = atoi(token);

            token = strtok(NULL, "|");
            *msgLen = atoi(token);

            fprintf(stdout, "\nClient %d | Size: %d", *client, *msgLen);
        }
        if (send(fd, bp, length, 0) < 0)
        {
            fprintf(stderr, "\nSending ERROR %d", errno);
        }
        else
            totalSent += 1;
        // break;
        // printf("%d sending...\n", totalSent);
    }
    return 1;
}

int accept_connection(int epoll_fd, struct epoll_event *event)
{
    int client_fd = 0;
    struct sockaddr_in remote_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    // Accept new Connectinos
    client_fd = accept(fd_server, (struct sockaddr *)&remote_addr, &addr_size);

    if (client_fd == -1)
        return -1;

    // Make the fd_new non-blocking
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK | fcntl(client_fd, F_GETFL, 0)) == -1)
        perror("\nUNBLOCK:");

    // Add the new socket descriptor to the epoll loop
    event->data.fd = client_fd;
    event->events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, event) == -1)
        SystemFatal("epoll_ctl");

    // Increment Total Connection
    pthread_mutex_lock(&conn_add);
    totalConnected += 1;
    fprintf(stdout, "\nTotal Connected: %d \t\t Remote Address:  %s\n", totalConnected, inet_ntoa(remote_addr.sin_addr));
    fflush(stdout);
    pthread_mutex_unlock(&conn_add);

    return 0;
}

int setup_epoll(struct epoll_event *event)
{
    int epoll_fd;

    // Create the epoll file descriptor
    epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
    if (epoll_fd == -1)
        SystemFatal("epoll_create");

    // Add the server socket to the epoll event loop
    event->events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
    event->data.fd = fd_server;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_server, event) == -1)
        SystemFatal("epoll_ctl");

    return epoll_fd;
}

int setup_listener_socket()
{
    int svrSocket;
    struct sockaddr_in svrDesc;

    // Setup listening Socket
    svrSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (svrSocket == -1)
        SystemFatal("socket");

    // set SO_REUSEADDR imediate port reuse
    int arg = 1;
    if (setsockopt(svrSocket, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
        SystemFatal("setsockopt");

    // Make the listening socket non-blocking
    if (fcntl(svrSocket, F_SETFL, O_NONBLOCK | fcntl(svrSocket, F_GETFL, 0)) == -1)
        SystemFatal("fcntl");

    // Bind to the specified listening port
    memset(&svrDesc, 0, sizeof(struct sockaddr_in));
    svrDesc.sin_family = AF_INET;
    svrDesc.sin_addr.s_addr = htonl(INADDR_ANY);
    svrDesc.sin_port = htons(SERVER_PORT);

    if (bind(svrSocket, (struct sockaddr *)&svrDesc, sizeof(svrDesc)) == -1)
        SystemFatal("bind");

    // Listen for fd_news; SOMAXCONN is 128 by default
    if (listen(svrSocket, SOMAXCONN) == -1)
        SystemFatal("listen");

    return svrSocket;
}

void signal_handle(struct sigaction *act)
{
    // Set Up Signal Handler
    act->sa_handler = close_fd;
    act->sa_flags = 0;

    if ((sigemptyset(&act->sa_mask) == -1 || sigaction(SIGINT, act, NULL) == -1))
    {
        //!Print logs

        perror("Failed to set SIGINT handler");
        exit(EXIT_FAILURE);
    }
}
// close fd
void close_fd(int signo)
{
    fprintf(stdout, "\n\n CLOSE");
    fflush(stdout);

    close(fd_server);
    exit(EXIT_SUCCESS);
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char *message)
{
    fprintf(stdout, "\n\nSYSTEM FATAL");
    fflush(stdout);

    perror(message);
    exit(EXIT_FAILURE);
}