#ifndef NET_UTILS_H_
#define NET_UTILS_H_

#include <stdlib.h>

#define UDP_PROT_NUMBER 17

int open_socket();

size_t prepare_udp(char *buffer, char *data_string, size_t data_size,
                   u_int32_t source_ip, u_int32_t dest_ip, u_int16_t source_port,
                   u_int16_t dest_port);

void send_packet(int socket_fd, u_int32_t dest_ip, char *packet, size_t tot_len);

#endif // NET_UTILS_H_
