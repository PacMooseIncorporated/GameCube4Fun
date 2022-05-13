/*
 * gclink: a network bootloader for the GameCube
 *
 * Copyright (c) 2022 TRSi
 * Written by:
 *	shazz <shazz@trsi.de>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include <ogc/lwp_threads.h>
#include <unistd.h>

#include "exi.h"
#include "sidestep.h"

// --------------------------------------------------------------------------------
//  DEFINES
//---------------------------------------------------------------------------------
#define PORT 2323
#define BUFFER_SIZE 1460		// MTU size I guess

// from system.h
// SYS_RESTART	0			/*!< Reboot the gamecube, force, if necessary, to boot the IPL menu. Cold reset is issued */
// SYS_HOTRESET 1			/*!< Restart the application. Kind of softreset */
// SYS_SHUTDOWN 2			/*!< Shutdown the thread system, card management system etc. Leave current thread running and return to caller */

// --------------------------------------------------------------------------------
//  DECLARATIONS
//---------------------------------------------------------------------------------
void * init_screen();
int setup_network_thread();
void * gclink(void *arg);
int parse_command(int csock, char * packet);

// commands
int execute_dol(int csock, int size);
int execute_elf(int csock, int size);
int say_hello(int csock);
int reset(int syscode);

// --------------------------------------------------------------------------------
//  GLOBALS
//---------------------------------------------------------------------------------
void * framebuffer = NULL;
static lwp_t gclink_handle = (lwp_t)NULL;

const static char gclink_exec_dol[] = "EXECDOL";
const static char gclink_exec_elf[] = "EXECELF";
const static char gclink_hello[] = "HELLO";
const static char gclink_reset[] = "RESET";

// stub .s sourced file
extern u8 stub_bin[];
extern u32 stub_bin_size;

//---------------------------------------------------------------------------------
// init
//---------------------------------------------------------------------------------
void * init_screen() {

	GXRModeObj *rmode;

	VIDEO_Init();
	PAD_Init();

	switch(VIDEO_GetCurrentTvMode()) {
		case VI_NTSC:
			rmode = &TVNtsc480IntDf;
			break;
		case VI_PAL:
			rmode = &TVPal528IntDf;
			break;
		case VI_EURGB60:
		case VI_MPAL:
			rmode = &TVMpal480IntDf;
			break;
		default:
			rmode = &TVNtsc480IntDf;
	}

	// rmode = VIDEO_GetPreferredMode(NULL);
	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	CON_Init(framebuffer, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	CON_EnableGecko(1, true);

	kprintf("USB Gecko traces enabled\n");

	VIDEO_Configure(rmode);
	VIDEO_ClearFrameBuffer (rmode, framebuffer, COLOR_BLACK);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// No stack - we need it all
	AR_Init(NULL, 0);

	// allow 32mhz exi bus
	*(volatile unsigned long*)0xcc00643c = 0x00000000;
	ipl_set_config(6);

	return 0;
}

//---------------------------------------------------------------------------------
// Routine to copy Stub reloader in memory
//---------------------------------------------------------------------------------
int installStub() {

	printf("Installing reload stub\n");
	void * dst = (void *) 0x80001800;

	// check it doesn't overload
	if ( ((u32)dst + stub_bin_size) > 0x80003000) {
		printf("The Reload stub size is too big: %d bytes, end address: %08X\n", stub_bin_size, ((u32)dst + stub_bin_size));
		return 1;
	}

	// Copy LoaderStub to 0x80001800
	printf("Copying reload stub to memory\n");
	memcpy(dst, stub_bin, stub_bin_size);

	printf("Invalidate caches\n");
	// Flush and invalidate cache
	DCFlushRange(dst, stub_bin_size);
	ICInvalidateRange(dst, stub_bin_size);

	return 0;
}

//---------------------------------------------------------------------------------
// Main
//---------------------------------------------------------------------------------
int main() {

	init_screen();

	printf("\n\ngclink server by Shazz/TRSi\n");

	if(exi_bba_exists()) {
		setup_network_thread();
	}
	else {
        printf("Broadband Adapter not found!\n");
    }

	printf("Press START reset the GameCube\n");

	while(1) {
		VIDEO_WaitVSync();
		PAD_ScanPads();

		if(PAD_ButtonsDown(0) & PAD_BUTTON_START) {
			printf("Reset!!!\n");
			reset(SYS_RESTART);
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

	int mram_size = (SYS_GetArenaHi()-SYS_GetArenaLo());
	int aram_size = (AR_GetSize()-AR_GetBaseAddress());

	printf("Memory Available: [MRAM] %i KB [ARAM] %i KB\n",(mram_size/1024), (aram_size/1024));
	printf("Largest DOL possible: %i KB\n", mram_size < aram_size ? mram_size/1024:aram_size/1024);

	printf("Configuring network...\n");

	// Configure the network interface (libogc2 doesn't use timeout)
	ret = if_config( localip, netmask, gateway, TRUE);
	if (ret>=0) {
		printf("Network configured IP: %s, GW: %s, MASK: %s\n", localip, gateway, netmask);

		LWP_CreateThread(	&gclink_handle,	/* thread handle */
							gclink,			/* code */
							localip,		/* arg pointer for thread */
							NULL,			/* stack base */
							16*1024,		/* stack size */
							50				/* thread priority */ );

		printf("Server is ready to receive commands!\n");
	}
	else {
		printf("Network configuration failed!\n");
		return 1;
	}

	return 0;
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
		reset(SYS_RESTART);

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
int reset(int syscode) {

	printf("Reset with syscode=%d\n", syscode);

	// while finding out how to patch DOL...
	exit(0);

	//TODO: probably kill a few things here!
	framebuffer = NULL;
	ARAMClear();
	GX_AbortFrame();

	u32 bi2Addr   = *(volatile u32*)0x800000F4;
	u32 osctxphys = *(volatile u32*)0x800000C0;
	u32 osctxvirt = *(volatile u32*)0x800000D4;

	// This doesnt look to work as expected...
	SYS_ResetSystem(syscode, 0, 0);

	*(volatile u32*)0x800000F4 = bi2Addr;
	*(volatile u32*)0x800000C0 = osctxphys;
	*(volatile u32*)0x800000D4 = osctxvirt;

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

	// create a buffer and a pointer to the position in this buffer
    u8 * data = (u8*) memalign(32, size);
    u8 * data_ptr = data;
	int ret;

	if(!data) {
		printf("Failed to allocate memory!!\n");
		return 1;
	}

	printf("Receiving file per packet of %i bytes:", BUFFER_SIZE);
	while(size > BUFFER_SIZE) {
		ret = net_recv(csock, data_ptr, BUFFER_SIZE, 0);
		size -= BUFFER_SIZE;
		data_ptr += BUFFER_SIZE;
		printf(".");
	}

	// last packet
	if(size) {
		ret = net_recv(csock, data_ptr, size, 0);
	}
	printf(". Done!\n");

	// install stub
	printf("Installing stub\n");
	ret = installStub();
	if (ret == 1) {
		printf("Could not copy the stub loader in RAM\n");
	}

	// Check DOL header
	DOLHEADER *dolhdr = (DOLHEADER*) data;
	printf("DOL Load address: %08X\n", dolhdr->textAddress[0]);
	printf("DOL Entrypoint: %08X\n", dolhdr->entryPoint);
	printf("BSS: %08X Size: %iKB\n", dolhdr->bssAddress, (int)((float)dolhdr->bssLength/1024));

	// Copy DOL to ARAM then execute
	ret = DOLtoARAM(data, 0, NULL);

    return ret;
}

//---------------------------------------------------------------------------------
// ELF execute command
//---------------------------------------------------------------------------------
int execute_elf(int csock, int size) {

 	// create a buffer and a pointer to the position in this buffer
    u8 * data = (u8*) memalign(32, size);
    u8 * data_ptr = data;
	int ret;

	if(!data) {
		printf("Failed to allocate memory!!\n");
		return 1;
	}

	printf("Receiving file per packet of %i bytes:", BUFFER_SIZE);
	while(size > BUFFER_SIZE) {
		ret = net_recv(csock, data_ptr, BUFFER_SIZE, 0);
		size -= BUFFER_SIZE;
		data_ptr += BUFFER_SIZE;
		printf(".");
	}

	// last packet
	if(size) {
		ret = net_recv(csock, data_ptr, size, 0);
	}
	printf(". Done!\n");

	// install stub
	ret = installStub();
	if (ret == 1) {
		printf("Could not copy the stub loader in RAM\n");
	}

	// Copy ELF to ARAM then execute
	ret = ELFtoARAM(data, 0, NULL);

    return ret;
}

