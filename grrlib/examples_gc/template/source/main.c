/*===========================================
        GRRLIB (GX Version)
        - Template Code -

        Minimum Code To Use GRRLIB
============================================*/
#include <grrlib.h>

#include <stdlib.h>
#include <gccore.h>

// Callback
static void reset_cb(u32 irq, void* ctx) {
  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

int main(int argc, char **argv) {

    PADStatus pads[4];

    // Initialise the Graphics & Video subsystem
    SYS_SetResetCallback(reset_cb);    
    GRRLIB_Init();

    // Initialise the pads
    PAD_Init();

    // Loop forever
    while(1) {


        PAD_Read(pads);
		if (pads[0].button & PAD_BUTTON_START) break;

        // ---------------------------------------------------------------------
        // Place your drawing code here
        // ---------------------------------------------------------------------

        GRRLIB_Render();  // Render the frame buffer to the TV
    }

    GRRLIB_Exit(); // Be a good boy, clear the memory allocated by GRRLIB

    exit(0);  // Use exit() to exit a program, do not use 'return' from main()
}
