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
#include <sdcard/gcsd.h>
#include <unistd.h>

#include "exi.h"
#include "sidestep.h"
#include "ffshim.h"
#include "fatfs/ff.h"

// --------------------------------------------------------------------------------
//  DEFINES
//---------------------------------------------------------------------------------
#define FILENAME "/autoexec.dol"
#define TIMEOUT 3*60

// --------------------------------------------------------------------------------
//  DECLARATIONS
//---------------------------------------------------------------------------------
void * init_screen();
void dol_alloc(int size);
int load_fat_file(const char * slot_name, const DISC_INTERFACE * iface, char * filename);

// --------------------------------------------------------------------------------
//  GLOBALS
//---------------------------------------------------------------------------------
u8 *dol = NULL;

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
	char * port = "sd2";
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
			port = "sda";
			iface = &__io_gcsda;
			break;
		}
		else if(buttonsDown & PAD_BUTTON_B) {
			port = "sdb";
			iface = &__io_gcsdb;
			break;
		}
		else if(buttonsDown & PAD_BUTTON_START) {
			port = "sd2";
			iface = &__io_gcsd2;
			break;
		}
		frames_counter += 1;
	}

	if (load_fat_file(port, iface, FILENAME) && (dol)) {

			// Print DOL header
			DOLHEADER *dolhdr = (DOLHEADER*) dol;
			printf("Loading %s from slot %s\n", FILENAME, port);
			printf("DOL Load address: %08X\n", dolhdr->textAddress[0]);
			printf("DOL Entrypoint: %08X\n", dolhdr->entryPoint);
			printf("BSS: %08X Size: %iKB\n", dolhdr->bssAddress, (int)((float)dolhdr->bssLength/1024));

			// Copy DOL to ARAM then execute
			DOLtoARAM(dol, 0, NULL);
	}
	else {
		printf("Cannot mount and find %s on port %s!\n", FILENAME, port);
		printf("Rebooting in 5s");
		sleep(5);
	}

	return 0;
}

//---------------------------------------------------------------------------------
// Load DOL from fat
//---------------------------------------------------------------------------------
int load_fat_file(const char *slot_name, const DISC_INTERFACE *iface_, char * filename)
{
    int res = 1;
    FATFS fs;
    iface = iface_;
	char name[256];
	FIL file;

    printf("Trying to mount %s\n", slot_name);
    if (f_mount(&fs, "", 1) != FR_OK)
    {
        kprintf("Couldn't mount %s\n", slot_name);
        return 0;
    }

    f_getlabel(slot_name, name, NULL);
    printf("Mounted %s as %s\n", name, slot_name);

    printf("Reading %s\n", filename);
    if (f_open(&file, filename, FA_READ) == FR_OK)
    {
		size_t size = f_size(&file);
		UINT _;

		dol_alloc(size);
		if (dol)
		{
			f_read(&file, dol, size, &_);
			f_close(&file);
		}
		else {
			kprintf("Failed to allocate memory for file\n");
			res = 0;
		}
    }
	else {
		kprintf("Failed to open file\n");
        res = 0;
	}

    printf("Unmounting slot  %s\n", slot_name);
    iface->shutdown();
    iface = NULL;

    return res;
}

//---------------------------------------------------------------------------------
// Allocate DOL
//---------------------------------------------------------------------------------
void dol_alloc(int size) {
    int mram_size = (SYS_GetArenaHi() - SYS_GetArenaLo());

    printf("Memory available: %iB\n", mram_size);
    printf("DOL size is %iB\n", size);

    if (size <= 0)
    {
        printf("The DOL is empty !\n");
        return;
    }

    if (size >= (AR_GetSize() - (64 * 1024)))
    {
        printf("DOL size is too big\n");
        return;
    }

    dol = (u8 *) memalign(32, size);
}