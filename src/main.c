#include <stdio.h>
#include "server.h"

static int port = 8808;

int main()
{
    struct server *serv = server_init(port);
    if (!serv)
    {
        perror("server");
        return 1;
    }
    return server_run(serv);
}