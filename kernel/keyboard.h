#ifndef ZEROOS_KEYBOARD_H
#define ZEROOS_KEYBOARD_H

#include <zeroos/types.h>

void keyboard_init(void);
char keyboard_getchar(void);
bool keyboard_has_key(void);

#endif
