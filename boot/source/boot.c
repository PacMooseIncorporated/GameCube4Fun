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
	const DISC_INTERFACE * iface = &__io_gcsd2;

	init_screen();

	printf("\n\nExploit quick boot by Shazz/TRSi\n");
	printf("Press:\n");
	printf("[Start] for sd2sp2 (default)\n");
	printf("[A]     for SD gecko in port A\n");
	printf("[B]     for SD gecko in port B\n");

	while(frames_counter <= TIMEOUT) {
		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);

		if(buttonsDown & PAD_BUTTON_A) {
			iface = &__io_gcsda;
			break;
		}
		else if(buttonsDown & PAD_BUTTON_B) {
			iface = &__io_gcsdb;
			break;
		}
		else if(buttonsDown & PAD_BUTTON_START) {
			iface = &__io_gcsd2;
			break;
		}
		frames_counter += 1;
	}

	if (!load_fat_file(iface, FILENAME)) {
		printf("Cannot mount fat and find %s!\n", FILENAME);
		printf("Rebooting in 5s");
		sleep(5);
	}

	return 0;
}

//---------------------------------------------------------------------------------
// Load DOL from fat
//---------------------------------------------------------------------------------
int load_fat_file(const DISC_INTERFACE *iface, char * filename)
{
    int res = 1;
	char label[256];

	// mount fat
    printf("Trying to mount fat\n");
    if (fatMountSimple("fat", iface) == false) {
        kprintf("Couldn't mount fat\n");
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
			u8 *dol = (u8*) memalign(32, size);
			if (dol) {
				fread(dol, 1, size, fp);

				// Print DOL header
				DOLHEADER *dolhdr = (DOLHEADER*) dol;
				printf("Loading %s from fat\n", FILENAME);
				printf("DOL Load address: %08X\n", dolhdr->textAddress[0]);
				printf("DOL Entrypoint: %08X\n", dolhdr->entryPoint);
				printf("BSS: %08X Size: %iKB\n", dolhdr->bssAddress, (int)((float)dolhdr->bssLength/1024));

				// execute DOL
				DOLtoARAM(dol, 0, NULL);

				//We shouldn't reach this point
				if (dol != NULL) free(dol);
			}
			else {
				printf("Couldn't allocate memory\n");
			}
		}
		else {
			printf("DOL is empty or too big to fit in ARAM\n");
		}
		fclose(fp);
    }
	else {
		kprintf("Failed to open file\n");
        res = 0;
	}

    printf("Unmounting fat\n");
    iface = NULL;
	fatUnmount("fat");

    return res;
}

