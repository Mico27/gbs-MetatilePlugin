#ifndef __STATESCALLER_H_INCLUDE
#define __STATESCALLER_H_INCLUDE

#include <gbdk/platform.h>

void state_init(void);
void state_update(void);

extern UBYTE pause_state_update;

#endif