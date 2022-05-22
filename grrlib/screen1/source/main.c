#include <grrlib.h>
#include <grrmod.h>

#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <libgctools.h>

// Include Graphics
#include "logo_png.h"
#include "font_png.h"
#include "font3d_png.h"
#include "scroller.h"

// include mod
#include "cream_of_the_earth_mod.h"

#include "intro.h"
#include "scene.h"

#define BBA_DEBUG false
#define TRACE_PORT 10000
#define TRACE_IP "192.168.1.53"
#define MOD_VOLUME 60

// Callback
static void return_to_loader (void) {
    return_to_gclink("fat:/gclink.dol");
  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

static void reset_cb(u32 irq, void* ctx) {
  	return_to_loader();
}

int main(int argc, char *argv[]) {

    // debug
    if(BBA_DEBUG) {
        setup_bba_logging(TRACE_PORT, TRACE_IP, KPRINTF, FALSE, argv);
	    kprintf("BBA traces enabled to server %s on port %d\n", TRACE_IP, TRACE_PORT);
    }
    else {
        GRRLIB_GeckoInit();
        GRRLIB_GeckoPrintf("Gecko debug enabled\n");
    }
    SYS_SetResetCallback(reset_cb);
    atexit(return_to_loader);

    GRRLIB_Init();
    GRRMOD_Init(true);
    GRRMOD_SetMOD((u8 *)cream_of_the_earth_mod, cream_of_the_earth_mod_size);
    GRRMOD_SetVolume(MOD_VOLUME, MOD_VOLUME);
    GRRMOD_Start();

    PAD_Init();

    // create scene
    t_scene * intro_scene = malloc(sizeof(t_scene));
    intro_scene->init = &screen_init;
    intro_scene->setup = &screen_setup;
    intro_scene->render = &screen_render;
    intro_scene->update = &screen_update;
    intro_scene->events = &screen_events;
    intro_scene->exit = &screen_exit;
    intro_scene->is_enabled = &screen_enabled;

    intro_scene->init();
    intro_scene->setup();

    while(intro_scene->is_enabled() == false) {
        intro_scene->render();
        intro_scene->update();
        intro_scene->events();
    }

    // free stuff
    GRRMOD_Unload();
    GRRMOD_End();

    GRRLIB_Exit(); // Be a good boy, clear the memory allocated by GRRLIB
    return_to_gclink("fat:/gclink.dol");

    exit(0);
}
