/*===========================================
    NoNameNo Compositing Sample Code
============================================*/
#include <grrlib.h>

#include <stdlib.h>

#include "gfx/font3d.h"


// Callback
static void reset_cb(u32 irq, void* ctx) {
  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

int main() {
    float rot=0;
    float i;
    int circsize=150;
    const char text[]="GRRLIB ROXX ";
    PADStatus pads[4];

    SYS_SetResetCallback(reset_cb);
    GRRLIB_Init();
    PAD_Init();


    GRRLIB_texImg *tex_font = GRRLIB_LoadTexture(font3d);
    GRRLIB_InitTileSet(tex_font, 64, 64, 32);
    GRRLIB_SetHandle(tex_font, tex_font->tilew/2, tex_font->tileh+circsize);

    GRRLIB_texImg *tex_screen = GRRLIB_CreateEmptyTexture(rmode->fbWidth,rmode->efbHeight);

    GRRLIB_Settings.antialias = true;

    while(1) {
        PAD_Read(pads);
        if (pads[0].button & PAD_BUTTON_START) {
            break;
        }

        // we say that we will want to capture to a texture all the following
        GRRLIB_CompoStart();

            for(i=0; i<360; i+=30) {
                // We draw some letters
                GRRLIB_DrawTile((rmode->fbWidth/2)-(tex_font->tilew/2), (rmode->efbHeight/2)-(tex_font->tileh+circsize), tex_font, rot+i, 1, 1, 0xFFFFFFFF, text[(int)(i/30)]-32);
            }

        // we say we want to capture now, (the buffer will be cleared after the capture)
        GRRLIB_CompoEnd(0, 0, tex_screen);

        rot-=0.6;

        // we now draw 3 times the captured buffer playing with color
        GRRLIB_DrawImg(0, 0, tex_screen, 0, 1, 1, 0xFF00FFFF);
        GRRLIB_DrawImg(50, 50, tex_screen, 0, 1, 1, 0xFFFF00FF);
        GRRLIB_DrawImg(100, 100, tex_screen, 0, 1, 1, 0xFFFFFFFF);

        GRRLIB_Render();
    }

    GRRLIB_FreeTexture(tex_screen);
    GRRLIB_FreeTexture(tex_font);
    GRRLIB_Exit(); // Be a good boy, clear the memory allocated by GRRLIB
    exit(0);
}
