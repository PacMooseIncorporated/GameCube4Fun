
#include <grrlib.h>

#include <stdlib.h>
#include <math.h>
#include <malloc.h>

// Include Graphics
#include "logo_png.h"
#include "font_png.h"
#include "font3d_png.h"
#include "scroller.h"

#include "intro.h"

#define TEXT "       TRSI IS BACK AGAIN WITH A NEW INTRO FOR THE GAMECUBE!!!! HI TO ALL MY TRSI FRIENDS!  "
#define SCROLL_SPEED 6
#define SCROLL_INTERLACE 10
#define APPLY_SIN true
#define SIN_FACTOR 20
#define SIN_SPEED 0.5
#define APPLY_MIRROR_WAVE true

bool time_to_exit = false;
float a;
u32 col[3] = {0xFFFFFFFF, 0xAAAAAAFF, 0x666666FF};
int cubeZ;
int cubeY;
float sinx=0;

GRRLIB_texImg * logo_background;
GRRLIB_texImg * text_font;
GRRLIB_texImg * scroll_font;
GRRLIB_texImg * tex_screen1;
GRRLIB_texImg * tex_screen2;
GRRLIB_texImg * tex_screen3;

int alpha = 1;
int alpha_dir = 1;
u8 mirror_alpha = 0x80;
char text[] = TEXT;
t_scroller * scroller;

bool show_cube = true;
bool show_scroller = true;

bool screen_enabled(void){
    return time_to_exit;
}

void screen_init(void) {

    // 3D
    a=0;
    cubeZ=0;
    cubeY=3;
    sinx=0;

    // texture2screen
    tex_screen1 = GRRLIB_CreateEmptyTexture(rmode->fbWidth, 100 /*rmode->efbHeight*/);
    tex_screen2 = GRRLIB_CreateEmptyTexture(rmode->fbWidth, 100);
    tex_screen3 = GRRLIB_CreateEmptyTexture(rmode->fbWidth, 200);

    // Load textures
    logo_background = GRRLIB_LoadTexturePNG(logo_png);
    text_font = GRRLIB_LoadTexture(font_png);
    scroll_font = GRRLIB_LoadTexture(font3d_png);

    // Tilesets
    GRRLIB_InitTileSet(text_font, 16, 16, 32);
    GRRLIB_InitTileSet(scroll_font, 64, 64, 32);
    GRRLIB_InitTileSet(tex_screen3, rmode->fbWidth, 1, 0);
    GRRLIB_InitTileSet(tex_screen2, rmode->fbWidth, 1, 0);

    // Scroller
    scroller = create_scroller(scroll_font, SCROLL_SPEED, SCROLL_INTERLACE, 0, 20, text, APPLY_SIN, SIN_FACTOR, SIN_SPEED);
    // scroller = create_scroller(scroll_font, SCROLL_SPEED, SCROLL_INTERLACE, 0, (rmode->efbHeight - scroll_font->tileh), text);

}

void screen_setup() {
    GRRLIB_Settings.antialias = true;

    GRRLIB_SetBackgroundColour(0x00, 0x00, 0x00, 0xFF);
    GRRLIB_Camera3dSettings(0.0f, 0.0f, 13.0f, 0,1,0, 0,0,0);
}

void screen_exit() {
    free_scroller(scroller);
    GRRLIB_FreeTexture(text_font);
    GRRLIB_FreeTexture(scroll_font);
    GRRLIB_FreeTexture(logo_background);
    GRRLIB_FreeTexture(tex_screen1);
    GRRLIB_FreeTexture(tex_screen2);
    GRRLIB_FreeTexture(tex_screen3);
}

void screen_events(){
    // process events
    PAD_ScanPads();
    if (PAD_ButtonsDown(0) & PAD_BUTTON_START) {
        time_to_exit = true;
    }
    else if (PAD_ButtonsDown(0) & PAD_BUTTON_UP) {
        cubeY++;
    }
    else if (PAD_ButtonsDown(0) & PAD_BUTTON_DOWN) {
        cubeY--;
    }
    else if (PAD_ButtonsDown(0) & PAD_BUTTON_A) {
        show_cube = !show_cube;
    }
    else if (PAD_ButtonsDown(0) & PAD_BUTTON_B) {
        show_scroller = !show_scroller;
    }
    else if (PAD_ButtonsDown(0) & PAD_BUTTON_RIGHT) {
        mirror_alpha=(mirror_alpha==255)?255:mirror_alpha+1;
        GRRLIB_GeckoPrintf("mirror_alpha: %d\n", mirror_alpha);
    }
    else if (PAD_ButtonsDown(0) & PAD_BUTTON_LEFT) {
        mirror_alpha=(mirror_alpha==0)?0:mirror_alpha-1;
        GRRLIB_GeckoPrintf("mirror_alpha: %d\n", mirror_alpha);
    }
}

void screen_update(void) {

    if(alpha <= 0 || alpha >= 255)
        alpha_dir = -alpha_dir;
    alpha += alpha_dir;
    a+=0.5f;
    update_scroll(scroller);

    // distort
    sinx += 0.02f;
}

void screen_render(void) {

    // clear screen
    // GRRLIB_FillScreen(0x0);

    // Resetting Vars
    GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);

    if(show_cube == true) {
        GRRLIB_CompoStart();
        {
            // Switch To 3D Mode
            GRRLIB_3dMode(0.1, 1000, 45, 0, 0);
            GRRLIB_ObjectView(0, cubeY, cubeZ, a,a*2,a*3,1,1,1);
            GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
            {
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
            }
            GX_End();
            // GRRLIB_Screen2Texture(0, 0, tex_screen3, 1);
        }
        GRRLIB_CompoEnd(0, 0, tex_screen3);
    }

    // Switch To 2D Mode to display text and background
    GRRLIB_2dMode();

    if(show_scroller == true) {
        // scroller in a render target
        GRRLIB_CompoStart();
        {
            display_scroll(scroller);
        }
        GRRLIB_CompoEnd(0, 0, tex_screen1);
    }

    // Drawing Background and text
    GRRLIB_DrawImg( 0, 0, logo_background, 0, 1, 1, RGBA(255, 255, 255, alpha) );
    GRRLIB_Printf((640-(16*29))/2, 20, text_font, 0xFFFFFFFF, 1, "PRESS A OR B TO ZOOM THE CUBE");

    if(show_cube == true) {
        // distort cube
        float tmp_sinx = sinx;
        for(int i=0; i<rmode->efbHeight; i++) {
            GRRLIB_DrawTile(0+sin(tmp_sinx)*60, i+120, tex_screen3, 0, 1, 1, 0xFFFFFFFF, i);
            tmp_sinx += 0.02f;
        }
    }

    if(show_scroller == true) {
        GRRLIB_DrawImg(0, 300, tex_screen1, 0, 1, 1, 0xFFFFFFFF);
        GRRLIB_BMFX_FlipV(tex_screen1, tex_screen2);
        GRRLIB_FlushTex(tex_screen2);

        if(APPLY_MIRROR_WAVE == true) {
            float tmp_sinx = sinx;
            int alpha = mirror_alpha;
            for(int i=0; i<rmode->efbHeight/2; i++) {
                GRRLIB_DrawTile(5+cos(tmp_sinx*10)*5, i+(rmode->efbHeight - 80), tex_screen2, 0, 1, 1, RGBA(255, 255, 255, alpha), 2*i);
                tmp_sinx += 0.03f;
                alpha = (alpha<=0)?0:mirror_alpha-(2*i);
            }
        }
        else {
            GRRLIB_DrawImg(0, (rmode->efbHeight - 80), tex_screen2, 0, 1, 0.5, RGBA(255, 255, 255, mirror_alpha));
        }

    }

    GRRLIB_Render();
}