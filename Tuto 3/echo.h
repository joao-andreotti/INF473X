#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define BUF_SIZE 4096
#define MAX_THREADS 2

void *answer_worker(void *);


typedef struct {
    int sockfd;
    struct sockaddr_in *addr;
    socklen_t addr_size;
    char *msgbuf;
    size_t msg_size;
} answer_data;