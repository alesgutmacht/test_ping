#ifndef PING_H
#define PING_H

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

int run_ping(char *addr_ip);
unsigned short get_checksum(void *hdr, int len);
int set_address_ip(char *addr_ip);
void close_socs();

#endif // PING_H
