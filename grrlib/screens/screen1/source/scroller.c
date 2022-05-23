#include <grrlib.h>

#include <stdlib.h>
#include <math.h>
#include <malloc.h>

#include "scroller.h"

#define NB_LETTERS 10

float t = 0;

t_scroller * create_scroller(GRRLIB_texImg * font, int speed, int interspace, u32 x, u32 y, char * text, bool apply_sin, int sin_factor, float sin_speed) {

    GRRLIB_FlushTex(font);

    t_scroller * scroller = malloc(sizeof(t_scroller));
    scroller->speed = speed;
    scroller->x = x;
    scroller->y = y;
    scroller->interspace = interspace;
    scroller->apply_sin = apply_sin;
    scroller->sin_factor = sin_factor;
    scroller->sin_speed = sin_speed;

    scroller->font = font;
    scroller->text = strdup(text);
    scroller->pos = NB_LETTERS;
    scroller->letters = malloc(NB_LETTERS*sizeof(t_letter *));
    scroller->x_min_limit = 4 - scroller->font->tilew;

    for(int i=0; i < NB_LETTERS; i++){
        t_letter * letter = malloc(sizeof(t_letter));
        letter->x_pos = (font->tilew + interspace)*i;

        if(apply_sin == true)
            letter->y_pos = (int)(y + (sin(scroller->sin_speed + (i*M_2_PI)))*scroller->sin_factor);
        else
            letter->y_pos = y;

        letter->text_ptr = &text[i];
        scroller->letters[i] = letter;
    }

    kprintf("Scroller created with font of size: (%d, %d)\n", scroller->font->tilew, scroller->font->tileh);
    kprintf("Scroller x limit: %d\n", scroller->x_min_limit);

    return scroller;
}

void display_scroll(t_scroller * scroller) {

    for(int i=0; i < NB_LETTERS; i++) {
        GRRLIB_DrawTile(scroller->letters[i]->x_pos, scroller->letters[i]->y_pos, scroller->font, 0, 1, 1, 0xFFFFFFFF, *(scroller->letters[i]->text_ptr)-32);
    }
}

void update_scroll(t_scroller * scroller) {

    t += 0.1;

    for(int i=0; i < NB_LETTERS; i++) {
        // scroll !
        scroller->letters[i]->x_pos -= scroller->speed;

        if(scroller->apply_sin == true)
            scroller->letters[i]->y_pos = (int)(scroller->y + (sin(t*scroller->sin_speed + (i*M_2_PI)))*scroller->sin_factor);

        // check overflow
        if(scroller->letters[i]->x_pos < scroller->x_min_limit) {
            int idx = (i==0)?NB_LETTERS-1:i-1;

            // wrap
            if(scroller->pos == strlen(scroller->text) - 1)
                scroller->pos = 0;
            else
                scroller->pos++;
            scroller->letters[i]->text_ptr = &scroller->text[scroller->pos];

            // new x pos
            scroller->letters[i]->x_pos = scroller->letters[idx]->x_pos + scroller->interspace + scroller->font->tilew;
            scroller->letters[i]->y_pos = scroller->y;
        }
    }
}

void free_scroller(t_scroller * scroller) {
    free(scroller->text);
    free(scroller->letters);
    free(scroller);
}