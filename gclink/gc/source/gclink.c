/*
 * gclink: a network bootloader for the GameCube
 *
 * Copyright (c) 2020 TRSi
 * Written by:
 *	shazz <shazz@trsi.de>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include <ogc/lwp_threads.h>

#include "exi.h"
#include "sidestep.h"

// --------------------------------------------------------------------------------
//  DECLARATIONS
//---------------------------------------------------------------------------------
void * init_console();
int setup_network_thread();
void * gclink(void *arg);
int parse_command(int csock, char * packet);

// commands
int execute_dol(int csock, int size);
int execute_elf(int csock, int size);
int say_hello(int csock);
int reset(bool shutdown);

// --------------------------------------------------------------------------------
//  GLOBALS
//---------------------------------------------------------------------------------
void * framebuffer = NULL;
static lwp_t gclink_handle = (lwp_t)NULL;

int PORT = 2323;
bool SHUTDOWN = false;
const static char gclink_exec_dol[] = "EXECDOL";
const static char gclink_exec_elf[] = "EXECELF";
const static char gclink_hello[] = "HELLO";
const static char gclink_reset[] = "RESET";


//---------------------------------------------------------------------------------
// init
//---------------------------------------------------------------------------------
void * init_console() {

	GXRModeObj *rmode;

	VIDEO_Init();
	PAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	console_init(framebuffer,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	VIDEO_Configure(rmode);
	VIDEO_ClearFrameBuffer (rmode, framebuffer, COLOR_BLACK);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	return 0;
}

//---------------------------------------------------------------------------------
// Main
//---------------------------------------------------------------------------------
int main() {

	init_console();

	if(exi_bba_exists()) {
		setup_network_thread();
	}
	else {
        printf("Broadband Adapter not found!\n");
		printf("Press START to reset!\n");
    }

	while(1) {
		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);

		if(buttonsDown & PAD_BUTTON_START) {
			reset(SHUTDOWN);
		}
	}

	return 0;
}

//---------------------------------------------------------------------------------
// Create thread to listen on the BBA interface
//---------------------------------------------------------------------------------
int setup_network_thread() {
	s32 ret;

	char localip[16] = {0};
	char gateway[16] = {0};
	char netmask[16] = {0};

	printf("\ngclink by Shazz/TRSi\n");
	printf("Configuring network...\n");

	// Configure the network interface
	ret = if_config( localip, netmask, gateway, TRUE, 20);
	if (ret>=0) {
		printf("Network configured IP: %s, GW: %s, MASK: %s\n", localip, gateway, netmask);

		LWP_CreateThread(	&gclink_handle,	/* thread handle */
							gclink,			/* code */
							localip,		/* arg pointer for thread */
							NULL,			/* stack base */
							16*1024,		/* stack size */
							50				/* thread priority */ );
	}
	else {
		printf("Network configuration failed!\n");
		printf("Press START to exit\n");
	}

	while(1) {

		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);

		if(buttonsDown & PAD_BUTTON_START) {
			printf("Bye bye!!!\n");
			exit(0);
		}
		else if((buttonsDown & PAD_BUTTON_A) && (buttonsDown & PAD_BUTTON_B) && (buttonsDown & PAD_TRIGGER_R) && (buttonsDown & PAD_TRIGGER_L)) {
			printf("Reset!!!\n");
			reset(SHUTDOWN);
		}
	}
}

//---------------------------------------------------------------------------------
// Thread entry point
//---------------------------------------------------------------------------------
void * gclink(void *arg) {

	int sock, csock;
	int ret;
	u32	clientlen;
	struct sockaddr_in client;
	struct sockaddr_in server;
	char temp[1026];

	clientlen = sizeof(client);

	sock = net_socket (AF_INET, SOCK_STREAM, IPPROTO_IP);

	if(sock == INVALID_SOCKET) {
    	printf ("Cannot create a socket!\n");
    }
	else {

		memset(&server, 0, sizeof (server));
		memset(&client, 0, sizeof (client));

		server.sin_family = AF_INET;
		server.sin_port = htons (PORT);
		server.sin_addr.s_addr = INADDR_ANY;
		ret = net_bind (sock, (struct sockaddr *) &server, sizeof (server));

		if(ret) {
			printf("Error %d binding socket!\n", ret);
		}
		else {

			if((ret = net_listen(sock, 5))) {
				printf("Error %d listening!\n", ret);
			}
			else {

				while(1) {

					csock = net_accept (sock, (struct sockaddr *) &client, &clientlen);

					if (csock < 0) {
						printf("Error connecting socket %d!\n", csock);
						while(1);
					}

					printf("Connecting port %d from %s\n", client.sin_port, inet_ntoa(client.sin_addr));
					memset (temp, 0, 1026);
					ret = net_recv(csock, temp, 1024, 0);
					printf("Received %d bytes\n", ret);

                    ret = parse_command(csock, temp);
					if (!ret) {
						printf("Commmand failed!");
					}

					net_close (csock);
				}
			}
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------------
// Command Parser
//---------------------------------------------------------------------------------
int parse_command(int csock, char * packet) {

    char response[200];

	printf("Parsing command\n");

    if(!strncmp(packet, gclink_exec_dol, strlen(gclink_exec_dol))) {
		char filename[50];
        int size = 0;
		char * token;

		token = strtok(&packet[strlen(gclink_exec_dol)], " ");
		if(token != NULL) {
			sprintf(filename, token);
			token = strtok(NULL, " ");
			if(token != NULL) {
				size = atoi(token);
			}
		}
		else {
			sprintf(response, "Incomplete %s command, missing filename", gclink_exec_dol);
			net_send(csock, response, strlen(response), 0);
			return 1;
		}

		if(size != 0) {
			printf("Executing DOL of size %d\n", size);

			sprintf(response, "OK");
			net_send(csock, response, strlen(response), 0);

			execute_dol(csock, size);

			sprintf(response, "DOL executed");
			net_send(csock, response, strlen(response), 0);
		}
		else {
			sprintf(response, "Incomplete %s command, missing size", gclink_exec_dol);
			net_send(csock, response, strlen(response), 0);
			return 1;
		}

    }
    else if(!strncmp(packet, gclink_exec_elf, strlen(gclink_exec_elf))) {
		char filename[50];
        int size = 0;
		char * token;

		token = strtok(&packet[strlen(gclink_exec_elf)], " ");
		if(token != NULL) {
			sprintf(filename, token);
			token = strtok(NULL, " ");
			if(token != NULL) {
				size = atoi(token);
			}
		}
		else {
			sprintf(response, "Incomplete %s command, missing filename", gclink_exec_dol);
			net_send(csock, response, strlen(response), 0);
			return 1;
		}

		if(size != 0) {

			sprintf(response, "OK");
			net_send(csock, response, strlen(response), 0);

			printf("Executing ELF of size %d\n", size);
			execute_elf(csock, size);

			sprintf(response, "ELF executed");
			net_send(csock, response, strlen(response), 0);
		}
		else {
			sprintf(response, "Incomplete %s command, missing size", gclink_exec_elf);
			net_send(csock, response, strlen(response), 0);
			return 1;
		}
    }
	else if(!strncmp(packet, gclink_hello, strlen(gclink_hello))) {

		printf("Saying hello\n");
		say_hello(csock);

        sprintf(response, "HELLO said!");
		net_send(csock, response, strlen(response), 0);
    }
	else if(!strncmp(packet, gclink_reset, strlen(gclink_reset))) {

		printf("Resetting gclink!\n");
		reset(SHUTDOWN);

        sprintf(response, "Reset requested!");
		net_send(csock, response, strlen(response), 0);
    }
    else {
        sprintf(response, "Unknown command");
        net_send(csock, response, strlen(response), 0);
		return 1;
    }

    return 0;
}

//---------------------------------------------------------------------------------
// RESET command
//---------------------------------------------------------------------------------
int reset(bool shutdown) {

	printf("Reset!\n");

	//TODO: probably kill a few things here!
	framebuffer = NULL;
	ARAMClear();
	GX_AbortFrame();

	if(shutdown) {
		u32 bi2Addr = *(volatile u32*)0x800000F4;
		u32 osctxphys = *(volatile u32*)0x800000C0;
		u32 osctxvirt = *(volatile u32*)0x800000D4;

		// This doesnt look to work as expected...
		// #define SYS_RESTART					0			/*!< Reboot the gamecube, force, if necessary, to boot the IPL menu. Cold reset is issued */
		// #define SYS_HOTRESET					1			/*!< Restart the application. Kind of softreset */
		// #define SYS_SHUTDOWN					2			/*!< Shutdown the thread system, card management system etc. Leave current thread running and return to caller */
		SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

		*(volatile u32*)0x800000F4 = bi2Addr;
		*(volatile u32*)0x800000C0 = osctxphys;
		*(volatile u32*)0x800000D4 = osctxvirt;
	}

	// restore thread
	__lwp_thread_stopmultitasking((void(*)())main());

    return 0;
}

//---------------------------------------------------------------------------------
// HELLO command
//---------------------------------------------------------------------------------
int say_hello(int csock) {

	char * response = "OLLEH";
	net_send(csock, response, strlen(response), 0);

    return 0;
}

//---------------------------------------------------------------------------------
// DOL execute command
//---------------------------------------------------------------------------------
int execute_dol(int csock, int size) {

    u8 * data = (u8*) memalign(32, size);
    int ret;

	// receive the DOL payload of the given size
    ret = net_recv(csock, data, size, 0);
    printf("Received %d bytes of DOL payload\n", ret);

	if(ret == size) {
		ret = DOLtoARAM(data, 0, NULL);
	}
	else {
		printf("The payload size doesn't match: %d vs %d\n", size, ret)
		return 1;
	}

    return ret;
}

//---------------------------------------------------------------------------------
// ELF execute command
//---------------------------------------------------------------------------------
int execute_elf(int csock, int size) {

    u8 * data = (u8*) memalign(32, size);
    int ret;

	// receive the ELF payload of the given size
    ret = net_recv(csock, data, size, 0);
    printf("Received %d bytes of ELF payload\n", ret);

	if(ret == size) {
		ret = ELFtoARAM(data, 0, NULL);
	}
	else {
		printf("The payload size doesn't match: %d vs %d\n", size, ret)
		return 1;
	}

    return ret;
}