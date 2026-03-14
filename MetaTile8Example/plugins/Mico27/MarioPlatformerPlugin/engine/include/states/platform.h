#ifndef STATE_PLATFORM_H
#define STATE_PLATFORM_H

#include <gb/gb.h>

void platform_init(void) BANKED;
void platform_update(void) BANKED;

extern WORD plat_grav; 
extern WORD plat_max_fall_vel; 

#endif
