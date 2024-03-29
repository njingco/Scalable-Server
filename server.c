/*---------------------------------------------------------------------------------------
 * SOURCE FILE:	    server
 * 
 * PROGRAM:		    server
 * 
 * FUNCTIONS:		
 *                  void *event_handler(void *arg);
 *                  int echo_message(int fd, struct ServerStats *svr);
 *                  void reset_stats(struct ServerStats *svr);
 *                  int accept_connection(int epoll_fd, struct epoll_event *event);
 *                  int setup_epoll(struct epoll_event *event);
 *                  int setup_listener_socket();
 *                  void signal_handle(struct sigaction *act);
 *                  void close_fd(int signo);
 *                  static void SystemFatal(const char *message);
 * 
 * DATE:			February  12, 2020
 * 
 * REVISIONS:		NA
 * 
 * DESIGNERS:       Nicole Jingco
 * 
 * PROGRAMMERS:		Nicole Jingco
 * 
 * Notes:
 * This is the main file to run the server program
 * ---------------------------------------------------------------------------------------*/
#include "server.h"

//Globals
int fd_server;
int totalConnected;

pthread_mutex_t connect_counter;
FILE *file;

/*--------------------------------------------------------------------------
 * FUNCTION:        main
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      int argc, char *argv[] - Not used
 *
 * RETURNS:        
 *
 * NOTES:
 * This is the main server function. This function sets up the listener 
 * socket , and creates the worker threads.
 * -----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    struct sigaction act;
    pthread_t thread[THREAD_COUNT];

    // Log Variables
    totalConnected = 0;

    // Signal Handler
    signal_handle(&act);

    // Log File
    file = fopen(SVR_LOG_DIR, "a+");

    fprintf(file, "IP,Client,Request,Sent\n");
    fflush(file);

    // Listener Socket
    fd_server = setup_listener_socket();
    int i = 0;
    // Create Worker Threads
    for (i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_create(&thread[i], NULL, event_handler, NULL))
        {
            SystemFatal("pthread create");
        }
    }

    fprintf(stdout, "\nStart Connecting");
    fflush(stdout);

    // Join threads
    int j = 0;
    for (j = 0; j < THREAD_COUNT; j++)
    {
        pthread_join(thread[j], NULL);
    }

    fprintf(stdout, "\nEnd Program");
    fflush(stdout);

    return 0;
}

/*--------------------------------------------------------------------------
 * FUNCTION:       event_handler
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      void * arg - not used
 *
 * RETURNS:        void *
 *
 * NOTES:
 * This is the functions threads use, This will setup epoll, and run the
 * epoll wait. This will also respond to the events such as accept new
 * clients and run the echo function.
 * -----------------------------------------------------------------------*/
void *event_handler(void *arg)
{
    int epoll_fd = 0;
    int num_events = 0;
    struct epoll_event events[EPOLL_QUEUE_LEN];
    struct epoll_event event;

    struct ServerStats *svr = (struct ServerStats *)malloc(sizeof(struct ServerStats));
    svr->client = (char *)malloc(sizeof(char) * NUM_LEN);
    svr->rcvd = (int *)malloc(sizeof(int));
    svr->sent = (int *)malloc(sizeof(int));
    svr->ip = (char *)malloc(sizeof(char) * IP_LEN);

    reset_stats(svr);

    epoll_fd = setup_epoll(&event);

    while (1)
    {
        num_events = epoll_wait(epoll_fd, events, EPOLL_QUEUE_LEN, -1);
        if (num_events < 0)
            SystemFatal("\nError in epoll_wait!");

        int i = 0;
        for (i = 0; i < num_events; i++)
        {
            // Error Case
            if (events[i].events & (EPOLLHUP | EPOLLERR))
            {
                if (errno != EAGAIN)
                {
                    fprintf(stderr, "EPOLL ERROR %d", errno);
                    close(events[i].data.fd);
                    continue;
                }
            }

            // Connection Request
            if (events[i].data.fd == fd_server)
            {
                if (accept_connection(epoll_fd, events) < 0)
                {
                    if (errno != EAGAIN)
                    {
                        fprintf(stdout, "\nERROR ACCEPTING NEW CONNECTIONS %d", errno);
                        close_fd(errno);
                    }
                    else
                        continue;
                }
                else
                {
                    // Increment Total Connection
                    pthread_mutex_lock(&connect_counter);
                    totalConnected += 1;
                    fprintf(stdout, "\rTotal Connected: %d         ", totalConnected);
                    fflush(stdout);
                    pthread_mutex_unlock(&connect_counter);
                }
            }
            else
            {
                int echo = 0;
                echo = echo_message(events[i].data.fd, svr);

                // Closed Connection
                if (echo == 0)
                {
                    close(events[i].data.fd);

                    pthread_mutex_lock(&connect_counter);
                    totalConnected -= 1;
                    fprintf(stdout, "\rTotal Connected: %d         ", totalConnected);
                    fflush(stdout);
                    pthread_mutex_unlock(&connect_counter);
                }
                else if (echo < 0)
                {
                    if (errno != EAGAIN)
                    {
                        perror("\nReceive Error");
                        close(events[i].data.fd);
                    }
                }

                // reset stats
                if (*svr->rcvd != 0 || *svr->sent != 0)
                {
                    pthread_mutex_lock(&connect_counter);
                    // IP, Client, number requested, number sent
                    fprintf(file, "%s,%s,%d,%d\n", svr->ip, svr->client, *svr->rcvd, *svr->sent);
                    fflush(file);
                    reset_stats(svr);
                    pthread_mutex_unlock(&connect_counter);
                }
            }
        }
    }
    return NULL;
}

/*--------------------------------------------------------------------------
 * FUNCTION:       echo_message
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      int fd -  socket
 *                 struct ServerStats *svr -  stats
 *
 * RETURNS:        int -1 if error, 0 if socket closed, 1 continue
 *
 * NOTES:
 * This function handles the receive and send to echo client message. 
 * This will also populate the client data to the svr structure
 * -----------------------------------------------------------------------*/
int echo_message(int fd, struct ServerStats *svr)
{
    int n = 1, bytes_to_read;
    char *bp, buf[BUFLEN];

    struct sockaddr_in addr;
    socklen_t len;
    len = sizeof addr;

    getpeername(fd, (struct sockaddr *)&addr, &len);
    strcpy(svr->ip, inet_ntoa(addr.sin_addr));

    while (n != 0)
    {
        bp = buf;
        bytes_to_read = BUFLEN;

        while ((n = recv(fd, bp, bytes_to_read, 0)) < BUFLEN)
        {
            if (n <= 0)
                return n;
            else
            {
                bp += n;
                bytes_to_read -= n;
            }
        }
        *svr->rcvd += 1;

        // Client Number
        char *token = strtok(buf, "|");
        strcpy(svr->client, token);

        // SEND
        if (send(fd, bp, BUFLEN, 0) < 0)
        {
            fprintf(stderr, "\nSending ERROR %d", errno);
        }
        else
        {
            *svr->sent += 1;
        }
    }
    return 0;
}

/*--------------------------------------------------------------------------
 * FUNCTION:       reset_stats
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      struct ServerStats *svr - Stats
 *
 * RETURNS:        NA
 *
 * NOTES:
 * This function reset the stats structure
 * -----------------------------------------------------------------------*/
void reset_stats(struct ServerStats *svr)
{
    strcpy(svr->client, "0");
    *svr->rcvd = 0;
    *svr->sent = 0;
}

/*--------------------------------------------------------------------------
 * FUNCTION:       accept_connection
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      int epoll_fd -  listener socket
 *                 struct epoll_event *event - epoll event
 *
 * RETURNS:        return -1 if an error occured
 *
 * NOTES:
 * This function handles the accept, and set the socket to non blocking
 * -----------------------------------------------------------------------*/
int accept_connection(int epoll_fd, struct epoll_event *event)
{
    int client_fd = 0;
    struct sockaddr_in remote_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    // Accept new Connections
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

    return 0;
}

/*--------------------------------------------------------------------------
 * FUNCTION:       setup_epoll
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      struct epoll_event *event
 *
 * RETURNS:        return the epoll descriptor
 *
 * NOTES:
 * This function sets up epoll 
 * -----------------------------------------------------------------------*/
int setup_epoll(struct epoll_event *event)
{
    int epoll_fd;

    // Create the epoll file descriptor
    epoll_fd = epoll_create(EPOLL_QUEUE_LEN);
    if (epoll_fd == -1)
        SystemFatal("\nepoll_create");

    // Add the server socket to the epoll event loop
    event->events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
    event->data.fd = fd_server;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_server, event) == -1)
        SystemFatal("\nepoll_ctl");

    return epoll_fd;
}

/*--------------------------------------------------------------------------
 * FUNCTION:       setup_listener_socket
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      NA
 *
 * RETURNS:        Return the listener socket
 *
 * NOTES:
 * Setup the listener socket
 * -----------------------------------------------------------------------*/
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

/*--------------------------------------------------------------------------
 * FUNCTION:       signal_handle
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      struct sigaction *act
 *
 * RETURNS:        NA
 *
 * NOTES:
 * This function handles the signal handler to close and clear up 
 * application
 * -----------------------------------------------------------------------*/
void signal_handle(struct sigaction *act)
{
    // Set Up Signal Handler
    act->sa_handler = close_fd;
    act->sa_flags = 0;

    if ((sigemptyset(&act->sa_mask) == -1 || sigaction(SIGINT, act, NULL) == -1))
    {
        perror("Failed to set SIGINT handler");
        exit(EXIT_FAILURE);
    }
}

/*--------------------------------------------------------------------------
 * FUNCTION:       close_fd
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      int signo
 *
 * RETURNS:        NA
 *
 * NOTES:
 * This function closes the socket and exits
 * -----------------------------------------------------------------------*/
void close_fd(int signo)
{
    fprintf(stdout, "\n\n CLOSE\n");
    fflush(stdout);

    close(fd_server);
    exit(EXIT_SUCCESS);
}

/*--------------------------------------------------------------------------
 * FUNCTION:       SystemFatal
 *
 * DATE:           February  12, 2020
 *
 * REVISIONS:      NA
 * 
 * DESIGNER:       Nicole Jingco
 *
 * PROGRAMMER:     Nicole Jingco
 *
 * INTERFACE:      const char *message - error message
 *
 * RETURNS:        NA
 *
 * NOTES:
 * Prints the error stored in errno and aborts the program.
 * -----------------------------------------------------------------------*/
static void SystemFatal(const char *message)
{
    fprintf(stdout, "\n\nSYSTEM FATAL");
    fflush(stdout);

    perror(message);
    exit(EXIT_FAILURE);
}