#define BUFFERSIZE 256
#define MAX_LISTEN_COUNT 16

enum steps
{
    no_name_step = 1,
    run_step,
    end_step,
    error_step
};

struct session
{
    int fd;
    char buf[BUFFERSIZE];
    int buf_used;
    char *name;
    enum steps step;
};

int server_create(int port);
int run(int fd_server);
void close_server(struct session ***sess, int len);

struct session *make_session(int fd);
void end_session(struct session **sess);

void accept_client(int server_fd, int *max_fd, struct session ***sess);

void send_msg(int fd, const char *msg, int size);
void send_all(struct session *except, struct session **sess,
              int count, const char *msg, int msg_len);

void find_max_descriptor(struct session **sess, int *max_fd);
void set_name(struct session *sess, const char *str, int str_len);
void handle(struct session *sess, const char *msg,
            int str_len, struct session **sessions, int count);
int session_handle(struct session **sess, struct session **sessions, int size);