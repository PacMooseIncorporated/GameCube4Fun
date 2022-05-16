#ifndef __BBA_DEBUG_H
#define __BBA_DEBUG_H

// devoptab only
int enable_bba_traces();

// custom
int configure_bba();
int open_udp_socket(int port, char * server_ip);
void close_udp_socket();
int bba_printf(const char *fmt, ...);

// UDP server
#define TRACE_PORT 10000
#define TRACE_IP "192.168.1.53"

#endif /* __BBA_DEBUG_H */