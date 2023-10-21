#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define BUF_SIZE 4096

void *send_worker(void *);
void *receive_worker(void *);


typedef struct {
    int sockfd;
    struct sockaddr_in addr;
} worker_arguments;