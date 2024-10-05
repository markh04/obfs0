#define BACKLOG 100
#define BUF_SIZE 1024
#define KEY_SIZE 4
#define MIN_CONTENT_LENGTH 1024
#define MAX_CONTENT_LENGTH 2048

typedef enum {CLIENT = 0, SERVER = 1} app_mode_t;

struct proc_args {
	unsigned int listen_addr;
	unsigned int target_addr;
	int listen_port;
	int target_port;
	app_mode_t mode;
};
struct thread_args {
	unsigned int listen_addr;
	unsigned int target_addr;
	int listen_port;
	int target_port;
	app_mode_t mode;
	int in_socket;
	int out_socket;
};
struct protocol_args {
	char key[KEY_SIZE];
	int content_length;
};

void process_args(int argc, char * argv[], struct proc_args * args);
void die(int status, const char * msg);
void print_addr(unsigned int ip, int port);

void start_listening(struct proc_args *);
