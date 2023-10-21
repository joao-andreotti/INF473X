// A good part of the code below is taken from Tutorial 4. Originnaly written by 'jiaziyi'

#include <stdlib.h>
#include <stdint.h>

#include "header.h"
#include "cap_utils.h"


char err_buf[PCAP_ERRBUF_SIZE] = {'\x00'};
void (*handlers[N_PROTOCOLS])(const u_char *, const struct pcap_pkthdr *) = {NULL};

pcap_t *open_device(char *device_name) {
    pcap_init(0, err_buf);
    pcap_t *handle = pcap_open_live(device_name, BUF_SIZE, 1, 1, err_buf);

	if (handle == NULL) {
		fprintf(stderr, "Unable to open device %s: %s\n", device_name, err_buf);
		exit(1);
	}

	pcap_set_immediate_mode(handle, 1);

    return handle;
}

void register_handler(u_char prot_number, void (*handler)(const u_char *, const struct pcap_pkthdr *)) {
    handlers[prot_number] = handler;
}

void init_capture(pcap_t *handle) {
	for (;;)
    	pcap_dispatch(handle, 0, process_packet, NULL);
}

void close_device(pcap_t *handle) {
    pcap_close(handle);
}

void process_packet(u_char */*args*/, const struct pcap_pkthdr *header, const u_char *buffer) {
	// Get the IP Header part of this packet, excluding the ethernet header
	struct iphdr *iph = (struct iphdr*)(buffer + sizeof(struct ethhdr));

    // Calls correct handler
    if (handlers[iph->protocol] == NULL)
        return;
    
    handlers[iph->protocol](buffer + sizeof(struct ethhdr), header);
}

void apply_packet_filter(pcap_t *handle, char *filter_exp, char *dev_name) {
	bpf_u_int32 net_ip, mask;
    int ret = pcap_lookupnet(dev_name, &net_ip, &mask, err_buf);
	if (ret < 0) {
		fprintf(stderr, "Error looking up net: %s \n", dev_name);
		exit(-1);
	}

	struct bpf_program fp;
	if (pcap_compile(handle, &fp, filter_exp, 1, net_ip) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
		exit(-1);
	}

	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
		exit(-1);
	}
}