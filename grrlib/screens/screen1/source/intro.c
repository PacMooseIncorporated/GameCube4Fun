
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
#define SCROLL_SPEED 8
#define SCROLL_INTERLACE 10

bool time_to_exit = false;
float rot;
u32 col[3] = {0xFFFFFFFF, 0xAAAAAAFF, 0x666666FF};
int cubeZ;
int cubeY;
float sinx=0;

GRRLIB_texImg * logo_background;
GRRLIB_texImg * text_font;
GRRLIB_texImg * scroll_font;
GRRLIB_texImg * off_scroller;
GRRLIB_texImg * off_mirror;
GRRLIB_texImg * off_3d;
GRRLIB_texImg * off_sine;

int alpha = 1;
int alpha_dir = 1;
u8 mirror_alpha = 0x80;
char text[] = TEXT;
t_scroller * scroller;

bool show_cube = true;
bool show_scroller = true;
bool show_mirror_wave  = true;
bool show_mirror  = true;

#define SIN_PERIOD 2
#define SIN_INCR 0.05
#define SIN_FACTOR 20
float sin_t = 0;

//---------------------------------------------------------------------------------
// patch to fix buggy GRRLIB
//--------------------------------------------------------------------------------
void CompoStart (void) {
    GX_SetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
    GX_PokeAlphaRead(GX_READ_NONE);
}

void CompoEnd(int posx, int posy, GRRLIB_texImg *tex) {

    GX_SetTexCopySrc(posx, posy, tex->w, tex->h);
    GX_SetTexCopyDst(tex->w, tex->h, GX_TF_RGBA8, GX_FALSE);

    GX_SetCopyClear((GXColor){ 0, 0, 0, 0 }, GX_MAX_Z24);
    GX_CopyTex(tex->data, GX_TRUE);
    GX_PixModeSync();
    GRRLIB_FlushTex(tex);
    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
}

//---------------------------------------------------------------------------------
// Bah
//--------------------------------------------------------------------------------
bool screen_enabled(void){
    return time_to_exit;
}

//---------------------------------------------------------------------------------
// Init, load stuff
//--------------------------------------------------------------------------------
void screen_init(void) {

    // 3D
    rot = 0;
    cubeZ = 0;
    cubeY = 3;
    sinx = 0;

    // Load textures
    logo_background = GRRLIB_LoadTexturePNG(logo_png);

    // small font tileset
    text_font = GRRLIB_LoadTexture(font_png);
    GRRLIB_InitTileSet(text_font, 16, 16, 32);

    // scroller font tileset
    scroll_font = GRRLIB_LoadTexture(font3d_png);
    GRRLIB_InitTileSet(scroll_font, 64, 64, 32);

    // scroller offscreen texture
    off_scroller = GRRLIB_CreateEmptyTexture(rmode->fbWidth, 64);
    GRRLIB_InitTileSet(off_scroller, 1, 64, 0);

    // scroller sinewave offscreen texture
    off_sine = GRRLIB_CreateEmptyTexture(rmode->fbWidth, 128);

    // scroller mirror offscreen texture
    off_mirror = GRRLIB_CreateEmptyTexture(rmode->fbWidth, 128);
    GRRLIB_InitTileSet(off_mirror, rmode->fbWidth, 1, 0);

    // 3D distort offscreen texture
    off_3d = GRRLIB_CreateEmptyTexture(rmode->fbWidth, 200);
    GRRLIB_InitTileSet(off_3d, rmode->fbWidth, 1, 0);

    // Scroller
    scroller = create_scroller(scroll_font, SCROLL_SPEED, SCROLL_INTERLACE, 0, 0, text);
}

//---------------------------------------------------------------------------------
// Setup before rendering
//---------------------------------------------------------------------------------
void screen_setup() {

    GRRLIB_Settings.antialias = true;

    GRRLIB_SetBackgroundColour(0x00, 0x00, 0x00, 0xFF);
    GRRLIB_Camera3dSettings(0.0f, 0.0f, 13.0f, 0,1,0, 0,0,0);
}

//---------------------------------------------------------------------------------
// Exit
//--------------------------------------------------------------------------------
void screen_exit() {
    free_scroller(scroller);
    GRRLIB_FreeTexture(text_font);
    GRRLIB_FreeTexture(scroll_font);
    GRRLIB_FreeTexture(logo_background);
    GRRLIB_FreeTexture(off_scroller);
    GRRLIB_FreeTexture(off_mirror);
    GRRLIB_FreeTexture(off_3d);
}

//---------------------------------------------------------------------------------
// Joystick events
//--------------------------------------------------------------------------------
void screen_events() {

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
    else if (PAD_ButtonsDown(0) & PAD_BUTTON_Y) {
        show_mirror = !show_mirror;
    }
    else if (PAD_ButtonsDown(0) & PAD_BUTTON_X) {
        show_mirror_wave = !show_mirror_wave;
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

//---------------------------------------------------------------------------------
// Update
//--------------------------------------------------------------------------------
void screen_update(void) {

    // background alpha
    if(alpha <= 0 || alpha >= 255)
        alpha_dir = -alpha_dir;
    alpha += alpha_dir;

    // 3D rotation
    rot += 0.5f;
    // distort (cube and mirror)
    sinx += 0.02f;

    // scroller update
    update_scroll(scroller);
}

//--------------------------------------------------------------------------------
// Render
//--------------------------------------------------------------------------------
void screen_render(void) {

    // Resetting Vars
    GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);

    // ******************************************
    // Offscreen rendering
    // ******************************************

    // Render the cube in a render target
    if(show_cube == true) {
        CompoStart();
        {
            // Switch To 3D Mode
            GRRLIB_3dMode(0.1, 1000, 45, 0, 0);
            GRRLIB_ObjectView(0, cubeY, cubeZ, rot,rot*2,rot*3,1,1,1);
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
        }
        CompoEnd(0, 0, off_3d);
    }
    else {
        CompoStart();
        {
            // Switch To 3D Mode
            GRRLIB_3dMode(0.1, 1000, 45, 0, 1);
            GRRLIB_ObjectView(0,0,0, rot,rot*2,rot*3,1,1,1);
            GRRLIB_DrawTorus(1, 2, 60, 60, true, 0xFFFFFFFF);
        }
        CompoEnd(0, 0, off_3d);
    }

    // Switch To 2D Mode to display text and background
    GRRLIB_2dMode();

    // render scroller offscreen
    if(show_scroller == true) {

        CompoStart();
        {
            render_scroll(scroller);
        }
        CompoEnd(0, 0, off_scroller);

        // apply distort on horizontal scroller offscreen
        CompoStart();
        {
            sin_t += SIN_INCR;
            // blit scroller by row
            for(int j=0; j < rmode->fbWidth; j++) {
                float y_sin = sin((sin_t + j* SIN_PERIOD * M_PI / 320));
                int y_pos = (int)(scroller->y + y_sin * SIN_FACTOR);
                GRRLIB_DrawTile(j, y_pos+32, off_scroller, 0, 1, 1, 0xFFFFFFFF, j);
            }
        }
        CompoEnd(0, 0, off_sine);
    }

    // ******************************************
    // Onscreen rendering
    // ******************************************

    // Drawing Background and text
    GRRLIB_DrawImg( 0, 0, logo_background, 0, 1, 1, RGBA(255, 255, 255, alpha) );
    GRRLIB_Printf((640-(16*27))/2, 20, text_font, 0xFFFFFFFF, 1, "PRESS A, B, X OR PAD ARROWS");

    // render cube line per line to apply a sine effect
    if(show_cube == true) {
        float tmp_sinx = sinx;
        for(int i=0; i<rmode->efbHeight; i++) {
            GRRLIB_DrawTile(0+sin(tmp_sinx)*60, i+120, off_3d, 0, 1, 1, 0xFFFFFFFF, i);
            tmp_sinx += 0.02f;
        }
    }

    if(show_scroller == true) {

        // display offscreen texture
        GRRLIB_DrawImg(0, 300, off_sine, 0, 1, 1, 0xFFFFFFFF);

        if(show_mirror == true) {
            // apply V-flip effect on scroller offscreen texture then a wave effect
            GRRLIB_BMFX_FlipV(off_sine, off_mirror);
            GRRLIB_FlushTex(off_mirror);

            if(show_mirror_wave == true) {

                float tmp_sinx = sinx;
                int alpha = mirror_alpha;
                for(int i=0; i<rmode->efbHeight/2; i++) {
                    GRRLIB_DrawTile(5+cos(tmp_sinx*10)*5, i+(rmode->efbHeight - 80), off_mirror, 0, 1, 1, RGBA(255, 255, 255, alpha), 2*i);
                    tmp_sinx += 0.03f;
                    alpha = (alpha<=0)?0:mirror_alpha-(2*i);
                }
            }
            else {
                GRRLIB_DrawImg(0, (rmode->efbHeight - 100), off_mirror, 0, 1, 0.5, RGBA(255, 255, 255, mirror_alpha));
            }
        }
    }

    // render all this crap
    GRRLIB_Render();
}