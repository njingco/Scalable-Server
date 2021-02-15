#include "server.h"

//Globals
int fd_server;

int main(int argc, char *argv[])
{

    return 0;
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