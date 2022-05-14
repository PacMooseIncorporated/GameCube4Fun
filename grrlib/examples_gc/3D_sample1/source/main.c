/*===========================================
    NoNameNo
    Simple Flat 3D cube
============================================*/
#include <grrlib.h>

#include <stdlib.h>
#include <math.h>
#include <malloc.h>

#include "gfx/font.h"

// Callback
static void reset_cb(u32 irq, void* ctx) {
  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

int main() {
    float a=0;
    u32 col[3] = {0xFFFFFFFF, 0xAAAAAAFF, 0x666666FF};
    int cubeZ=0;
    PADStatus pads[4];

    GRRLIB_Init();
    PAD_Init();
    SYS_SetResetCallback(reset_cb);    

    GRRLIB_texImg *tex_font = GRRLIB_LoadTexture(font);
    GRRLIB_InitTileSet(tex_font, 16, 16, 32);

    GRRLIB_Settings.antialias = true;

    GRRLIB_SetBackgroundColour(0x00, 0x00, 0x00, 0xFF);
    GRRLIB_Camera3dSettings(0.0f,0.0f,13.0f, 0,1,0, 0,0,0);

    while(1) {
        GRRLIB_2dMode();
        PAD_Read(pads); 
        if (pads[0].button & PAD_BUTTON_START) {
            break;
        }
        else if (pads[0].button & PAD_BUTTON_A) {
            cubeZ++;
        }
        else if (pads[0].button & PAD_BUTTON_B) {
            cubeZ--;
        }

        GRRLIB_3dMode(0.1,1000,45,0,0);
        GRRLIB_ObjectView(0,0,cubeZ, a,a*2,a*3,1,1,1);
        GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
            GX_Position3f32(-1.0f,1.0f,-1.0f);
            GX_Color1u32(col[0]);
            GX_Position3f32(-1.0f,-1.0f,-1.0f);
            GX_Color1u32(col[0]);
            GX_Position3f32(1.0f,-1.0f,-1.0f);
            GX_Color1u32(col[0]);
            GX_Position3f32(1.0f,1.0f,-1.0f);
            GX_Color1u32(col[0]);

            GX_Position3f32(-1.0f,1.0f,1.0f);
            GX_Color1u32(col[0]);
            GX_Position3f32(-1.0f,-1.0f,1.0f);
            GX_Color1u32(col[0]);
            GX_Position3f32(1.0f,-1.0f,1.0f);
            GX_Color1u32(col[0]);
            GX_Position3f32(1.0f,1.0f,1.0f);
            GX_Color1u32(col[0]);

            GX_Position3f32(-1.0f,1.0f,1.0f);
            GX_Color1u32(col[1]);
            GX_Position3f32(1.0f,1.0f,1.0f);
            GX_Color1u32(col[1]);
            GX_Position3f32(1.0f,1.0f,-1.0f);
            GX_Color1u32(col[1]);
            GX_Position3f32(-1.0f,1.0f,-1.0f);
            GX_Color1u32(col[1]);

            GX_Position3f32(-1.0f,-1.0f,1.0f);
            GX_Color1u32(col[1]);
            GX_Position3f32(1.0f,-1.0f,1.0f);
            GX_Color1u32(col[1]);
            GX_Position3f32(1.0f,-1.0f,-1.0f);
            GX_Color1u32(col[1]);
            GX_Position3f32(-1.0f,-1.0f,-1.0f);
            GX_Color1u32(col[1]);

            GX_Position3f32(-1.0f,1.0f,1.0f);
            GX_Color1u32(col[2]);
            GX_Position3f32(-1.0f,1.0f,-1.0f);
            GX_Color1u32(col[2]);
            GX_Position3f32(-1.0f,-1.0f,-1.0f);
            GX_Color1u32(col[2]);
            GX_Position3f32(-1.0f,-1.0f,1.0f);
            GX_Color1u32(col[2]);

            GX_Position3f32(1.0f,1.0f,1.0f);
            GX_Color1u32(col[2]);
            GX_Position3f32(1.0f,1.0f,-1.0f);
            GX_Color1u32(col[2]);
            GX_Position3f32(1.0f,-1.0f,-1.0f);
            GX_Color1u32(col[2]);
            GX_Position3f32(1.0f,-1.0f,1.0f);
            GX_Color1u32(col[2]);
        GX_End();
        a+=0.5f;

        // Switch To 2D Mode to display text
        GRRLIB_2dMode();
        GRRLIB_Printf((640-(16*29))/2, 20, tex_font, 0xFFFFFFFF, 1, "PRESS A OR B TO ZOOM THE CUBE");

        GRRLIB_Render();
    }
    GRRLIB_FreeTexture(tex_font);
    GRRLIB_Exit(); // Be a good boy, clear the memory allocated by GRRLIB

    exit(0);
}
