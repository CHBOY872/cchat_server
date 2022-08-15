#include "server.h"

static int port = 8808;

int main()
{
    int fd_server = server_create(port);
    return run(fd_server);
}

/*
    DEBUG CODE IN VALGRIND
    FORMAT CODE
    COMMENT CODE
*/