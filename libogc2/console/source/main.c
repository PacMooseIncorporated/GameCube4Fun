#include <gccore.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <time.h>
#include <sys/time.h>

#include <debug.h>
#include <math.h>

#include <libgctools.h>

// setting console output
out_t output = KPRINTF;
bool keep_console = true;

static void *xfb = NULL;

u32 first_frame = 1;
GXRModeObj *rmode;
vu16 oldstate;
vu16 keystate;
vu16 keydown;
vu16 keyup;
PADStatus pad[4];

#define TRACE_PORT 10000
#define TRACE_IP "192.168.1.53"

// Callbacks
static void return_to_loader (void) {
	printf("Return to loader\n");
	close_bba_logging();

  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

static void reset_cb(u32 irq, void* ctx) {
	printf("Reset button pushed with IRQ %d\n!", irq);
	return_to_loader();
}

int main() {

	int is_connected = 0;

	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	PAD_Init();

	// be sure we're going back to the gclink loader
	SYS_SetResetCallback(reset_cb);

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	console_init(xfb, 20, 20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*2);
	printf("\n\nTesting console\n");

	switch(output) {
		case KPRINTF:
			printf("Using kprintf\n");
			break;
		case PRINTF:
			printf("Using printf\n");
			break;
		case BBA_PRINTF:
			printf("Using bba_printf\n");
			break;
	}

	is_connected = setup_bba_logging(TRACE_PORT, TRACE_IP, output, keep_console);
	kprintf("BBA Traces enabled\n");

	time_t gc_time;
	gc_time = time(NULL);
	srand(gc_time);
	printf("\nRandom number is %08x\n",rand());

	while(1) {

		gc_time = time(NULL);
		printf("\x1b[12;0HRTC time is %s     ",ctime(&gc_time));

		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);

		if (buttonsDown & PAD_BUTTON_A) {
			printf("Doing bad things to test exceptions handler!\n");
			int a = 1/0;
			printf("That's bad! %d\n", a);
			exit(0);
		}
		if (buttonsDown & PAD_BUTTON_START) {
			printf("Exiting with exit(0)\n");
			exit(0);
		}
		if (buttonsDown & PAD_BUTTON_B) {
			if(is_connected) {
				printf("Send UDP message\n");
				switch(output) {
					case KPRINTF:
						kprintf("\x1b[12;0HRTC time is %s     ", ctime(&gc_time));
						break;
					case PRINTF:
						printf("\x1b[12;0HRTC time is %s     ", ctime(&gc_time));
						break;
					case BBA_PRINTF:
						bba_printf("\x1b[12;0HRTC time is %s     ", ctime(&gc_time));
						break;
				}
			}
			else
				printf("No UDP socket established\n");
		}
	}
	printf("Bye bye!)\n");

	return 0;
}