#include "server.h"

//Globals
int fd_server;
int totalConnected;
int totalSent;
int main(int argc, char *argv[])
{
    struct sigaction act;
    int arg;
    int port = SERVER_PORT;
    struct sockaddr_in addr;
    pthread_t thread[EPOLL_QUEUE_LEN];
    totalConnected = 0;
    totalSent = 0;

    // Set Up Signal Handler
    act.sa_handler = close_fd;
    act.sa_flags = 0;
    if ((sigemptyset(&act.sa_mask) == -1 || sigaction(SIGINT, &act, NULL) == -1))
    {
        //!Print logs

        perror("Failed to set SIGINT handler");
        exit(EXIT_FAILURE);
    }

    // Setup listening Socket
    fd_server = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_server == -1)
        SystemFatal("socket");

    // set SO_REUSEADDR imediate port reuse
    arg = 1;
    if (setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
        SystemFatal("setsockopt");

    // Make the listening socket non-blocking
    if (fcntl(fd_server, F_SETFL, O_NONBLOCK | fcntl(fd_server, F_GETFL, 0)) == -1)
        SystemFatal("fcntl");

    // Bind to the specified listening port
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(fd_server, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        SystemFatal("bind");

    // Listen for fd_news; SOMAXCONN is 128 by default
    if (listen(fd_server, SOMAXCONN) == -1)
        SystemFatal("listen");

    for (int i = 0; i < EPOLL_QUEUE_LEN; i++)
    {
        if (pthread_create(&thread[i], NULL, epoll_loop, (void *)&arg))
        {
            SystemFatal("pthread create");
        }
    }

    // Join threads
    for (int i = 0; i < EPOLL_QUEUE_LEN; i++)
    {
        pthread_join(thread[i], NULL);
    }

    return 0;
}

void *epoll_loop(void *arg)
{
    int epoll_fd, num_fds, fd_new;
    static struct epoll_event events[EPOLL_QUEUE_LEN], event;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    struct sockaddr_in remote_addr;

    // Create the epoll file descriptor
    epoll_fd = epoll_create(EPOLL_QUEUE_LEN);

    if (epoll_fd == -1)
        SystemFatal("epoll_create");

    // Add the server socket to the epoll event loop
    // event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = fd_server;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_server, &event) == -1)
        SystemFatal("epoll_ctl");

    while (TRUE)
    {
        //struct epoll_event events[MAX_EVENTS];
        num_fds = epoll_wait(epoll_fd, events, EPOLL_QUEUE_LEN, -1);

        if (num_fds < 0)
            SystemFatal("Error in epoll_wait!");

        for (int i = 0; i < num_fds; i++)
        {
            // Case 1: Error condition
            if (events[i].events & (EPOLLHUP | EPOLLERR))
            {
                // fputs("epoll: EPOLLERR", stderr);
                fprintf(stderr, "\nepoll: EPOLLERR");
                fflush(stderr);
                close(events[i].data.fd);
                continue;
            }
            assert(events[i].events & EPOLLIN);

            // Case 2: Server is receiving a connection request
            if (events[i].data.fd == fd_server)
            {
                //socklen_t addr_size = sizeof(remote_addr);
                fd_new = accept(fd_server, (struct sockaddr *)&remote_addr, &addr_size);
                if (fd_new == -1)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        // perror("accept");
                    }
                    continue;
                }

                // Make the fd_new non-blocking
                if (fcntl(fd_new, F_SETFL, O_NONBLOCK | fcntl(fd_new, F_GETFL, 0)) == -1)
                    perror("\nunblock");

                // Add the new socket descriptor to the epoll loop
                event.data.fd = fd_new;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_new, &event) == -1)
                    SystemFatal("epoll_ctl");

                totalConnected += 1;
                fprintf(stdout, "\nTotal Connected: %d \t\t Remote Address:  %s\n", totalConnected, inet_ntoa(remote_addr.sin_addr));
                // printf("\nRemote Address:  %s", inet_ntoa(remote_addr.sin_addr));
                continue;
            }

            // Case 3: One of the sockets has read data
            if (!read_socket(events[i].data.fd))
            {
                // epoll will remove the fd from its set
                // automatically when the fd is closed
                // close(events[i].data.fd);
            }
        }
    }
    return NULL;
}

// Function to read socket data
static int read_socket(int fd)
{
    int n = -1, bytes_to_read = BUFLEN;
    char rbuf[BUFLEN];
    char *rbp;

    // fprintf(stdout, "test");
    rbp = rbuf;

    while (n != 0)
    {
        while ((n = recv(fd, &rbuf, bytes_to_read, 0)) < BUFLEN)
        {
            if (n == -1 && (errno != EAGAIN || errno != EWOULDBLOCK))
            {
                perror("\nrcv error");
                return n;
            }
            else
            {
                rbp += n;
                bytes_to_read -= n;
            }
        }
        send(fd, rbp, BUFLEN, 0);
        totalSent += 1;
        // printf("%d sending...\n", totalSent);

        memset(rbuf, '\0', sizeof(rbuf));
        rbp = rbuf;
        bytes_to_read = BUFLEN;
    }

    // fprintf(stdout, "\nMessages Sent: %d\n", totalSent);
    fflush(stdout);
    return TRUE;
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

// close fd
void close_fd(int signo)
{
    close(fd_server);
    exit(EXIT_SUCCESS);
}