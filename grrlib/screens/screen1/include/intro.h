#ifndef __INTRO_H
#define __INTRO_H

#include <stdbool.h>

void screen_init(void);
void screen_setup(void);
void screen_exit(void);
void screen_events(void);
void screen_update(void);
void screen_render(void);
bool screen_enabled(void);

#endif /* __INTRO_H */