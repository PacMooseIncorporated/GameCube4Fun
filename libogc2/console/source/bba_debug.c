#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <network.h>
#include <errno.h>
#include <ogcsys.h>
#include <sys/iosupport.h>

#include "exi.h"
#include "bba_debug.h"

// --------------------------------------------------------------------------------
//  INTERNAL DECLARATIONS
//---------------------------------------------------------------------------------
int open_udp_socket(int port, char * server_ip);
int netudp_write(struct _reent *r, void *fd, const char *ptr, size_t len);

// --------------------------------------------------------------------------------
//  GLOBALS
//---------------------------------------------------------------------------------
s32 sock = -1;
bool devoptab_is_enabled = false;

//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
const devoptab_t dotab_stdudp = {
    "stdudp",     // device name
    0,            // size of file structure
    NULL,         // device open
    NULL,         // device close
    netudp_write, // device write
    NULL,         // device read
    NULL,         // device seek
    NULL,         // device fstat
    NULL,         // device stat
    NULL,         // device link
    NULL,         // device unlink
    NULL,         // device chdir
    NULL,         // device rename
    NULL,         // device mkdir
    0,            // dirStateSize
    NULL,         // device diropen_r
    NULL,         // device dirreset_r
    NULL,         // device dirnext_r
    NULL,         // device dirclose_r
    NULL,         // device statvfs_r
    NULL,         // device ftrunctate_r
    NULL,         // device fsync_r
    NULL,         // deviceData;
};

//---------------------------------------------------------------------------------
// Configure BBA
//---------------------------------------------------------------------------------
int setup_bba_logging(int port, char* ip_address, bool use_kprintf, bool use_printf) {

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

    printf("Creating socket\n");
    // assuming path is a string containing the IP address and not a hostname
    if (open_udp_socket(port, ip_address) < 0)
        return -1;

    if(use_printf || use_kprintf) {

        printf("Replacing devoptab_list entries for stdout/stderr\n");

        // update newlib devoptab_list with this devoptab
        if(use_printf)
            devoptab_list[STD_OUT] = &dotab_stdudp;
        if(use_kprintf)
            devoptab_list[STD_ERR] = &dotab_stdudp;

        devoptab_is_enabled = true;

        // Traces tests
        kprintf("[kprintf] BBA traces enabled\n");
        printf("[printf] BBA traces enabled\n");
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

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(server_ip);
	address.sin_port = htons((short)port);

	if (net_connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
		printf("open_udp_socket: connect: %d\n", errno);
		return -1;
	}

	return 1;
}

//---------------------------------------------------------------------------------
// Close UDP Socket
//---------------------------------------------------------------------------------
void close_bba_logging() {
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

//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
int netudp_write(struct _reent *r, void *fd, const char *ptr, size_t len)
{
    int ret;

    if (!devoptab_is_enabled)
        return -1;

    ret = bba_printf("%s", ptr);

    return ret;
}