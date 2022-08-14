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
void accept_client(int server_fd, int *max_fd, struct session ***sess);
int session_handle(struct session *sess);
int run(int fd_server);