#ifndef REORDER_H
#define REORDER_H

#include <gbdk/platform.h>
#include "gbs_types.h"

void reorder_all(void) BANKED;
void temp_deactivate_all(void) BANKED;
void temp_activate_all(void) BANKED;

#endif
