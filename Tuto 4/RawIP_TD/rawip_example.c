/*
 * rawip_example.c
 *
 *  Created on: May 4, 2016
 *      Author: jiaziyi
 */


#include<assert.h>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include "header.h"

#define SRC_IP  "129.104.221.133" //set your source ip here. It can be a fake one
#define SRC_PORT 23445 //set the source port here. It can be a fake one

// #define DEST_IP "129.104.221.133" //set your destination ip here
#define DEST_IP "129.104.252.43" //set your destination ip here
#define DEST_PORT 55000 //set the destination port here
#define TEST_STRING "I have a working implementation of a raw ip socket\x0A" //a test string as packet payload

int main(int argc, char *argv[])
{
	char source_ip[] = SRC_IP;
	char dest_ip[] = DEST_IP;

	int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    int hincl = 1;                  /* 1 = on, 0 = off */
    setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));

	if(fd < 0)
	{
		perror("Error creating raw socket ");
		exit(1);
	}

	char packet[65536], *data;
	char data_string[] = TEST_STRING;
	memset(packet, 0, 65536);

	// Some of the following code takes inspiration from:
	// https://www.binarytides.com/raw-sockets-c-code-linux/
	//IP header pointer
	struct iphdr *iph = (struct iphdr *) packet;

	//UDP header pointer
	struct udphdr *udph = (struct udphdr *)(packet + sizeof(struct iphdr));

	//data section pointer
	data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);

	//fill the data section
	strncpy(data, data_string, strlen(data_string));

	//fill the IP header here
	assert(sizeof(struct iphdr) == 20);
	iph->version = 4; // IPv4
	iph->ihl = 5;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data_string);
	iph->id = 0;
	iph->frag_off = 0;
	iph->ttl = 0xFF;
	iph->protocol = 17;

	// parses and sets addresses
	iph->saddr = inet_addr(source_ip);
	iph->daddr = inet_addr(dest_ip);

	// calculates IPv4 checksum
	iph->check = 0;
	iph->check = checksum((unsigned short *) iph, sizeof(struct iphdr));

	//fill the UDP header
	udph->source = htons(SRC_PORT);
	udph->dest = htons(DEST_PORT);
	udph->len = htons(sizeof(struct udphdr) + strlen(data_string));
	
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
		~checksum((unsigned short *) udph, sizeof(struct udphdr) + strlen(data)) + 1;

	sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);

	udph->check = (short) ~sum;

	//send the packet
	struct sockaddr_in daddr;
    memset(&daddr, 0, sizeof(daddr));

    daddr.sin_family = AF_INET;

    if (inet_pton(AF_INET, dest_ip, &daddr.sin_addr) != 1) {
        printf("Failed to parse destination address.\n");
        exit(-1);
    }

	if (sendto(fd, packet, iph->tot_len, 0, (struct sockaddr *) &daddr, sizeof(daddr)) == -1) {
		perror("Failed to send packet");
		exit(1);
	}

	return 0;
}
