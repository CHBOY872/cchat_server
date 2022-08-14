#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "server.h"

int server_create(int port)
{
    int fd_server = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd_server)
    {
        perror("socket");
        exit(1);
    }
    int opt = 1;
    setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (-1 == bind(fd_server, (struct sockaddr *)&server_addr,
                   sizeof(server_addr)))
    {
        perror("bind");
        exit(2);
    }
    if (-1 == listen(fd_server, MAX_LISTEN_COUNT))
    {
        perror("listen");
        exit(3);
    }
    return fd_server;
}

void accept_client(int server_fd, int *max_fd, struct session ***sess)
{
    int i;
    struct sockaddr_in client_addr;
    socklen_t len = 0;

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
    if (-1 == client_fd)
        return;

    if (!*sess)
    {
        if (*max_fd < client_fd)
            *max_fd = client_fd;
        *sess = malloc(sizeof(struct session) * (*max_fd + 1));
        for (i = 0; i <= *max_fd; i++)
            (*sess)[i] = NULL;
    }
    if (*max_fd < client_fd)
    {
        struct session **sess_tmp =
            malloc(sizeof(struct session) * (client_fd + 1));
        for (i = 0; i <= client_fd; i++)
            sess_tmp[i] = i <= *max_fd ? (*sess)[i] : NULL;
        *max_fd = client_fd;
    }
    (*sess)[i] = malloc(sizeof(struct session));
    (*sess)[i]->fd = client_fd;
    (*sess)[i]->name = NULL;
    (*sess)[i]->step = no_name_step;
    (*sess)[i]->buf_used = 0;
}

int run(int fd_server)
{
    fd_set rds;
    int max_fd = fd_server;
    int i;
    struct session **sess = NULL;
    for (;;)
    {
        FD_ZERO(&rds);
        FD_ZERO(&rds);
        FD_SET(fd_server, &rds);
        if (sess)
        {
            for (i = 0; i <= max_fd; i++)
            {
                if (sess[i])
                    FD_SET(i, &rds);
            }
        }

        int stat = select(max_fd, &rds, NULL, NULL, NULL);
        if (stat == -1)
        {
            perror("select");
            exit(4);
        }

        if (FD_ISSET(fd_server, &rds))
            accept_client(fd_server, &max_fd, &sess);

        if (sess)
        {
            for (i = 0; i <= max_fd; i++)
            {
                if (sess[i])
                {
                    if (FD_ISSET(i, &rds))
                    {
                        stat = session_handle(sess[i]);
                        if (max_fd < i)
                            max_fd = i;
                    }
                }
            }
        }
    }
    return 0;
}

void send_all(struct session *except);

void set_name(struct session *sess, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        if (sess->buf[i] == '\n')
            break;
    }
    sess->name = malloc(sizeof(char) * (1 + i));
    memcpy(sess->name, sess->buf, i);
    sess->name[i] = 0;
    memset(sess->name, 0, len);
    sess->buf_used = 0;
    sess->step = run_step;
}
void str_handle(struct session *sess, int len);
void end_session(struct session **sess);

int session_handle(struct session *sess)
{
    int fd = sess->fd;
    int rc = read(fd, sess->buf, BUFFERSIZE - sess->buf_used);
    sess->buf_used += rc;

    if (-1 == rc)
        sess->step = error_step;

    switch (sess->step)
    {
    case no_name_step:
	set_name(sess, rc);
        break;
    case run_step:
        break;
    case end_step:
        break;
    case error_step:
        break;
    }
    return 0;
}