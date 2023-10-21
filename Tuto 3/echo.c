#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>

#include "echo.h"

size_t active_threads = 0;
pthread_mutex_t active_threads_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
    // checks if all arguments were passed to function
    if (argc < 2) {
        printf("Usage: %s PORT_NUMBER\n", argv[0]);
        exit(-1);
    }

    // creates socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed.\n");
        exit(-1);
    }

    // parses address and port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) != 0) {
        printf("Failed to bind socket.\n");
        exit(-1);
    }

    // read-echo loop
    for (;;) {
        // allocates memory
        char *msgbuf = (char *) malloc(BUF_SIZE + 1);
        msgbuf[BUF_SIZE] = '\x00';

        struct sockaddr_in *src_addr = malloc(sizeof(struct sockaddr_in));
        memset(src_addr, 0, sizeof(struct sockaddr_in));
        
        // reads input and prepares struct to be passed to worker
        socklen_t src_addr_size = sizeof(struct sockaddr_in);
        size_t rcvd_bytes = recvfrom(sockfd, (void *) msgbuf, BUF_SIZE, 0, (struct sockaddr *) src_addr, &src_addr_size);

        answer_data *ad = malloc(sizeof(answer_data));
        ad->sockfd = sockfd;
        ad->addr = src_addr;
        ad->addr_size = src_addr_size;
        ad->msgbuf = msgbuf;
        ad->msg_size = rcvd_bytes;

        // waits until the thread cap is not saturated
        for (;;) {
            if (pthread_mutex_lock(&active_threads_mutex)) {
                printf("Failed locking mutex.\n");
                exit(-1);
            }

            if (active_threads < MAX_THREADS)
                break;

            sched_yield();

            if (pthread_mutex_unlock(&active_threads_mutex)) {
                printf("Failed unlocking mutex.\n");
                exit(-1);
            }
        }


        // spawns thread to respond to message
        pthread_t response_thread;
        pthread_create(&response_thread, NULL, answer_worker, (void *) ad);  
        active_threads++;

        if (pthread_mutex_unlock(&active_threads_mutex)) {
            printf("Failed unlocking mutex.\n");
            exit(-1);
        }

    }

    // closes socket
    close(sockfd);
    exit(0);
}


void *answer_worker(void *args) {
    answer_data *data = (answer_data *) args;
    
    // echoes data back to sender
    sendto(data->sockfd, (void *) data->msgbuf, data->msg_size, 0, (struct sockaddr *) data->addr, data->addr_size);

    // frees memory
    free(data->addr);
    free(data->msgbuf);
    free(data);

    // reduces number of current workers
    if (pthread_mutex_lock(&active_threads_mutex)) {
        printf("Failed locking mutex.\n");
        exit(-1);
    }

    active_threads--;

    if (pthread_mutex_unlock(&active_threads_mutex)) {
        printf("Failed unlocking mutex.\n");
        exit(-1);
    }

    return NULL;
}