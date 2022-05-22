#include <grrlib.h>

typedef struct letter {
    int x_pos;
    int y_pos;
    char * text_ptr;
} t_letter;

typedef struct scroller {
    int speed;
    int interspace;
    bool apply_sin;
    int sin_factor;
    float sin_speed;
    int x;
    int y;
    int x_min_limit;
    char * text;
    t_letter ** letters;
    int pos;

    GRRLIB_texImg * font;

} t_scroller;

t_scroller * create_scroller(GRRLIB_texImg * font, int speed, int interspace, u32 x, u32 y, char * text, bool apply_sin, int sin_factor, float sin_speed);
void display_scroll(t_scroller * scroller);
void update_scroll(t_scroller * scroller);
void free_scroller(t_scroller * scroller);