/*===========================================
    NoNameNo FAKE Unlimited 3D textured Cube rout ;)
    Still the same technic used in the 2D version
============================================*/
#include <grrlib.h>

#include <stdlib.h>
#include <math.h>
#include <malloc.h>


#include "gfx/girl.h"
#include "gfx/logo.h"
#include "gfx/font.h"

// Callback
static void reset_cb(u32 irq, void* ctx) {
  	void (*reload)() = (void(*)()) 0x80001800;
  	reload ();
}

int main() {
    int i;
    int screen_index = 0;
    float t=0;
    const int R=81;
    const int r=71;
    const int d=120;
    float f=0;
    float ff=0;
    float spr=0;
    int n=1;
    float a=0;
    float cubeZ=10.0f;
    float camZ=30.0f;
    int nb_screens = 3;

    PADStatus pads[4];

    SYS_SetResetCallback(reset_cb);
    GRRLIB_Init();
    PAD_Init();

    GRRLIB_Settings.antialias = false;

    GRRLIB_texImg *tex_screen[nb_screens];
    for(i=0; i<nb_screens; i++) {
        tex_screen[i] = GRRLIB_CreateEmptyTexture(rmode->fbWidth, rmode->efbHeight);
    }

    GRRLIB_texImg *tex_girl = GRRLIB_LoadTexture(girl);
    GRRLIB_texImg *tex_logo = GRRLIB_LoadTexture(logo);
    GRRLIB_texImg *tex_font = GRRLIB_LoadTexture(font);

    GRRLIB_InitTileSet(tex_font, 16, 16, 32);

    for(i=0;i<=255;i+=1) {
        GRRLIB_Printf((640-(16*16))/2, 200, tex_font, 0xFFFFFF00|i, 1, "HOW MANY 3D CUBE");
        GRRLIB_Printf((640-(16*20))/2, 216, tex_font, 0xFFFFFF00|i, 1, "CAN YOU DISPLAY WITH");
        GRRLIB_DrawImg((640-352)/2, 248, tex_logo, 0, 1, 1, 0xFFFFFF00|i);
        GRRLIB_Printf((640-(16*28))/2, 480-16, tex_font, 0xFFFFFF00|i, 1, "BY NONAMENO FROM GRRLIB TEAM");
        GRRLIB_Render();
    }
    for(i=255;i>=0;i-=2) {
        GRRLIB_Printf((640-(16*16))/2, 200, tex_font, 0xFFFFFF00|i, 1, "HOW MANY 3D CUBE");
        GRRLIB_Printf((640-(16*20))/2, 216, tex_font, 0xFFFFFF00|i, 1, "CAN YOU DISPLAY WITH");
        GRRLIB_DrawImg((640-352)/2, 248, tex_logo, 0, 1, 1, 0xFFFFFF00|i);
        GRRLIB_Printf((640-(16*28))/2, 480-16, tex_font, 0xFFFFFF00|i, 1, "BY NONAMENO FROM GRRLIB TEAM");
        GRRLIB_Render();
    }

    GRRLIB_SetBackgroundColour(0xFF, 0x00, 0x00, 0xFF);

    while(1) {
        PAD_Read(pads);
        GRRLIB_Camera3dSettings(0.0f,0.0f,camZ, 0,1,0, 0,0,0);

        GRRLIB_2dMode();
        GRRLIB_DrawImg(0, 0, tex_screen[screen_index], 0, 1, 1, 0xFFFFFFFF);

        GRRLIB_3dMode(0.1, 1000, 45, 1, 0);
        GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);

        cubeZ+=0.02f;
        GRRLIB_ObjectView((float)(((R + r-ff)*cos(t-f) - d*cos(((R + r-f)/r)*t)))/20.0f,(float)(((R + r-ff)*sin(t) - d*sin(((R + r)/r)*t)-f))/20.0f,sin(cubeZ)*10, a,a*2,a*3,1,1,1);
        GRRLIB_SetTexture(tex_girl, 0);
        GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
            GX_Position3f32(-1.0f,1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,0.0f);
            GX_Position3f32(1.0f,1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,0.0f);
            GX_Position3f32(1.0f,-1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,1.0f);
            GX_Position3f32(-1.0f,-1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,1.0f);

            GX_Position3f32(1.0f,1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,0.0f);
            GX_Position3f32(-1.0f,1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,0.0f);
            GX_Position3f32(-1.0f,-1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,1.0f);
            GX_Position3f32(1.0f,-1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,1.0f);

            GX_Position3f32(1.0f,1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,0.0f);
            GX_Position3f32(1.0f,1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,0.0f);
            GX_Position3f32(1.0f,-1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,1.0f);
            GX_Position3f32(1.0f,-1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,1.0f);

            GX_Position3f32(-1.0f,1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,0.0f);
            GX_Position3f32(-1.0f,1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,0.0f);
            GX_Position3f32(-1.0f,-1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,1.0f);
            GX_Position3f32(-1.0f,-1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,1.0f);

            GX_Position3f32(-1.0f,1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,0.0f);
            GX_Position3f32(1.0f,1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,0.0f);
            GX_Position3f32(1.0f,1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,1.0f);
            GX_Position3f32(-1.0f,1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,1.0f);

            GX_Position3f32(1.0f,-1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,0.0f);
            GX_Position3f32(-1.0f,-1.0f,-1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,0.0f);
            GX_Position3f32(-1.0f,-1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(1.0f,1.0f);
            GX_Position3f32(1.0f,-1.0f,1.0f);
            GX_Color1u32(0xFFFFFFFF);
            GX_TexCoord2f32(0.0f,1.0f);
        GX_End();

        a+=0.5f;

        GRRLIB_2dMode();
        GRRLIB_Screen2Texture(0, 0, tex_screen[screen_index], GX_FALSE);
        GRRLIB_Printf((640-(16*6*5))/2+5, 200+5, tex_font, 0x00000088, 5, "%06d",(int)spr);
        GRRLIB_Printf((640-(16*6*5))/2, 200, tex_font, 0xFFEEEE88, 5, "%06d",(int)spr);

        GRRLIB_Render();
        screen_index++;
        screen_index %= nb_screens;
        spr+=0.1f;
        t+=0.01f;

        if(t>n*2*M_PI) {
            n++;
            f+=0.01f;
        }

        if(f>2*M_PI) {
            f=0;
            ff+=0.02f;
        }

        if (pads[0].button & PAD_BUTTON_START) {
            break;
        }
    }
    GRRLIB_FreeTexture(tex_logo);
    GRRLIB_FreeTexture(tex_girl);
    GRRLIB_FreeTexture(tex_font);
    for(i=0; i<nb_screens; i++) {
        GRRLIB_FreeTexture(tex_screen[i]);
    }
    GRRLIB_Exit(); // Be a good boy, clear the memory allocated by GRRLIB

    return 0;
}

