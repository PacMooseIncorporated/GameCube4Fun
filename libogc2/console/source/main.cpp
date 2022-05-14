#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <time.h>
#include <sys/time.h>

#include <iostream>
#include <debug.h>
#include <math.h>


static void *xfb = NULL;

u32 first_frame = 1;
GXRModeObj *rmode;
vu16 oldstate;
vu16 keystate;
vu16 keydown;
vu16 keyup;
PADStatus pad[4];

// Callbacks
static void return_to_loader (void) {
	printf("Return to loader\n");
  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

static void reset_cb(u32 irq, void* ctx) {
	printf("Reset button pushed with IRQ %d\n!", irq);
	return_to_loader();
}

int main() {

	VIDEO_Init();

	rmode = VIDEO_GetPreferredMode(NULL);

	PAD_Init();

	// be sure we're going back to the gclink loader
	// atexit(return_to_loader);
	SYS_SetResetCallback(reset_cb);

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_Configure(rmode);
		
	VIDEO_SetNextFramebuffer(xfb);
	
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	console_init(xfb, 20, 20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*2);

	time_t gc_time;
	gc_time = time(NULL);

	srand(gc_time);

	printf("\n\nTesting console\n");
	std::cout << "Hello World" << std::endl;
	printf("Random number is %08x\n",rand());

	while(1) {

		gc_time = time(NULL);
		printf("\x1b[10;0HRTC time is %s     ",ctime(&gc_time));

		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);

		if (buttonsDown & PAD_BUTTON_START) {
			printf("Existing with specific atexit\n");
			atexit(return_to_loader);
			exit(0);
		}
		if (buttonsDown & PAD_BUTTON_A) {
			printf("Existing with exit(0)\n");
			exit(0);
		}
	}

	printf("Bye bye!)\n");
}