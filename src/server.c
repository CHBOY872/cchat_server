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

#define BLOCKME "BLOCKME" /* keyword for blocking messages */

static char greetings_msg[] = "Welcome to the chat!\n"
                              "You don't have a nickname. "
                              "You will be marked as <>.\n"
                              "If you want to add/change a name,"
                              " type [name <your name>]\n";
static char name_change_msg[] = "usage: name <name>\n";

void server_write_to(int fd, const char *msg, int len)
{
    write(fd, msg, len);
}

struct server *server_init(int port)
{
    int i;
    int opt = 1;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
        return NULL;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    if (-1 == bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)))
        return NULL;
    if (-1 == listen(fd, MAX_LISTEN_COUNT))
        return NULL;
    struct server *serv = malloc(sizeof(struct server));
    serv->server_fd = fd;
    serv->sess_array = malloc(sizeof(struct session *) * INIT_SIZE_COUNT);
    serv->sess_array_size = INIT_SIZE_COUNT;
    for (i = 0; i < serv->sess_array_size; i++)
        serv->sess_array[i] = NULL;
    return serv;
}

struct session *session_init(int fd)
{
    struct session *sess = malloc(sizeof(struct session));
    memset(sess->buf, 0, BUFFERSIZE);
    sess->fd = fd;
    sess->buf_used = 0;
    sess->name = NULL;
    return sess;
}

void session_close(struct session **sess)
{
    close((*sess)->fd);
    if ((*sess)->name)
        free((*sess)->name);
    free(*sess);
    *sess = NULL;
}

void server_resize_array(struct server *serv, int fd)
{
    int i;
    int new_size = serv->sess_array_size;
    while (fd >= serv->sess_array_size)
        new_size *= 2;

    struct session **temp = malloc(sizeof(struct session *) * new_size);
    for (i = 0; i < new_size; i++)
        temp[i] = i < serv->sess_array_size ? serv->sess_array[i] : NULL;
    free(serv->sess_array);
    serv->sess_array = temp;
    serv->sess_array_size = new_size;
}

void server_accept_client(struct server *serv)
{
    struct sockaddr_in addr;
    socklen_t len = 0;
    int fd = accept(serv->server_fd, (struct sockaddr *)&addr, &len);
    if (-1 == fd)
        return;
    if (fd >= serv->sess_array_size)
        server_resize_array(serv, fd);
    serv->sess_array[fd] = session_init(fd);
    server_write_to(fd, greetings_msg, sizeof(greetings_msg));
}

char *find_word(const char *str)
{
    int i = -1;
    do
    {
        i++;
        if (str[i] == ' ' || !str[i])
        {
            char *word = malloc(sizeof(char) * (i + 1));
            memcpy(word, str, i);
            word[i] = 0;
            return word;
        }
    } while (str[i]);
    return NULL;
}

int session_check_is_name_set(struct session *sess,
                              const char *first_word,
                              const char *second_word)
{
    if (first_word)
    {
        if (!strcmp(first_word, "name"))
        {
            if (second_word)
            {
                if (sess->name)
                    free(sess->name);
                int len = strlen(second_word);
                sess->name = malloc(sizeof(char) * (len + 1));
                memcpy(sess->name, second_word, len);
                sess->name[len] = 0;
            }
            else
                server_write_to(sess->fd, name_change_msg,
                                sizeof(name_change_msg));
            return 1;
        }
    }
    return 0;
}

void server_send_all(const char *msg,
                     struct session *except,
                     struct server *serv)
{
    int msg_len = strlen(msg) + 1;
    int i;
    for (i = 0; i < serv->sess_array_size; i++)
    {
        if (serv->sess_array[i])
        {
            if (serv->sess_array[i] != except)
                server_write_to(i, msg, msg_len);
        }
    }
}

void session_message_handle(struct session *sess, struct server *serv,
                            char *msg)
{
    if (strstr(msg, BLOCKME))
        return;
    int i = 0;
    while (msg[i] == ' ' && msg[i])
        i++;
    char *first_word = msg + i;
    first_word = find_word(first_word);
    i += strlen(first_word);
    while (msg[i] == ' ' && msg[i])
        i++;
    char *second_word = msg + i;
    second_word = find_word(second_word);
    if (!session_check_is_name_set(sess, first_word, second_word))
    {
        int name_len = sess->name ? strlen(sess->name) : 0;
        int msg_len = strlen(msg);
        char *to_msg = malloc(sizeof(char) * (name_len + msg_len + 1 + 4));
        if (sess->name)
            sprintf(to_msg, "<%s> %s\n", sess->name, msg);
        else
            sprintf(to_msg, "<> %s\n", msg);
        server_send_all(to_msg, sess, serv);
        free(to_msg);
    }
    if (first_word)
        free(first_word);
    if (second_word)
        free(second_word);
}

void session_string_handle(struct session *sess, struct server *serv)
{
    int pos = -1, i;
    char *str;
    for (i = 0; i < sess->buf_used; i++)
    {
        if (sess->buf[i] == '\n')
        {
            pos = i;
            break;
        }
    }
    if (pos == -1)
        return;

    str = malloc(sizeof(char) * pos);
    memcpy(str, sess->buf, pos);
    if (str[pos - 1] == '\r')
        str[pos - 1] = 0;
    session_message_handle(sess, serv, str);
    free(str);
}

int session_handle(struct session *sess, struct server *serv)
{
    int rc, fd = sess->fd, buf_used = sess->buf_used;
    rc = read(fd, sess->buf, BUFFERSIZE - buf_used);
    if (rc <= 0)
        return -1;
    sess->buf_used += rc;
    if (sess->buf_used >= BUFFERSIZE)
        return -1;

    session_string_handle(sess, serv);

    memset(sess->buf, 0, sess->buf_used);
    sess->buf_used = 0;
    return 0;
}

int server_run(struct server *serv)
{
    fd_set rds;
    int i;
    int max_fd;
    int stat;
    for (;;)
    {
        FD_ZERO(&rds);
        FD_SET(serv->server_fd, &rds);
        max_fd = serv->server_fd;
        for (i = 0; i < serv->sess_array_size; i++)
        {
            if (serv->sess_array[i])
            {
                FD_SET(i, &rds);
                if (i > max_fd)
                    max_fd = i;
            }
        }
        stat = select(max_fd + 1, &rds, NULL, NULL, NULL);
        if (stat == -1)
        {
            perror("select");
            return 1;
        }
        if (FD_ISSET(serv->server_fd, &rds))
            server_accept_client(serv);
        for (i = 0; i < serv->sess_array_size; i++)
        {
            if (serv->sess_array[i])
            {
                if (FD_ISSET(i, &rds))
                {
                    if (-1 == session_handle(serv->sess_array[i], serv))
                        session_close(&(serv->sess_array[i]));
                }
            }
        }
    }
    return 0;
}