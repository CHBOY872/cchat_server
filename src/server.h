#ifndef SERVER_H_SENTRY
#define SERVER_H_SENTRY

#define BUFFERSIZE 256
#define INIT_SIZE_COUNT 16

#define MAX_LISTEN_COUNT 10

struct session
{
    int fd;
    char buf[BUFFERSIZE];
    int buf_used;
    char *name;
};

struct server
{
    struct session **sess_array;
    int sess_array_size;
    int server_fd;
};

struct server *server_init(int port);
void server_accept_client(struct server *serv);
void server_resize_array(struct server *serv, int fd);
void server_write_to(int fd, const char *msg, int len);
int server_run(struct server *serv);

struct session *session_init(int fd);
void session_close(struct session **sess);
int session_handle(struct session *sess, struct server *serv);
void session_string_handle(struct session *sess, struct server *serv);
void session_message_handle(struct session *sess, struct server *serv,
                            char *msg);
int session_check_is_name_set(struct session *sess,
                              const char *first_word,
                              const char *second_word);
void server_send_all(const char *msg,
                     struct session *except,
                     struct server *serv);

char *find_word(const char *str);

#endif