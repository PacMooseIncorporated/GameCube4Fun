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

static void return_to_loader (void)
{
	printf("Return to loader\n");
  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

int main() {

	VIDEO_Init();

	rmode = VIDEO_GetPreferredMode(NULL);

	PAD_Init();

	atexit(return_to_loader);

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
		
	VIDEO_Configure(rmode);
		
	VIDEO_SetNextFramebuffer(xfb);
	
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	console_init(xfb,30,30,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*2);

	time_t gc_time;
	gc_time = time(NULL);

	srand(gc_time);

	printf("testing console\n");
	std::cout << "Hello World" << std::endl;
	printf("random number is %08x\n",rand());

	while(1) {

		gc_time = time(NULL);
		printf("\x1b[10;0HRTC time is %s     ",ctime(&gc_time));

		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);

		if (buttonsDown & PAD_BUTTON_START) {
			printf("atexit(0)\n");
			atexit(0);
		}
		if (buttonsDown & PAD_BUTTON_A) {
			printf("atexit(return_to_loader)\n");
			atexit(return_to_loader);
		}
		if (buttonsDown & PAD_BUTTON_B) {
			printf("exit(0)\n");
			exit(0);
		}
		if (buttonsDown & PAD_BUTTON_X) {
			break;
		}
	}

	printf("Bye bye!)\n");
	return_to_loader();

}