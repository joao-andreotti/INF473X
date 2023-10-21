#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define BUF_SIZE 4096

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
    struct sockaddr_in src_addr;
    socklen_t src_addr_size = sizeof(src_addr);
    memset(&src_addr, 0, sizeof(src_addr));

    char *msgbuf = (char *) malloc(BUF_SIZE);

    while (msgbuf != NULL) {
        size_t recvd_bytes = recvfrom(sockfd, (void *) msgbuf, BUF_SIZE, 0, (struct sockaddr *) &src_addr, &src_addr_size);
        sendto(sockfd, (void *) msgbuf, recvd_bytes, 0, (struct sockaddr *) &src_addr, src_addr_size);
    }

    // closes socket
    close(sockfd);
    exit(0);
}