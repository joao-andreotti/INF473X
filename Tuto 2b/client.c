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

    // read-send loop
    char *msgbuf = (char *) malloc(sizeof(char) * (BUF_SIZE + 1));
    msgbuf[BUF_SIZE] = '\x00'; // will never get touched by any functions (strlen always works)

    while ((msgbuf = fgets(msgbuf, BUF_SIZE, stdin)) != NULL) {
        sendto(sockfd, (void *) msgbuf, strlen(msgbuf) + 1, 0, (struct sockaddr *) &addr, sizeof(addr));
    }

    // closes socket
    close(sockfd);
    exit(0);
}