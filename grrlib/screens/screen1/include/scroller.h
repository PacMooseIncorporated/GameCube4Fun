#ifndef __SCROLLER_H
#define __SCROLLER_H

#include <grrlib.h>

typedef struct letter {
    int x_pos;
    int y_pos;
    char * text_ptr;
} t_letter;

typedef struct scroller {
    int speed;
    int interspace;
    int x;
    int y;
    int x_min_limit;
    char * text;
    t_letter ** letters;
    int pos;

    GRRLIB_texImg * font;

} t_scroller;

t_scroller * create_scroller(GRRLIB_texImg * font, int speed, int interspace, u32 x, u32 y, char * text);
void render_scroll(t_scroller * scroller);
void update_scroll(t_scroller * scroller);
void free_scroller(t_scroller * scroller);

#endif /* __SCROLLER_H */