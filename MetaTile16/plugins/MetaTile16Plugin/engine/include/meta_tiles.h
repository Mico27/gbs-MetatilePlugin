#ifndef META_TILES_H
#define META_TILES_H

/*
SRAM_MAP_WIDTH and SRAM_MAP_HEIGHT must be multiple of 2 for performance reason when calculating METATILE_MAP_OFFSET.
METATILE_MAP_OFFSET must be modified to reflect SRAM_MAP_WIDTH and SRAM_MAP_HEIGHT values.
The product of SRAM_MAP_WIDTH and SRAM_MAP_HEIGHT must not exceed the SRAM max size of 8192.
The bigger the MAX_MAP_DATA_SIZE, the less space there will be for save data.
Scenes can be smaller than the SRAM_MAP size but cannot be bigger then it.
Dont forget to change the values in collision.h too
*/
#define SRAM_MAP_WIDTH 128
#define SRAM_MAP_HEIGHT 16
#define METATILE_MAP_OFFSET(x,y)  ((((y >> 1) & 15) << 7) + ((x >> 1) & 127))
#define TILE_MAP_OFFSET(metatile_idx,x,y)  (((metatile_idx >> 4) << 6) + ((metatile_idx & 15) << 1) + ((y & 1) << 5) + (x & 1))

#include <gbdk/platform.h>
#include "gbs_types.h"
#include "vm.h"

#define MAX_MAP_DATA_SIZE (SRAM_MAP_WIDTH * SRAM_MAP_HEIGHT)

extern uint8_t __at(0xB400) sram_collision_data[1024]; //sram_map_data Address 0xBC00 - 0x0400(1024)
extern uint8_t __at(0xB800) sram_map_data[MAX_MAP_DATA_SIZE]; //0xA000 + (0x2000 (8k SRAM max size) - 0x0800 (MAX_MAP_DATA_SIZE))

extern UBYTE metatile_bank;
extern unsigned char* metatile_ptr;

extern UBYTE metatile_attr_bank;
extern unsigned char* metatile_attr_ptr;

extern UBYTE metatile_collision_bank;
extern unsigned char* metatile_collision_ptr;


void replace_meta_tile(UBYTE x, UBYTE y, UBYTE tile_id, UBYTE commit) BANKED;

#endif
