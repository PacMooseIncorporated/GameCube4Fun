/*===========================================
        GRRLIB
	Spot Light Sample Code
============================================*/
#include <grrlib.h>

#include <stdlib.h>
#include <math.h>

#include "Snap_ITC_12.h"

// Callback
static void reset_cb(u32 irq, void* ctx) {
  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

int main(int argc, char **argv) {
    f32 lightx=0.0f;
    PADStatus pads[4];

    GRRLIB_Init();

    GRRLIB_texImg *tex_font = GRRLIB_LoadTexture(Snap_ITC_12);
    GRRLIB_InitTileSet(tex_font, 17, 22, 32);

    PAD_Init();
    SYS_SetResetCallback(reset_cb);    

    GRRLIB_SetBackgroundColour(0x40, 0x40, 0x40, 0xFF);

    while(1) {
        GRRLIB_2dMode();
        PAD_Read(pads);
        if (pads[0].button & PAD_BUTTON_START) {
            break;
        }

        GRRLIB_Camera3dSettings(0.0f,0.0f,3.0f, 0,1,0, 0,0,0);

        GRRLIB_SetLightAmbient(0x404040FF);

        GRRLIB_SetLightSpot(1, (guVector){ sin(lightx)*2.5f, 0.8f, 0 }, (guVector){  sin(lightx)*2.5f, 0.0f, 0.0f }, -4.0f, 5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0x0000FFFF);
        GRRLIB_SetLightSpot(2, (guVector){ -sin(lightx)*2.5f, 0.8f, 0 }, (guVector){  -sin(lightx)*2.5f, 0.0f, 0.0f }, -4.0f, 5.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0xFF0000FF);

        GRRLIB_3dMode(0.1,1000,45,0,1);
        GRRLIB_ObjectView(0,-0.8,0, -90,0,0,1,1,1);
        GRRLIB_DrawTessPanel(6.2f,0.17f,3.7f,0.1f,0,0xFFFFFFFF);

        lightx+=0.05f;

        GRRLIB_2dMode();
        GRRLIB_Printf((640-(17*26))/2, 480-25, tex_font, 0xFFFFFFFF, 1, "GRRLIB SPOT LIGHT SAMPLE 1");

        GRRLIB_Render();
    }

    GRRLIB_FreeTexture(tex_font);
    GRRLIB_Exit();

    exit(0);
}
