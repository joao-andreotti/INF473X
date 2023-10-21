#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fancyclient.h"

int main(int argc, char *argv[]) {
    // checks if all arguments were passed to function
    if (argc < 3) {
        printf("Usage: %s IP_ADDRESS PORT_NUMBER\n", argv[0]);
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

    if (inet_pton(AF_INET, argv[1], &addr.sin_addr) != 1) {
        printf("Failed to parse host address.\n");
        exit(-1);
    }

    addr.sin_port = htons(atoi(argv[2]));

    // prepares arguments
    worker_arguments args = {.sockfd = sockfd, .addr = addr};

    // creates worker threads
    pthread_t sender, receiver;
    pthread_create(&sender, NULL, send_worker, (void *) &args);
    pthread_create(&receiver, NULL, receive_worker, (void *) &args);

    // joins sender thread (which can return)
    pthread_join(sender, NULL);

    // closes socket
    close(sockfd);
    exit(0);
}


void *send_worker(void *args) {
    worker_arguments *wa = (worker_arguments *) args;

    char *msgbuf = (char *) malloc(sizeof(char) * (BUF_SIZE + 1));
    msgbuf[BUF_SIZE] = '\x00'; // will never get touched by any functions (strlen always works)

    while ((msgbuf = fgets(msgbuf, BUF_SIZE, stdin)) != NULL) {
        sendto(wa->sockfd, (void *) msgbuf, strlen(msgbuf) + 1, 0, (struct sockaddr *) &wa->addr, sizeof(wa->addr));
    }

    return NULL;
}

void *receive_worker(void *args) {
    worker_arguments *wa = (worker_arguments *) args;

    char *msgbuf = (char *) malloc(sizeof(char) * (BUF_SIZE + 1));
    msgbuf[BUF_SIZE] = '\x00'; // will never get touched by any functions (strlen always works)

    for (;;) {
        recv(wa->sockfd, (void *) msgbuf, BUF_SIZE, 0);

        printf("%s", msgbuf);
    }
    
    return NULL;
}
