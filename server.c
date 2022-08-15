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

static char greetings_msg[] = "Welcome to the chat! Enter your name\n";
static char name_change_msg[] = "usage: name <name> (note: only one space)"
                                " between command and argument\n";

void close_server(struct session ***sess, int len)
{
    int i;
    for (i = 0; i <= len; i++)
    {
        if ((*sess)[i])
            end_session(&(*sess)[i]);
    }
    free(*sess);
}

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

struct session *make_session(int fd)
{
    struct session *sess = malloc(sizeof(struct session));
    sess->fd = fd;
    sess->name = NULL;
    sess->step = no_name_step;
    sess->buf_used = 0;
    memset(sess->buf, 0, BUFFERSIZE);
    return sess;
}

void accept_client(int server_fd, int *max_fd, struct session ***sess)
{ /* accept connection to server */
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
        *sess = malloc(sizeof(struct session *) * (*max_fd + 1));
        for (i = 0; i < *max_fd; i++)
            (*sess)[i] = NULL;
    }
    if (*max_fd < client_fd)
    {
        struct session **sess_tmp = NULL;
        sess_tmp =
            malloc(sizeof(struct session *) * (client_fd + 1));
        for (i = 0; i <= client_fd; i++)
            sess_tmp[i] = i <= *max_fd ? (*sess)[i] : NULL;
        *max_fd = client_fd;
        free(*sess);
        *sess = sess_tmp;
    }
    (*sess)[client_fd] = make_session(client_fd);
    send_msg(client_fd, greetings_msg, sizeof(greetings_msg));
}

int run(int fd_server) /* main server cycle */
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

        int stat = select(max_fd + 1, &rds, NULL, NULL, NULL);
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
                        if (-1 == session_handle(&sess[i], sess, max_fd + 1))
                            find_max_descriptor(sess, &max_fd);
                    }
                }
            }
        }
    }
    close_server(&sess, max_fd);
    return 0;
}

void send_msg(int fd, const char *msg, int size)
{
    write(fd, msg, size);
}

void send_all(struct session *except, struct session **sess,
              int count, const char *msg, int msg_len)
{ /* send messages to all */
    int i;
    for (i = 0; i < count; i++)
    {
        if (sess[i])
        { /* if address of session not equal to except sess address */
            /* send message */
            if (except != sess[i])
                send_msg(sess[i]->fd, msg, msg_len);
        }
    }
}

void set_name(struct session *sess, const char *str, int str_len)
{
    if (sess->name)
        free(sess->name);
    sess->name = malloc(sizeof(char) * (str_len + 1));
    memcpy(sess->name, str, str_len);
    sess->name[str_len] = 0;
    memset(sess->buf, 0, str_len);
    sess->buf_used = 0;
    sess->step = run_step;
}

void handle(struct session *sess, const char *msg,
            int str_len, struct session **sessions, int count)
{
    if (strstr(msg, "BLOCKME")) /* if msg contains BLOCKME */
    {
        memset(sess->buf, 0, sess->buf_used);
        sess->buf_used = 0;
        return;
    }
    int i, left_str_len;
    for (i = 0, left_str_len = str_len; i < str_len; i++, left_str_len--)
    { /* find first word in msg */
        if (msg[i] == ' ')
            break;
    }
    char *first_word = malloc(sizeof(char) * (i + 1));
    memcpy(first_word, msg, i);
    first_word[i] = 0;
    if (!strcmp(first_word, "name")) /* if first word = "name" */
    {
        const char *ptr = msg + i + 1;

        for (i = 0; i < left_str_len && *(ptr + i) && *(ptr + i) != ' '; i++)
            ; /* find second word */

        if (!i) /* there is not any word which would be a name */
        {
            send_msg(sess->fd, name_change_msg, sizeof(name_change_msg));
            free(first_word);
            return;
        }

        char *name = malloc(i + 1);
        memcpy(name, ptr, i + 1);
        name[i] = 0;

        set_name(sess, name, i);
        free(name);
        free(first_word);
        return;
    }
    int name_len = strlen(sess->name);
    int full_msg_len = sizeof(char) * (name_len + 3 + str_len + 2);
    /* template: <name> text..... 2 for brackets,
    1 for space, 1 for '\n', 1 for endline (0) */
    char *full_msg = malloc(full_msg_len);
    sprintf(full_msg, "<%s> %s\n", sess->name, msg);
    send_all(sess, sessions, count, full_msg, full_msg_len);
    free(full_msg);
    free(first_word);
    memset(sess->buf, 0, sess->buf_used);
    sess->buf_used = 0;
}

void find_max_descriptor(struct session **sess, int *max_fd)
{
    int i;
    int len = *max_fd;
    for (i = 0; i <= len; i++)
    {
        if (sess[i])
        {
            if (*max_fd < i)
                *max_fd = i;
        }
    }
}

void end_session(struct session **sess) /* delete session */
{
    if ((*sess)->name)
        free((*sess)->name);

    close((*sess)->fd);
    free(*sess);
    *sess = NULL;
}

int session_handle(struct session **sess, struct session **sessions, int size)
{
    int fd = (*sess)->fd;
    int buf_used = (*sess)->buf_used;
    int rc = read(fd, (*sess)->buf, BUFFERSIZE - (*sess)->buf_used);
    int i;
    (*sess)->buf_used += rc;

    if (-1 == rc)
    {
        (*sess)->step = error_step;
        end_session(sess);
        return -1;
    }

    if (!rc)
    {
        (*sess)->step = end_step;
        end_session(sess);
        return -1;
    }

    for (i = 0; i < rc; i++)
    {
        if ((*sess)->buf[i + buf_used] == '\n' ||
            (*sess)->buf[i + buf_used] == '\r')
            break;
    }

    int str_len = i;
    char *str = malloc(sizeof(char) * (str_len + 1));
    memcpy(str, (*sess)->buf, str_len);
    str[str_len] = 0;

    switch ((*sess)->step)
    {
    case no_name_step:
        set_name(*sess, str, str_len);
        break;
    case run_step:
        handle(*sess, str, str_len, sessions, size);
        break;
    case end_step:
        end_session(sess);
        return -1;
    case error_step:
        break;
    }

    free(str);
    return 0;
}