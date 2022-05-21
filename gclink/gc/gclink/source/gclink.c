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
#include <ogc/usbgecko.h>
#include <unistd.h>

#include <fat.h>
#include <sdcard/gcsd.h>

#include "exi.h"
#include "sidestep.h"

// --------------------------------------------------------------------------------
//  DEFINES
//---------------------------------------------------------------------------------
#define USE_DHCP false
#define DEFAULT_IP "192.168.1.230"
#define DEFAULT_GTW "192.168.1.1"
#define DEFAULT_MASK "255.255.255.0"
#define PORT 2323
#define BUFFER_SIZE 1460		// MTU size I guess
#define GECKO_CHANNEL 1
#define PC_READY 0x80
#define PC_OK    0x81
#define GC_READY 0x88
#define GC_OK    0x89

#define SWISS_FILENAME "fat:/autoexec.dol"

// from system.h
// SYS_RESTART	0			/*!< Reboot the gamecube, force, if necessary, to boot the IPL menu. Cold reset is issued */
// SYS_HOTRESET 1			/*!< Restart the application. Kind of softreset */
// SYS_SHUTDOWN 2			/*!< Shutdown the thread system, card management system etc. Leave current thread running and return to caller */

// --------------------------------------------------------------------------------
//  DECLARATIONS
//---------------------------------------------------------------------------------
typedef struct
{
	char localip[16];
	char gateway[16];
	char netmask[16];
} t_bba_config;

typedef enum storage {
    SD2SP2, SDGECKO_A, SDGECKO_B
} storage_t;

void * init_screen();
int setup_network_thread();
int setup_gecko_thread();

void * gclink_bba(void *arg);
void * gclink_gecko(void *arg);

int parse_command(int csock, char * packet, char ** bba_config);
int run_executable(u8 * data, bool is_dol, char ** bba_config);
int install_stub();
int load_fat_file(const DISC_INTERFACE * iface, char * filename);
int copy_fat_file(const DISC_INTERFACE *iface, void * data, u32 size, char * filename);

// commands
int execute_dol(int csock, int size, char ** bba_config);
int execute_elf(int csock, int size, char ** bba_config);
int copy_dol(int csock, int size, char * filename, storage_t dest, char ** bba_config);
int say_hello(int csock);
int reset(int syscode);
int run_fat_dol(char * filename);

// --------------------------------------------------------------------------------
//  GLOBALS
//---------------------------------------------------------------------------------
void * framebuffer = NULL;
static lwp_t gclink_handle = (lwp_t)NULL;

const static char gclink_exec_dol[] = "EXECDOL";
const static char gclink_exec_elf[] = "EXECELF";
const static char gclink_copy_dol[] = "COPYDOL";
const static char gclink_hello[] = "HELLO";
const static char gclink_reset[] = "RESET";

// stub .s sourced file
extern u8 stub_bin[];
extern u32 stub_bin_size;

static t_bba_config bba_config_struct;

//---------------------------------------------------------------------------------
// Reset button callbak
//---------------------------------------------------------------------------------
static void reset_cb(u32 irq, void* ctx) {
  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

//---------------------------------------------------------------------------------
// Utils
//--------------------------------------------------------------------------------
unsigned int convert_int(unsigned int in) {
  unsigned int out;
  char *p_in = (char *) &in;
  char *p_out = (char *) &out;
  p_out[0] = p_in[3];
  p_out[1] = p_in[2];
  p_out[2] = p_in[1];
  p_out[3] = p_in[0];
  return out;
}
#define __stringify(rn) #rn

//---------------------------------------------------------------------------------
// init
//---------------------------------------------------------------------------------
void * init_screen() {

	GXRModeObj *rmode;

	VIDEO_Init();
	DVD_Init();
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
	kprintf("USB Gecko traces enabled, hello Gecko!!!\n");

	VIDEO_Configure(rmode);
	VIDEO_ClearFrameBuffer (rmode, framebuffer, COLOR_BLACK);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// No stack - we need it all
	AR_Init(NULL, 0);
	AR_Reset();
	AR_Init(NULL, 0);
	ARAMClear();

	// allow 32mhz exi bus
	*(volatile unsigned long*)0xcc00643c = 0x00000000;
	ipl_set_config(6);

	return 0;
}

//---------------------------------------------------------------------------------
// Routine to run an ELF or a DOL
//---------------------------------------------------------------------------------
int run_executable(u8 * data, bool is_dol, char ** bba_config) {

	int ret = 0;
	int n = 0;

	// install stub
	// printf("Installing stub\n");
	// ret = install_stub();
	// if (ret == 1) {
	// 	printf("Could not copy the stub loader in RAM\n");
	// }

	// SYS_SetResetCallback(reset_cb);

	if(bba_config != NULL) {
		n = 3;
		printf("Program args: [%s, %s, %s]\n", bba_config[0], bba_config[1], bba_config[2]);
	}

	if(is_dol) {
		// Check DOL header
		DOLHEADER *dolhdr = (DOLHEADER*) data;
		printf("DOL Load address: %08X\n", dolhdr->textAddress[0]);
		printf("DOL Entrypoint: %08X\n", dolhdr->entryPoint);
		printf("BSS: %08X Size: %iKB\n", dolhdr->bssAddress, (int)((float)dolhdr->bssLength/1024));

		// Copy DOL to ARAM then execute
		ret = DOLtoARAM(data, n, bba_config);
	}
	else {
		printf("Executing ELF\n");
		ret = ELFtoARAM(data, n, bba_config);
	}

	return ret;

}

//---------------------------------------------------------------------------------
// Copy DOL to fat
//---------------------------------------------------------------------------------
int copy_fat_file(const DISC_INTERFACE *iface, void * data, u32 size, char * filename) {

	int bytes_written = 0;
	char target_filename[50];

	// mount fat
    printf("Mounting fat\n");
    if (fatMountSimple("fat", iface) == false) {
        printf("Error: Couldn't mount fat\n");
        return -1;
    }

	snprintf(target_filename, sizeof(target_filename), "%s%s", "fat:/", filename);

	// write file
    printf("Writing  %s\n", target_filename);
	FILE *fp = fopen(target_filename, "wb");
	if (fp) {
		fseek(fp, 0, SEEK_SET);
		bytes_written = fwrite(data, 1, size, fp);
		fclose(fp);

		if(bytes_written != size) {
			printf("Error: Couldn't write file, bytes written: %d expected: %d\n", bytes_written, size);
			return -1;
		}
	}
	else {
		printf("Error: Couldn't open file %s for writing\n", target_filename);
		return -1;
	}

	printf("Success: file %s was writing on storage\n", target_filename);

	return 1;
}

//---------------------------------------------------------------------------------
// Load DOL from fat
//---------------------------------------------------------------------------------
int load_fat_file(const DISC_INTERFACE *iface, char * filename) {

    int res = 1;
	u8 * dol = NULL;

	// mount fat
    printf("Mounting fat\n");
    if (fatMountSimple("fat", iface) == false) {
        printf("Error: Couldn't mount fat\n");
        return 0;
    }

	// read file and copy to ARAM
    printf("Reading %s\n", filename);
	FILE *fp = fopen(filename, "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);

		int mram_size = (SYS_GetArenaHi() - SYS_GetArenaLo());
		printf("Memory available: %iB\n", mram_size);
		printf("DOL size is %iB\n", size);

		fseek(fp, 0, SEEK_SET);

		if ((size > 0) && (size < (AR_GetSize() - (64 * 1024)))) {
			dol = (u8*) memalign(32, size);
			if (dol) {
				fread(dol, 1, size, fp);

				// Print DOL header
				DOLHEADER *dolhdr = (DOLHEADER*) dol;
				printf("Loading %s from fat\n", filename);
				printf("DOL Load address: %08X\n", dolhdr->textAddress[0]);
				printf("DOL Entrypoint: %08X\n", dolhdr->entryPoint);
				printf("BSS: %08X Size: %iKB\n", dolhdr->bssAddress, (int)((float)dolhdr->bssLength/1024));
			}
			else {
				printf("Error: Couldn't allocate memory\n");
				res = -1;
			}
		}
		else {
			printf("Error: DOL is empty or too big to fit in ARAM\n");
			res = -1;
		}
		printf("Closing file\n");
		fclose(fp);
    }
	else {
		kprintf("Error: Failed to open file\n");
        res = -1;
	}

    printf("Unmounting fat\n");
	fatUnmount("fat");

	// execute DOL
	if (dol != NULL) {
		DOLtoARAM(dol, 0, NULL);

		//We shouldn't reach this point
		if (dol != NULL) free(dol);
	}

    return res;
}


//---------------------------------------------------------------------------------
// Routine to copy Stub reloader in memory
//---------------------------------------------------------------------------------
int install_stub() {

	kprintf("Installing reload stub\n");
	void * dst = (void *) 0x80001800;

	// check it doesn't overload
	if ( ((u32)dst + stub_bin_size) > 0x80003000) {
		printf("The Reload stub size is too big: %d bytes, end address: %08X\n", stub_bin_size, ((u32)dst + stub_bin_size));
		return 1;
	}

	// Copy LoaderStub to 0x80001800
	memcpy(dst, stub_bin, stub_bin_size);

	// Flush and invalidate cache
	DCFlushRange(dst, stub_bin_size);
	ICInvalidateRange(dst, stub_bin_size);

	return 0;
}

void pad_check() {

	PAD_ScanPads();

	if((PAD_ButtonsDown(0) & PAD_BUTTON_START) || SYS_ResetButtonDown()) {
		printf("Reset!!!\n");
		reset(SYS_RESTART);
	}
	if(PAD_ButtonsDown(0) & PAD_TRIGGER_Z) {
		printf("Lading swiss from FAT storage\n");
		load_fat_file(&__io_gcsd2, SWISS_FILENAME);
		load_fat_file(&__io_gcsdb, SWISS_FILENAME);
		load_fat_file(&__io_gcsda, SWISS_FILENAME);
	}
	if((PAD_ButtonsDown(0) & PAD_BUTTON_A)) {
		CON_EnableGecko(GECKO_CHANNEL, true);
		kprintf("Hello Gecko on port %d!!!\n", GECKO_CHANNEL);
	}
}

//---------------------------------------------------------------------------------
// Main
//---------------------------------------------------------------------------------
int main() {

	int mram_size = (SYS_GetArenaHi() - SYS_GetArenaLo())/1024;
	init_screen();

	printf("\n\ngclink server by Shazz/TRSi - Version 0.5\n");

	install_stub();
	SYS_SetResetCallback(reset_cb);

	printf("Splash RAM Available (MRAM): %i / 24576 KB\n", mram_size);
	printf("Audio RAM Available: (ARAM): %i / 16384 KB (addr: %#X)\n", AR_GetSize()/1024, AR_GetBaseAddress());
	printf("Press START/RESET to reset the GameCube or Z for swiss\n");

	pad_check();

	if(exi_bba_exists()) {
		setup_network_thread();
	}
	else {
        printf("Broadband Adapter not found!\n");

		if(usb_isgeckoalive(GECKO_CHANNEL)) {
			setup_gecko_thread();
		}
		else {
			printf("USB Gecko Adapter not found!\n");
		}
    }

	while(1) {
		VIDEO_WaitVSync();
		pad_check();
	}
	return 0;
}

//---------------------------------------------------------------------------------
// Create thread to listen on the BBA interface
//---------------------------------------------------------------------------------
int setup_network_thread() {

	char localip[16] = {0};
	char gateway[16] = {0};
	char netmask[16] = {0};

	if(!USE_DHCP) {
		strcpy(localip, DEFAULT_IP);
		strcpy(gateway, DEFAULT_GTW);
		strcpy(netmask, DEFAULT_MASK);
	}

	printf("Configuring network...\n");

	// Configure the network interface (libogc2 doesn't use timeout)
	if (if_config( localip, netmask, gateway, USE_DHCP) >= 0) {
		printf("Network configured IP: %s, GW: %s, MASK: %s\n", localip, gateway, netmask);

		strcpy(bba_config_struct.localip, localip);
		strcpy(bba_config_struct.gateway, gateway);
		strcpy(bba_config_struct.netmask, netmask);

		LWP_CreateThread(	&gclink_handle,					/* thread handle */
							gclink_bba,						/* code */
							(void *)&bba_config_struct,		/* arg pointer for thread */
							NULL,							/* stack base */
							16*1024,						/* stack size */
							10								/* thread priority */ );

		printf("BBA Server is ready to receive commands!\n");
	}
	else {
		printf("Network configuration failed!\n");
		return 1;
	}

	return 0;
}

//---------------------------------------------------------------------------------
// Create listener on the USB Gecko interface
//---------------------------------------------------------------------------------
int setup_gecko_thread() {

	LWP_CreateThread(	&gclink_handle,	/* thread handle */
						gclink_gecko,	/* code */
						NULL,			/* arg pointer for thread */
						NULL,			/* stack base */
						16*1024,		/* stack size */
						50				/* thread priority */ );

	printf("Gecko Server is ready to receive commands!\n");

	return 0;
}

//---------------------------------------------------------------------------------
// Gecko Thread entry point
//---------------------------------------------------------------------------------
void * gclink_gecko(void *arg) {

	unsigned char data = GC_READY;
	unsigned int size = 0;
	unsigned char* buffer = NULL;
	unsigned char* pointer = NULL;

	printf("\nSending ready\n");
	usb_sendbuffer_safe(GECKO_CHANNEL, &data, 1);

	printf("Waiting for connection via USB-Gecko in Slot B ...\n");
	while((data != PC_READY) && (data != PC_OK)) {
		usb_recvbuffer_safe(GECKO_CHANNEL, &data, 1);
	}

	if(data == PC_READY)
	{
		printf("Respond with OK\n");

		// Sometimes the PC can fail to receive the byte, this helps
		usleep(100000);
		data = GC_OK;
		usb_sendbuffer_safe(GECKO_CHANNEL, &data,1);
	}

	printf("Getting DOL info...\n");
	usb_recvbuffer_safe(GECKO_CHANNEL, &size, 4);
	size = convert_int(size);
	printf("Size: %i bytes\n", size);

	printf("Receiving file...\n");
	buffer = (unsigned char*) memalign(32, size);
	pointer = buffer;

	if(!buffer) {
		printf("Failed to allocate memory!!\n");
	}
	else {
		while(size>0xF7D8) {
			usb_recvbuffer_safe(GECKO_CHANNEL, (void*)pointer, 0xF7D8);
			size-=0xF7D8;
			pointer+=0xF7D8;
		}

		if(size) {
			usb_recvbuffer_safe(GECKO_CHANNEL, (void*)pointer, size);
		}

		run_executable((u8 *)buffer, true, NULL);

	// infinite loop for now, would be better to accept more commands...
	while(1);
	}

	return NULL;
}


//---------------------------------------------------------------------------------
// Network Thread entry point
//---------------------------------------------------------------------------------
void * gclink_bba(void * arg) {

	int sock, csock;
	int ret;
	u32	clientlen;
	struct sockaddr_in client;
	struct sockaddr_in server;
	char temp[1026];

	// to avoid incompatible pointer type warning, recast struct pointer from void *
	t_bba_config * bba_config_struct = (t_bba_config *) arg;
	char * bba_config[3] = {bba_config_struct->localip, bba_config_struct->gateway, bba_config_struct->netmask};

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

                    ret = parse_command(csock, temp, bba_config);
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
int parse_command(int csock, char * packet, char ** bba_config) {

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
			return -1;
		}

		if(size != 0) {
			printf("Executing DOL of size %d\n", size);

			sprintf(response, "OK");
			net_send(csock, response, strlen(response), 0);

			execute_dol(csock, size, bba_config);

			sprintf(response, "DOL executed");
			net_send(csock, response, strlen(response), 0);
		}
		else {
			sprintf(response, "Incomplete %s command, missing size", gclink_exec_dol);
			net_send(csock, response, strlen(response), 0);
			return -1;
		}

    }
	else if(!strncmp(packet, gclink_copy_dol, strlen(gclink_copy_dol))) {
		char filename[50];
        int size = 0;
		char * token;
		char storage[10];
		storage_t dest;

		token = strtok(&packet[strlen(gclink_exec_dol)], " ");
		if(token != NULL) {

			// filename
			sprintf(filename, token);

			// storage
			token = strtok(NULL, " ");
			if(token != NULL) {
				sprintf(storage, token);
			}

			// size
			token = strtok(NULL, " ");
			if(token != NULL) {
				size = atoi(token);
			}
		}
		else {
			sprintf(response, "Incomplete %s command, missing filename", gclink_exec_dol);
			net_send(csock, response, strlen(response), 0);
			return -1;
		}

		if(size != 0) {

			printf("Copying DOL of size %d\n", size);

			sprintf(response, "OK");
			net_send(csock, response, strlen(response), 0);

			if(strncmp(storage, "sd2sp2", 6) == 0)
				dest = SD2SP2;
			else if(strncmp(storage, "gecko_a", 7) == 0)
				dest = SDGECKO_A;
			else if(strncmp(storage, "gecko_b", 7)  == 0)
				dest = SDGECKO_B;
			else {
				sprintf(response, "Unknown storage %s", storage);
				net_send(csock, response, strlen(response), 0);
				return -1;
			}

			printf("DOL %s of size %d received, ready to be copied on %s\n", filename, size, storage);
			copy_dol(csock, size, filename, dest, bba_config);

			sprintf(response, "DOL copied");
			net_send(csock, response, strlen(response), 0);
		}
		else {
			sprintf(response, "Incomplete %s command, missing size", gclink_exec_dol);
			net_send(csock, response, strlen(response), 0);
			return -1;
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
			return -1;
		}

		if(size != 0) {

			sprintf(response, "OK");
			net_send(csock, response, strlen(response), 0);

			printf("Executing ELF of size %d\n", size);
			execute_elf(csock, size, bba_config);

			sprintf(response, "ELF executed");
			net_send(csock, response, strlen(response), 0);
		}
		else {
			sprintf(response, "Incomplete %s command, missing size", gclink_exec_elf);
			net_send(csock, response, strlen(response), 0);
			return -1;
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
		return -1;
    }

    return 1;
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

    return 1;
}

//---------------------------------------------------------------------------------
// HELLO command
//---------------------------------------------------------------------------------
int say_hello(int csock) {

	char * response = "OLLEH";
	net_send(csock, response, strlen(response), 0);

    return 1;
}

//---------------------------------------------------------------------------------
// DOL copy command
//---------------------------------------------------------------------------------
int copy_dol(int csock, int size, char * filename, storage_t dest, char ** bba_config) {

	// create a buffer and a pointer to the position in this buffer
    u8 * data = (u8*) memalign(32, size);
    u8 * data_ptr = data;
	int ret = 0;
	int received_size = size;

	if(!data) {
		printf("Failed to allocate memory!!\n");
		return -1;
	}

	printf("Receiving file of size %d per packet of %i bytes:", size, BUFFER_SIZE);
	while(received_size > BUFFER_SIZE) {
		ret = net_recv(csock, data_ptr, BUFFER_SIZE, 0);
		received_size -= BUFFER_SIZE;
		data_ptr += BUFFER_SIZE;
		printf(".");
	}

	// last packet
	if(received_size) {
		ret = net_recv(csock, data_ptr, received_size, 0);
	}

	printf(".Done!\n");
	printf("Starting copy of file %s and size %d to %d\n", filename, size, dest);

	switch(dest) {
		case SD2SP2:
			printf("Copying file %s of size %d to sd2sp2\n", filename, size);
			ret = copy_fat_file(&__io_gcsd2, (void *) data, size, filename);
			break;
		case SDGECKO_A:
			printf("Copying file %s of size %d to port A\n", filename, size);
			ret = copy_fat_file(&__io_gcsda, (void *) data, size, filename);
			break;
		case SDGECKO_B:
			printf("Copying file %s of size %d to port B\n", filename, size);
			ret = copy_fat_file(&__io_gcsdb, (void *) data, size, filename);
			break;
		default :
			printf("Unknow storage port: %d\n", dest);
			return -1;
	}

    return ret;
}

//---------------------------------------------------------------------------------
// DOL execute command
//---------------------------------------------------------------------------------
int execute_dol(int csock, int size, char ** bba_config) {

	// create a buffer and a pointer to the position in this buffer
    u8 * data = (u8*) memalign(32, size);
    u8 * data_ptr = data;
	int ret = 0;

	if(!data) {
		printf("Failed to allocate memory!!\n");
		return -1;
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
	printf(".Done!\n");

	run_executable(data, true, bba_config);

    return ret;
}

//---------------------------------------------------------------------------------
// ELF execute command
//---------------------------------------------------------------------------------
int execute_elf(int csock, int size, char ** bba_config) {

 	// create a buffer and a pointer to the position in this buffer
    u8 * data = (u8*) memalign(32, size);
    u8 * data_ptr = data;
	int ret = 1;

	if(!data) {
		printf("Failed to allocate memory!!\n");
		return -1;
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
	printf(".Done!\n");

	run_executable(data, false, bba_config);

    return ret;
}

//---------------------------------------------------------------------------------
// DOL run for FAT command
//---------------------------------------------------------------------------------
int run_fat_dol(char * filename) {

	int ret;

	ret = load_fat_file(&__io_gcsd2, filename);
	return ret;
}