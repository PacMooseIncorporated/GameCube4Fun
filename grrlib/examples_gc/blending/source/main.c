/*===========================================
        GRRLIB (GX Version)
        Example code by Xane

        This example shows the different
        new blending modes.
============================================*/
#include <grrlib.h>

#include <stdlib.h>
#include <math.h>

// Include Graphics
#include "GFX/RGFX_Background.h"
#include "GFX/RGFX_Blob01.h"
#include "GFX/RGFX_Blob02.h"
#include "GFX/RGFX_Blob03.h"
#include "GFX/RGFX_Font.h"

// Declare Static Functions
static void ExitGame();

// Prepare Graphics
GRRLIB_texImg *GFX_Background;
GRRLIB_texImg *GFX_Blob[3];
GRRLIB_texImg *GFX_Font;


int main() {
    // Init Variables
    u32 PADKeyDown;

    u8 Stage = 0;
    u8 Blending = 0;
    u8 BlobType = 0;
    u8 Color;
    u16 Step = 0;

    // Init GRRLIB
    GRRLIB_Init();
    PAD_Init();

    // Load Textures
    GFX_Background = GRRLIB_LoadTextureJPG(RGFX_Background);
    GFX_Blob[0]    = GRRLIB_LoadTexturePNG(RGFX_Blob01);
    GFX_Blob[1]    = GRRLIB_LoadTexturePNG(RGFX_Blob02);
    GFX_Blob[2]    = GRRLIB_LoadTexturePNG(RGFX_Blob03);
    GFX_Font       = GRRLIB_LoadTexturePNG(RGFX_Font);
    GRRLIB_InitTileSet(GFX_Font, 8, 16, 32);

    // Set Handles
    GRRLIB_SetMidHandle( GFX_Blob[0], true );
    GRRLIB_SetMidHandle( GFX_Blob[1], true );
    GRRLIB_SetMidHandle( GFX_Blob[2], true );


    while (true) {
        PAD_ScanPads();
        PADKeyDown = PAD_ButtonsDown(PAD_CHAN0);

        // Update Stage
        Step = Step + 1;
        if (Step == 720) {
            Step = 0;
        }
        float SX = 320 + (sin(DegToRad(Step  )) * 250);
        float SY = 240 + (cos(DegToRad(Step*3)) * 100);

        // Draw Stage
        GRRLIB_DrawImg( 0, 0, GFX_Background, 0, 1, 1, RGBA(255, 255, 255, 255) );
        GRRLIB_SetBlend( (Blending + 1) );
        switch (Stage) {
            case 2: Color = 160; break;
            case 3: Color = 128; break;
            case 4: Color = 64;  break;
            default: Color = 255; break;
        }
        GRRLIB_DrawImg( SX, SY, GFX_Blob[BlobType], 0, 1, 1, RGBA(Color, Color, Color, 255) );

        // Draw Text
        GRRLIB_SetBlend ( GRRLIB_BLEND_ALPHA );
        GRRLIB_Rectangle( 28, 28, 480 + 16, 76, RGBA(0, 0, 0, 160), 1 );
        GRRLIB_Printf   ( 32, 48, GFX_Font, 0xFFFFFFFF, 1, "Press LEFT and RIGHT to switch through the different stages." );
        GRRLIB_Printf   ( 32, 64, GFX_Font, 0xFFFFFFFF, 1, "Press A to change the blob sprite." );
        switch (Stage) {
            case 0: GRRLIB_Printf( 32, 88, GFX_Font, 0xFFFFFFFF, 1, "Stage 1: Additive Blending" );       Blending = 0; break;
            case 1: GRRLIB_Printf( 32, 88, GFX_Font, 0xFFFFFFFF, 1, "Stage 2: Alpha Light Blending" );    Blending = 1; break;
            case 2: GRRLIB_Printf( 32, 88, GFX_Font, 0xFFFFFFFF, 1, "Stage 3: Multiply Blending (75%)" ); Blending = 2; break;
            case 3: GRRLIB_Printf( 32, 88, GFX_Font, 0xFFFFFFFF, 1, "Stage 4: Multiply Blending (50%)" ); Blending = 2; break;
            case 4: GRRLIB_Printf( 32, 88, GFX_Font, 0xFFFFFFFF, 1, "Stage 5: Multiply Blending (25%)" ); Blending = 2; break;
            case 5: GRRLIB_Printf( 32, 88, GFX_Font, 0xFFFFFFFF, 1, "Stage 6: Invert Color Blending" );   Blending = 3; break;
        }

        GRRLIB_Render();
        if (PADKeyDown & PAD_BUTTON_RIGHT) {
            if (Stage < 5) {
                Stage += 1;
            }
        }
        if (PADKeyDown & PAD_BUTTON_LEFT) {
            if (Stage > 0) {
                Stage -= 1;
            }
        }
        if (PADKeyDown & PAD_BUTTON_A) {
            BlobType += 1;
            if (BlobType > 2) {
                BlobType = 0;
            }
        }
        if (PADKeyDown & PAD_BUTTON_START) {
            break;
        }
    }
    ExitGame();
    return 0;
}

static void ExitGame() {
    // Free all memory used by textures.
    GRRLIB_FreeTexture(GFX_Background);
    GRRLIB_FreeTexture(GFX_Blob[0]);
    GRRLIB_FreeTexture(GFX_Blob[1]);
    GRRLIB_FreeTexture(GFX_Blob[2]);
    GRRLIB_FreeTexture(GFX_Font);

    // Deinitialize GRRLIB & Video
    GRRLIB_Exit();

    // Exit application
    exit(0);
}
