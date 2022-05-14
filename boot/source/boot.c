/*
 * boot: a configurable exploit bootloader
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
#include <debug.h>
#include <errno.h>
#include <unistd.h>

#include <fat.h>
#include <sdcard/gcsd.h>

#include "exi.h"
#include "sidestep.h"

// --------------------------------------------------------------------------------
//  DEFINES
//---------------------------------------------------------------------------------
#define FILENAME "fat:/autoexec.dol"
#define GCLINK "fat:/gclink.dol"
#define TIMEOUT 3*60

// --------------------------------------------------------------------------------
//  DECLARATIONS
//---------------------------------------------------------------------------------
void * init_screen();
void dol_alloc(int size);
int load_fat_file(const DISC_INTERFACE * iface, char * filename);

// --------------------------------------------------------------------------------
//  GLOBALS
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
// init
//---------------------------------------------------------------------------------
void * init_screen() {

	GXRModeObj *rmode;
	void * framebuffer = NULL;

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
// Main
//---------------------------------------------------------------------------------
int main() {

	int frames_counter = 0;

	init_screen();

	printf("\n\nExploit boot loader by Shazz/TRSi\n");
	printf("Withing 3s, press:\n");
	printf("[Start] for sd2sp2 (default)\n");
	printf("[B]     for SD gecko in port B\n");
	printf("[A]     for SD gecko in port A\n");
	printf("[Z]     for gclink on sd2sp2\n");

	while(frames_counter <= TIMEOUT) {
		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);

		if(buttonsDown & PAD_BUTTON_A) {
			if (!load_fat_file(&__io_gcsda, FILENAME)) {
				printf("Error: Cannot mount fat on sda and load %s!\n", FILENAME);
				frames_counter = 0;
			}
		}
		else if(buttonsDown & PAD_BUTTON_B) {
			if (!load_fat_file(&__io_gcsdb, FILENAME)) {
				printf("Error: Cannot mount fat on sdb and load %s!\n", FILENAME);
				frames_counter = 0;
			}
		}
		else if(buttonsDown & PAD_BUTTON_START) {
			if (!load_fat_file(&__io_gcsd2, FILENAME)) {
				printf("Error: Cannot mount fat on sd2 and load %s!\n", FILENAME);
				frames_counter = 0;
			}
		}
		else if(buttonsDown & PAD_TRIGGER_Z) {
			if (!load_fat_file(&__io_gcsd2, GCLINK)) {
				printf("Error: Cannot mount fat on sd2 and load %s!\n", GCLINK);
				frames_counter = 0;
			}
		}
		frames_counter += 1;
	}

	// do in order
	load_fat_file(&__io_gcsd2, FILENAME);
	load_fat_file(&__io_gcsdb, FILENAME);
	load_fat_file(&__io_gcsda, FILENAME);
	load_fat_file(&__io_gcsd2, GCLINK);

	printf("Error: Cannot mount and find any autoexec.dol, rebooting in 5s!\n");
	sleep(5);
	exit(0);

	return 0;
}

//---------------------------------------------------------------------------------
// Load DOL from fat
//---------------------------------------------------------------------------------
int load_fat_file(const DISC_INTERFACE *iface, char * filename)
{
    int res = 1;
	char label[256];
	u8 * dol = NULL;

	// mount fat
    printf("Trying to mount fat\n");
    if (fatMountSimple("fat", iface) == false) {
        printf("Error: Couldn't mount fat\n");
        return 0;
    }

	// get label
    fatGetVolumeLabel("fat", label);
    printf("Mounted %s as fat:/\n", label);

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
				printf("Loading %s from fat\n", FILENAME);
				printf("DOL Load address: %08X\n", dolhdr->textAddress[0]);
				printf("DOL Entrypoint: %08X\n", dolhdr->entryPoint);
				printf("BSS: %08X Size: %iKB\n", dolhdr->bssAddress, (int)((float)dolhdr->bssLength/1024));
			}
			else {
				printf("Error: Couldn't allocate memory\n");
				res = 0;
			}
		}
		else {
			printf("Error: DOL is empty or too big to fit in ARAM\n");
			res = 0;
		}
		printf("Closing file\n");
		fclose(fp);
    }
	else {
		kprintf("Error: Failed to open file\n");
        res = 0;
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

