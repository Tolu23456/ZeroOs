#ifndef ZEROOS_TIMER_H
#define ZEROOS_TIMER_H

#include <zeroos/types.h>

void timer_init(uint32_t freq);
uint64_t timer_get_ticks(void);
void timer_sleep(uint64_t ms);

#endif
