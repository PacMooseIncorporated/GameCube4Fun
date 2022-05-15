#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <network.h>
#include <errno.h>
#include <ogcsys.h>

#include "exi.h"
#include "bba_debug.h"

// --------------------------------------------------------------------------------
//  GLOBALS
//---------------------------------------------------------------------------------
// socket
s32 sock = -1;

//---------------------------------------------------------------------------------
// Configure BBA
//---------------------------------------------------------------------------------
int configure_bba() {

	char localip[16] = {0};
	char gateway[16] = {0};
	char netmask[16] = {0};
	int ret = 1;

	// allow 32mhz exi bus
	*(volatile unsigned long*)0xcc00643c = 0x00000000;
	ipl_set_config(6);

	printf("Checking BBA\n");
	if(exi_bba_exists()) {

		printf("Configuring network...\n");
		ret = if_config( localip, netmask, gateway, TRUE);
		if(ret < 0) {
			printf("BBA network stack not configured!\n");
			return -1;
		}
		printf("Network configured IP: %s, GW: %s, MASK: %s\n", localip, gateway, netmask);
	}
	else {
		printf("BBA not found!!!!\n");
		return -1;
	}

	return 1;
}

//---------------------------------------------------------------------------------
// Create UDP Socket to Trace server
//---------------------------------------------------------------------------------
int open_udp_socket(int port, char * server_ip) {

	struct sockaddr_in address;

	if ((sock = net_socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		printf("open_udp_socket: socket: %d\n", errno);
		return -1;
	}

	// not needed
    // bool _true = true;
	// if (net_ioctl (sock, FIONBIO, (char *)&_true) < 0) {
	// 	printf("open_udp_socket: ioctl FIONBIO: %d\n", errno);
	// 	return -1;
	// }

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(server_ip);
	address.sin_port = htons((short)port);

    // not needed
	// printf("binding socket\n");
	// if(net_bind(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
	// 	printf("open_udp_socket: bind: %d\n", errno);
	// 	return -1;
	// }

	printf("connecting socket\n");
	if (net_connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
		printf("open_udp_socket: connect: %d\n", errno);
		return -1;
	}

    // tell the Trace server what we are connected :)
	bba_printf("Socket established!!\n");

	return 1;
}

//---------------------------------------------------------------------------------
// Close UDP Socket
//---------------------------------------------------------------------------------
void close_udp_socket() {
	net_close(sock);
}

//---------------------------------------------------------------------------------
// Print on the remote terminal
//---------------------------------------------------------------------------------
int bba_printf (const char *fmt, ...) {
	char temp[1026];
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = vsnprintf(temp, sizeof (temp), fmt, ap);
	va_end (ap);

	if (net_send(sock, temp, strlen (temp), 0) < 0) {
		printf("Cannot send UDP packet\n");
	}

	return rc;
}