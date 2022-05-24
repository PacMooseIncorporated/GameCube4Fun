#include <grrlib.h>

#include <stdlib.h>
#include <math.h>
#include <malloc.h>

#include "scroller.h"

#define NB_LETTERS 10

//--------------------------------------------------------------------------------
// Create am horizontal text scroller with an optional sine effect
//--------------------------------------------------------------------------------
t_scroller * create_scroller(GRRLIB_texImg * font, int speed, int interspace, u32 x, u32 y, char * text) {

    t_scroller * scroller = malloc(sizeof(t_scroller));
    scroller->speed = speed;
    scroller->x = x;
    scroller->y = y;
    scroller->interspace = interspace;

    scroller->font = font;
    scroller->text = strdup(text);
    scroller->pos = NB_LETTERS;
    scroller->letters = malloc(NB_LETTERS*sizeof(t_letter *));
    scroller->x_min_limit = 4 - scroller->font->tilew;

    for(int i=0; i < NB_LETTERS; i++){
        t_letter * letter = malloc(sizeof(t_letter));
        letter->x_pos = (font->tilew + interspace)*i;
        letter->y_pos = y;

        letter->text_ptr = &text[i];
        scroller->letters[i] = letter;
    }

    kprintf("Scroller created with font of size: (%d, %d)\n", scroller->font->tilew, scroller->font->tileh);
    kprintf("Scroller x limit: %d\n", scroller->x_min_limit);

    return scroller;
}

//--------------------------------------------------------------------------------
// Render scroll
//--------------------------------------------------------------------------------
void render_scroll(t_scroller * scroller) {

    // draw letter by letter
    for(int i=0; i < NB_LETTERS; i++) {
        GRRLIB_DrawTile(scroller->letters[i]->x_pos, scroller->letters[i]->y_pos, scroller->font, 0, 1, 1, 0xFFFFFFFF, *(scroller->letters[i]->text_ptr)-32);
    }
}

//--------------------------------------------------------------------------------
// Update scroll
//--------------------------------------------------------------------------------
void update_scroll(t_scroller * scroller) {

    for(int i=0; i < NB_LETTERS; i++) {
        // scroll !
        scroller->letters[i]->x_pos -= scroller->speed;

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

//--------------------------------------------------------------------------------
// Free scroll resources
//--------------------------------------------------------------------------------
void free_scroller(t_scroller * scroller) {

    kprintf("Free scroller resources\n");
    free(scroller->text);
    free(scroller->letters);
    free(scroller);
}