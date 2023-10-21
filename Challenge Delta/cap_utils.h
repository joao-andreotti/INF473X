#ifndef CAP_UTILS_H_
#define CAP_UTILS_H_

#include <pcap.h>

#define BUF_SIZE 65536
#define N_PROTOCOLS 256

pcap_t *open_device(char *device_name);

typedef unsigned char u_char; 
typedef unsigned int u_int;

void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer);
void register_handler(u_char prot_number, void (*handler)(const u_char *, const struct pcap_pkthdr *));
void init_capture(pcap_t *handle);
void close_device(pcap_t *handle);
void process_packet(u_char */*args*/, const struct pcap_pkthdr */*header*/, const u_char *buffer);
void apply_packet_filter(pcap_t *handle, char *filter_exp, char *dev_name);

#endif // CAP_UTILS_H_