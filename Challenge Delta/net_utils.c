#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "header.h"
#include "net_utils.h"

void send_packet(int socket_fd, u_int32_t dest_ip, char *packet, size_t tot_len) {
    struct sockaddr_in daddr;
    memset(&daddr, 0, sizeof(daddr));

    daddr.sin_family = AF_INET;
    daddr.sin_addr.s_addr = dest_ip;

	if (sendto(socket_fd, packet, tot_len, 0, (struct sockaddr *) &daddr, sizeof(daddr)) == -1) {
		perror("Failed to send packet");
		exit(-1);
	}
}

int open_socket() {
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    int hincl = 1; // 1 = on, 0 = off
    setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));

	if (fd < 0) {
		perror("Error creating raw socket ");
		exit(1);
	}

    return fd;
}

size_t prepare_udp(char *buffer, char *data_string, size_t data_size,
                 u_int32_t source_ip, u_int32_t dest_ip, u_int16_t source_port,
                 u_int16_t dest_port) {
	// Some of the following code takes inspiration from:
	// https://www.binarytides.com/raw-sockets-c-code-linux/
	//IP header pointer
	struct iphdr *iph = (struct iphdr *) buffer;

	//UDP header pointer
	struct udphdr *udph = (struct udphdr *)(buffer + sizeof(struct iphdr));

	//data section pointer
	char *data = buffer + sizeof(struct iphdr) + sizeof(struct udphdr);

	//fill the data section
	memcpy(data, data_string, data_size);

	//fill the IP header here
	assert(sizeof(struct iphdr) == 20);
	iph->version = 4; // IPv4
	iph->ihl = 5;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + data_size;
	iph->id = 0;
	iph->frag_off = 0;
	iph->ttl = 0xFF;
	iph->protocol = 17; // UDP

	// parses and sets addresses
	iph->saddr = source_ip;
	iph->daddr = dest_ip;

	// calculates IPv4 checksum
	iph->check = 0;
	iph->check = checksum((unsigned short *) iph, sizeof(struct iphdr));

	//fill the UDP header
	udph->source = htons(source_port);
	udph->dest = htons(dest_port);
	udph->len = htons(sizeof(struct udphdr) + data_size);
	
	// calculates UDP checksum
	struct pseudo_udp_header pshdr = {
		.source_address = iph->saddr,
		.dest_address = iph->daddr,
		.placeholder = 0x00,
		.protocol = iph->protocol,
		.udp_length = udph->len,
	};

	u_int16_t check_ps = checksum((unsigned short *) &pshdr, sizeof(struct pseudo_udp_header));

	unsigned long sum = 
		~check_ps + 1 +
		~checksum((unsigned short *) udph, sizeof(struct udphdr) + data_size) + 1;

	sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);

	udph->check = 0;//(short) ~sum;

    return iph->tot_len;
}