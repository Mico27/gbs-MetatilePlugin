#pragma bank 255

#include "scroll.h"

#include <string.h>

#include "system.h"
#include "actor.h"
#include "camera.h"
#include "data_manager.h"
#include "game_time.h"
#include "math.h"
#include "fade_manager.h"
#include "parallax.h"
#include "palette.h"
#include "meta_tiles.h"
#include "continuous_scene.h"
#include "data/game_globals.h"
#include "vm.h"
#include "macro.h"
#include "data/states_defines.h"

// put submap of a large map to screen
void set_bkg_submap(UINT8 x, UINT8 y, UINT8 w, UINT8 h, const unsigned char *map, UINT8 map_w) OLDCALL;

void scroll_queue_row(UBYTE x, UBYTE y);
void scroll_queue_col(UBYTE x, UBYTE y);
void scroll_load_pending_row(void);
void scroll_load_pending_col(void);
void scroll_load_row(UBYTE x, UBYTE y);
void scroll_load_col(UBYTE x, UBYTE y, UBYTE height);
UBYTE scroll_viewport(parallax_row_t * port);

INT16 scroll_x;
INT16 scroll_y;
INT16 draw_scroll_x;
INT16 draw_scroll_y;
UINT16 scroll_x_min;
UINT16 scroll_y_min;
UINT16 scroll_x_max;
UINT16 scroll_y_max;
UBYTE bkg_scroll_x;
UBYTE bkg_scroll_y;
BYTE scroll_offset_x;
BYTE scroll_offset_y;
BYTE bkg_offset_x;
BYTE bkg_offset_y;
UBYTE pending_h_x, pending_h_y;
UBYTE pending_h_i;
UBYTE pending_w_x, pending_w_y;
UBYTE pending_w_i;
UBYTE current_row, new_row;
UBYTE current_col, new_col;

UBYTE scroll_render_disabled;

UWORD bkg_address_offset;

static FASTUBYTE _save_bank;

void scroll_init(void) BANKED {
    draw_scroll_x   = 0;
    draw_scroll_y   = 0;
    scroll_x_min    = 0;
    scroll_y_min    = 0;
    scroll_x_max    = 0;
    scroll_y_max    = 0;
    scroll_offset_x = 0;
    scroll_offset_y = 0;
    bkg_offset_x = 0;
    bkg_offset_y = 0;
    bkg_scroll_x = 0;
    bkg_scroll_y = 0;
    scroll_render_disabled = 0;
    scroll_reset();
}

void scroll_reset(void) BANKED {
    pending_w_i     = 0;
    pending_h_i     = 0;
    if (!is_transitioning_scene){
        scroll_x = 0x400;
        scroll_y = 0x400;
        bkg_offset_x = 0;
        bkg_offset_y = 0;
    }
}

void scroll_update(void) BANKED {
    INT16 x, y;
    UBYTE render = FALSE;

    x = SUBPX_TO_PX((UWORD)camera_x) - (SCREENWIDTH >> 1);
    y = SUBPX_TO_PX((UWORD)camera_y) - (SCREENHEIGHT >> 1);

#ifndef DISABLE_SCROLL_LIMITS
    if (!(continuous_scene_enabled & DIRECTION_LEFT_FLAG) && (((UWORD)x > SCREEN_OOB_LEFT_PX) || (x < scroll_x_min))) {
        x = scroll_x_min;
    } else if (!(continuous_scene_enabled & DIRECTION_RIGHT_FLAG) && (((UWORD)x < SCREEN_OOB_LEFT_PX) && (x > scroll_x_max))) {
        x = scroll_x_max;
    }
    if (!(continuous_scene_enabled & DIRECTION_TOP_FLAG) && (((UWORD)y > SCREEN_OOB_TOP_PX) || (y < scroll_y_min))) {
        y = scroll_y_min;
    } else if (!(continuous_scene_enabled & DIRECTION_BOTTOM_FLAG) && (((UWORD)y < SCREEN_OOB_TOP_PX) && (y > scroll_y_max))) {
        y = scroll_y_max;
    }
#endif

    current_col = PX_TO_TILE(scroll_x);
    current_row = PX_TO_TILE(scroll_y);
    new_col = PX_TO_TILE(x);
    new_row = PX_TO_TILE(y);

    scroll_x = x;
    scroll_y = y;
    draw_scroll_x = x + scroll_offset_x;
    draw_scroll_y = y + scroll_offset_y;
    bkg_scroll_x = (draw_scroll_x + TILE_TO_PX(bkg_offset_x));
    bkg_scroll_y = (draw_scroll_y + TILE_TO_PX(bkg_offset_y));

    if (scroll_viewport(parallax_rows)) return;
    if (scroll_viewport(parallax_rows + 1)) return;
    scroll_viewport(parallax_rows + 2);
}

UBYTE scroll_viewport(parallax_row_t * port) {
    if (port->next_y) {
        // one of upper parallax slices
        UINT16 shift_scroll_x;
        if (port->shift == 127) {
            shift_scroll_x = 0;
        } else if (port->shift < 0) {
            shift_scroll_x = draw_scroll_x << (-port->shift);
        } else {
            shift_scroll_x = draw_scroll_x >> port->shift;
        }

        port->shadow_scx = shift_scroll_x;
        if (scroll_render_disabled) return FALSE;
        UBYTE shift_col = PX_TO_TILE(shift_scroll_x);

        // If column is +/- 1 just render next column
        if (current_col == (UBYTE)(new_col - 1)) {
            // Render right column
            UBYTE x = shift_col - SCREEN_PAD_LEFT + SCREEN_TILE_REFRES_W - 1;
            scroll_load_col(x, port->start_tile, port->tile_height);
        } else if (current_col == (UBYTE)(new_col + 1)) {
            // Render left column
            UBYTE x = shift_col - SCREEN_PAD_LEFT;
            scroll_load_col(x, port->start_tile, port->tile_height);
        } else if (current_col != new_col) {
            // If column differs by more than 1 render entire viewport
            scroll_render_rows(shift_scroll_x, 0, port->start_tile, port->tile_height);
        }
        return FALSE;
    } else {
        // last parallax slice OR no parallax
        port->shadow_scx = draw_scroll_x;

        // If column is +/- 1 just render next column
        if (current_col == (UBYTE)(new_col - 1)) {
            // Queue right column
            UBYTE x = new_col - SCREEN_PAD_LEFT + SCREEN_TILE_REFRES_W - 1;
            UBYTE y = (scene_LCD_type == LCD_parallax)? port->start_tile : (new_row - SCREEN_PAD_TOP);
            scroll_queue_col(x, y);
            x = (x >= SCREEN_OOB_LEFT) ? 0 : x;
            y = (new_row - SCREEN_PAD_TOP);
            y = (y >= SCREEN_OOB_TOP) ? 0 : y;
            activate_actors_in_col(x, y);
        } else if (current_col == (UBYTE)(new_col + 1)) {
            // Queue left column
            UBYTE x = new_col - SCREEN_PAD_LEFT;
            UBYTE y = (scene_LCD_type == LCD_parallax)? port->start_tile : (new_row - SCREEN_PAD_TOP);
            scroll_queue_col(x, y);
            x = (x >= SCREEN_OOB_LEFT) ? 0 : x;
            y = (new_row - SCREEN_PAD_TOP);
            y = (y >= SCREEN_OOB_TOP) ? 0 : y;
            activate_actors_in_col(x, y);
        } else if (current_col != new_col) {
            // If column differs by more than 1 render entire screen
            scroll_render_rows(draw_scroll_x, draw_scroll_y, ((scene_LCD_type == LCD_parallax) ? port->start_tile : -SCREEN_PAD_TOP), SCREEN_TILE_REFRES_H);
            return TRUE;
        } else if (pending_h_i) {
            scroll_load_pending_col();
        }

        // If row is +/- 1 just render next row
        if (current_row == (UBYTE)(new_row - 1)) {
            // Queue bottom row
            UBYTE x = new_col - SCREEN_PAD_LEFT;
            UBYTE y = new_row - SCREEN_PAD_TOP + SCREEN_TILE_REFRES_H - 1;
            scroll_queue_row(x, y);
            x = (x >= SCREEN_OOB_LEFT) ? 0 : x;
            y = (y >= SCREEN_OOB_TOP) ? 0 : y;     
            activate_actors_in_row(x, y);
        } else if (current_row == (UBYTE)(new_row + 1)) {
            // Queue top row
            UBYTE x = new_col - SCREEN_PAD_LEFT;
            UBYTE y = (scene_LCD_type == LCD_parallax) ? port->start_tile : (new_row - SCREEN_PAD_TOP);
            scroll_queue_row(x, y);
            x = (x >= SCREEN_OOB_LEFT) ? 0 : x;
            y = (y >= SCREEN_OOB_TOP) ? 0 : y;
            activate_actors_in_row(x, y);
        } else if (current_row != new_row) {
            // If row differs by more than 1 render entire screen
            scroll_render_rows(draw_scroll_x, draw_scroll_y, ((scene_LCD_type == LCD_parallax) ? port->start_tile : -SCREEN_PAD_TOP), SCREEN_TILE_REFRES_H);
            return TRUE;
        } else if (pending_w_i) {
            scroll_load_pending_row();
        }

        return TRUE;
    }
}

void scroll_repaint(void) BANKED {
    scroll_reset();
    scroll_update();
}

void scroll_render_rows(INT16 scroll_x, INT16 scroll_y, BYTE row_offset, BYTE n_rows) BANKED {
    // Clear pending rows/ columns
    pending_w_i = 0;
    pending_h_i = 0;

    if (scroll_render_disabled) return;

    UBYTE x = PX_TO_TILE(scroll_x) - SCREEN_PAD_LEFT;
    UBYTE y = PX_TO_TILE(scroll_y) + row_offset;
    if (scene_LCD_type == LCD_parallax) {
        n_rows = MIN(n_rows, image_tile_height - y);
    }

    for (BYTE i = 0; i != n_rows; ++i, y++) {
        scroll_load_row(x, y);
        activate_actors_in_row(((x >= SCREEN_OOB_LEFT) ? 0 : x), ((y >= SCREEN_OOB_TOP) ? 0 : y));
    }
}

void scroll_render_cols(INT16 scroll_x, INT16 scroll_y, BYTE col_offset, BYTE n_cols) BANKED {
    // Clear pending rows/ columns
    pending_w_i = 0;
    pending_h_i = 0;

    if (scroll_render_disabled) return;

    UBYTE x = PX_TO_TILE(scroll_x) + col_offset;
    UBYTE y = PX_TO_TILE(scroll_y) - SCREEN_PAD_TOP;
    UBYTE height = (scene_LCD_type == LCD_parallax) ? MIN(SCREEN_TILE_REFRES_H, image_tile_height - y) : SCREEN_TILE_REFRES_H;

    for (BYTE i = 0; i != n_cols; ++i, x++) {
        scroll_load_col(x, y, height);
        activate_actors_in_col(((x >= SCREEN_OOB_LEFT) ? 0 : x), ((y >= SCREEN_OOB_TOP) ? 0 : y));
    }
}

void scroll_queue_row(UBYTE x, UBYTE y) {

    while (pending_w_i) {
        // If previous row wasn't fully rendered
        // render it now before starting next row
        scroll_load_pending_row();
    }

    if (scroll_render_disabled) return;

    pending_w_x = x;
    pending_w_y = y;
    pending_w_i = SCREEN_TILE_REFRES_W;

    scroll_load_pending_row();
}

void scroll_queue_col(UBYTE x, UBYTE y) {

    while (pending_h_i) {
        // If previous column wasn't fully rendered
        // render it now before starting next column
        scroll_load_pending_col();
    }

    if (scroll_render_disabled) return;

    pending_h_x = x;
    pending_h_y = y;
    pending_h_i = (scene_LCD_type == LCD_parallax) ? MIN(SCREEN_TILE_REFRES_H, image_tile_height - y) : SCREEN_TILE_REFRES_H;
    scroll_load_pending_col();
}

// ============================================================
// Non-metatile tile rendering — current scene
// ============================================================

// Render a row section from any banked ROM tilemap into VRAM.
// Tiles outside the source bounds write oob_tile_id instead.
void load_tile_row(const unsigned char * from, UBYTE x, UBYTE y, UBYTE width, UBYTE source_width, UBYTE source_height, UBYTE oob_tile_id, UBYTE bank) NONBANKED {
    _save_bank = CURRENT_BANK;
    SWITCH_ROM(bank);
    UWORD y_offset = (y * (UWORD)source_width);
    width = width + x;
    for (x; x != width; x++) {
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), (x < source_width && y < source_height) ? *(from + y_offset + x) : oob_tile_id);
        bkg_address_offset = (bkg_address_offset & 0xFFE0) + ((bkg_address_offset + 1) & 31);
    }
    SWITCH_ROM(_save_bank);
}

// Render a column section from any banked ROM tilemap into VRAM.
void load_tile_col(const unsigned char * from, UBYTE x, UWORD y, UWORD height, UBYTE source_width, UBYTE source_height, UBYTE oob_tile_id, UBYTE bank) NONBANKED {
    _save_bank = CURRENT_BANK;
    SWITCH_ROM(bank);
    UWORD tile_offset = (y * (UINT16)source_width) + x;
    height = tile_offset + (height * (UINT16)source_width);
    for (tile_offset; tile_offset != height; tile_offset += (UINT16)source_width) {
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), (x < source_width && y < source_height) ? *(from + tile_offset) : oob_tile_id);
        bkg_address_offset = (bkg_address_offset + 32) & 1023;
        y++;
    }
    SWITCH_ROM(_save_bank);
}

// ============================================================
// Non-metatile fill — current scene
// ============================================================

// Fill a row section in VRAM with a constant raw tile ID.
static void fill_tile_row_plain(UBYTE width, UBYTE tile_id) {
    for (UBYTE x = 0; x != width; x++) {
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset & 0xFFE0) + ((bkg_address_offset + 1) & 31);
    }
}

// Fill a column section in VRAM with a constant raw tile ID.
static void fill_tile_col_plain(UBYTE height, UBYTE tile_id) {
    for (UBYTE y = 0; y != height; y++) {
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset + 32) & 1023;
    }
}

// ============================================================
// Metatile tile rendering — current scene (SRAM lookup)
// ============================================================

// Render a row section using the current scene's SRAM metatile map and banked VRAM tile table.
void load_metatile_row(const UBYTE* from, UBYTE x, UBYTE y, UBYTE width, UBYTE bank) NONBANKED {
    _save_bank = CURRENT_BANK;
    SWITCH_ROM(bank);
    UWORD metatile_y_offset = METATILE_Y_OFFSET(y);
#if METATILE_SIZE == METATILE_SIZE_16
    UBYTE tile_y_offset = TILE_Y_OFFSET(y);
#endif
    width = width + x;
    for (x; x != width; x++) {
#if METATILE_SIZE == METATILE_SIZE_16
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), *(from + get_metatile_tile(metatile_y_offset + METATILE_X_OFFSET(x), tile_y_offset + TILE_X_OFFSET(x))));
#else
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), *(from + (UWORD)sram_map_data[(metatile_y_offset + x)]));
#endif
        bkg_address_offset = (bkg_address_offset & 0xFFE0) + ((bkg_address_offset + 1) & 31);
    }
    SWITCH_ROM(_save_bank);
}

// Render a column section using the current scene's SRAM metatile map and banked VRAM tile table.
void load_metatile_col(const UBYTE* from, UBYTE x, UBYTE y, UBYTE height, UBYTE bank) NONBANKED {
    _save_bank = CURRENT_BANK;
    SWITCH_ROM(bank);
#if METATILE_SIZE == METATILE_SIZE_16
    UBYTE metatile_x_offset = METATILE_X_OFFSET(x);
    UBYTE tile_x_offset = TILE_X_OFFSET(x);
#endif
    height = height + y;
    for (y; y != height; y++) {
#if METATILE_SIZE == METATILE_SIZE_16
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), *(from + get_metatile_tile(METATILE_Y_OFFSET(y) + metatile_x_offset, TILE_Y_OFFSET(y) + tile_x_offset)));
#else
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), *(from + (UWORD)sram_map_data[METATILE_MAP_OFFSET(x, y)]));
#endif
        bkg_address_offset = (bkg_address_offset + 32) & 1023;
    }
    SWITCH_ROM(_save_bank);
}

// ============================================================
// Metatile fill — current scene
// ============================================================

// Fill a row section by resolving a metatile ID through the current scene's VRAM tile table.
static void fill_metatile_row(UBYTE x_offset, UBYTE y_offset, UBYTE width, UBYTE metatile_id) {
    for (UBYTE x = 0; x != width; x++) {
#if METATILE_SIZE == METATILE_SIZE_16
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), ReadBankedUBYTE(metatile_ptr + TILE_MAP_OFFSET(metatile_id, x_offset + x, y_offset), metatile_bank));
#else
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), ReadBankedUBYTE(metatile_ptr + metatile_id, metatile_bank));
#endif
        bkg_address_offset = (bkg_address_offset & 0xFFE0) + ((bkg_address_offset + 1) & 31);
    }
}

// Fill a column section by resolving a metatile ID through the current scene's VRAM tile table.
static void fill_metatile_col(UBYTE x_offset, UBYTE y_offset, UBYTE height, UBYTE metatile_id) {
    for (UBYTE y = 0; y != height; y++) {
#if METATILE_SIZE == METATILE_SIZE_16
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), ReadBankedUBYTE(metatile_ptr + TILE_MAP_OFFSET(metatile_id, x_offset, y_offset + y), metatile_bank));
#else
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), ReadBankedUBYTE(metatile_ptr + metatile_id, metatile_bank));
#endif
        bkg_address_offset = (bkg_address_offset + 32) & 1023;
    }
}

// ============================================================
// Fill dispatch — current scene (metatile or plain)
// ============================================================

// Fill a row section with fill_tile_id; treats it as a metatile ID when MetaTile is active.
void fill_tile_row(UBYTE x_offset, UBYTE y_offset, UBYTE width) {
    if (metatile_bank) {
        fill_metatile_row(x_offset, y_offset, width, fill_tile_id);
    } else {
        fill_tile_row_plain(width, fill_tile_id);
    }
}

// Fill a column section with fill_tile_id; treats it as a metatile ID when MetaTile is active.
void fill_tile_col(UBYTE x_offset, UBYTE y_offset, UBYTE height) {
    if (metatile_bank) {
        fill_metatile_col(x_offset, y_offset, height, fill_tile_id);
    } else {
        fill_tile_col_plain(height, fill_tile_id);
    }
}

// ============================================================
// Metatile tile rendering — neighbor scene (ROM tilemap lookup)
// ============================================================

// Render a row from a neighbor scene's ROM tilemap: reads the metatile ID via ReadBankedUBYTE,
// then resolves the VRAM tile index through the shared metatile_ptr table.
static void load_neighbor_metatile_row(const UBYTE* tilemap, UBYTE x, UBYTE y,
                                       UBYTE width, UBYTE source_width, UBYTE source_height, UBYTE bank) {
#if METATILE_SIZE == METATILE_SIZE_16
    UBYTE metatile_src_w = source_width >> 1;
    UWORD y_offset = ((UWORD)(y >> 1)) * metatile_src_w;
    width = width + x;
    for (x; x != width; x++) {
        UBYTE tile_id;
        if (x < source_width && y < source_height) {
            UBYTE mid = ReadBankedUBYTE(tilemap + y_offset + (x >> 1), bank);
            tile_id = ReadBankedUBYTE(metatile_ptr + TILE_MAP_OFFSET(mid, x, y), metatile_bank);
        } else {
            tile_id = ReadBankedUBYTE(metatile_ptr + TILE_MAP_OFFSET(fill_tile_id, x, y), metatile_bank);
        }
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset & 0xFFE0) + ((bkg_address_offset + 1) & 31);
    }
#else
    UWORD y_offset = (y * (UWORD)source_width);
    width = width + x;
    for (x; x != width; x++) {
        UBYTE tile_id;
        if (x < source_width && y < source_height) {
            UBYTE mid = ReadBankedUBYTE(tilemap + y_offset + x, bank);
            tile_id = ReadBankedUBYTE(metatile_ptr + mid, metatile_bank);
        } else {
            tile_id = ReadBankedUBYTE(metatile_ptr + fill_tile_id, metatile_bank);
        }
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset & 0xFFE0) + ((bkg_address_offset + 1) & 31);
    }
#endif
}

// Render a column from a neighbor scene's ROM tilemap via metatile lookup.
static void load_neighbor_metatile_col(const UBYTE* tilemap, UBYTE x, UWORD y,
                                       UWORD height, UBYTE source_width, UBYTE source_height, UBYTE bank) {
#if METATILE_SIZE == METATILE_SIZE_16
    UBYTE metatile_src_w = source_width >> 1;
    UBYTE metatile_x = x >> 1;
    UWORD end_y = y + height;
    for (y; y != end_y; y++) {
        UBYTE tile_id;
        if (x < source_width && y < source_height) {
            UWORD rom_offset = ((UWORD)(y >> 1)) * metatile_src_w + metatile_x;
            UBYTE mid = ReadBankedUBYTE(tilemap + rom_offset, bank);
            tile_id = ReadBankedUBYTE(metatile_ptr + TILE_MAP_OFFSET(mid, x, y), metatile_bank);
        } else {
            tile_id = ReadBankedUBYTE(metatile_ptr + TILE_MAP_OFFSET(fill_tile_id, x, y), metatile_bank);
        }
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset + 32) & 1023;
    }
#else
    UWORD tile_offset = (y * (UINT16)source_width) + x;
    height = tile_offset + (height * (UINT16)source_width);
    for (tile_offset; tile_offset != height; tile_offset += (UINT16)source_width) {
        UBYTE tile_id;
        if (x < source_width && y < source_height) {
            UBYTE mid = ReadBankedUBYTE(tilemap + tile_offset, bank);
            tile_id = ReadBankedUBYTE(metatile_ptr + mid, metatile_bank);
        } else {
            tile_id = ReadBankedUBYTE(metatile_ptr + fill_tile_id, metatile_bank);
        }
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset + 32) & 1023;
        y++;
    }
#endif
}

// ============================================================
// Tile rendering dispatch — neighbor scene
// ============================================================

// Render a row from a neighbor scene: uses metatile ROM lookup when MetaTile is active,
// otherwise reads the tile directly from the neighbor's banked tilemap.
static void neighbor_load_tile_row(continuous_scene_t* continuous_scene, UBYTE x, UBYTE y, UBYTE width) {
    if (metatile_bank) {
        load_neighbor_metatile_row(continuous_scene->tilemap.ptr, x, y, width, continuous_scene->tile_width, continuous_scene->tile_height, continuous_scene->tilemap.bank);
    } else {
        load_tile_row(continuous_scene->tilemap.ptr, x, y, width, continuous_scene->tile_width, continuous_scene->tile_height, fill_tile_id, continuous_scene->tilemap.bank);
    }
}

// Render a column from a neighbor scene: uses metatile ROM lookup when MetaTile is active,
// otherwise reads the tile directly from the neighbor's banked tilemap.
static void neighbor_load_tile_col(continuous_scene_t* continuous_scene, UBYTE x, UWORD y, UWORD height) {
    if (metatile_bank) {
        load_neighbor_metatile_col(continuous_scene->tilemap.ptr, x, y, height, continuous_scene->tile_width, continuous_scene->tile_height, continuous_scene->tilemap.bank);
    } else {
        load_tile_col(continuous_scene->tilemap.ptr, x, y, height, continuous_scene->tile_width, continuous_scene->tile_height, fill_tile_id, continuous_scene->tilemap.bank);
    }
}

void load_tile_row_continuous(UBYTE x, UBYTE y, UBYTE width) {
    // Used for continuous scene row rendering to adjust for different scene sizes and offsets
    bkg_address_offset = ((UWORD)get_bkg_xy_addr((x + bkg_offset_x), (y + bkg_offset_y))) - 0x9800;
    continuous_scene_t* continuous_scene;
    BYTE top_x_offset = (continuous_scene_enabled & DIRECTION_TOP_FLAG) ? continuous_scenes[DIRECTION_TOP].offset : 0;
    BYTE left_y_offset = (continuous_scene_enabled & DIRECTION_LEFT_FLAG) ? continuous_scenes[DIRECTION_LEFT].offset : 0;
    BYTE bottom_x_offset = (continuous_scene_enabled & DIRECTION_BOTTOM_FLAG) ? continuous_scenes[DIRECTION_BOTTOM].offset : 0;
    BYTE right_y_offset = (continuous_scene_enabled & DIRECTION_RIGHT_FLAG) ? continuous_scenes[DIRECTION_RIGHT].offset : 0;
    UBYTE section_width;
    if (x > SCREEN_OOB_LEFT){
        section_width = MIN(width, (UBYTE)(0 - x));
        if (y > SCREEN_OOB_TOP) {
            if (top_x_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_TOP];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_row(continuous_scene,
                        x + top_x_offset,
                        continuous_scene->tile_height + y,
                        section_width);
                } else {
                    fill_tile_row(x, y, section_width);
                }
            } else if (left_y_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_row(continuous_scene,
                        continuous_scene->tile_width + x,
                        y + left_y_offset,
                        section_width);
                } else {
                    fill_tile_row(x, y, section_width);
                }
            } else {
                continuous_scene = &continuous_scenes[DIRECTION_TOP_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_row(continuous_scene,
                        continuous_scene->tile_width + x + top_x_offset,
                        continuous_scene->tile_height + y + left_y_offset,
                        section_width);
                } else {
                    fill_tile_row(x, y, section_width);
                }
            }
        } else if (y < image_tile_height) {
            continuous_scene = &continuous_scenes[DIRECTION_LEFT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_row(continuous_scene,
                    continuous_scene->tile_width + x,
                    y + left_y_offset,
                    section_width);
            } else {
                fill_tile_row(x, y, section_width);
            }
        } else {
            if (bottom_x_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_row(continuous_scene,
                        x + bottom_x_offset,
                        (y - image_tile_height),
                        section_width);
                } else {
                    fill_tile_row(x, y, section_width);
                }
            } else if ((left_y_offset + continuous_scenes[DIRECTION_LEFT].tile_height) > image_tile_height){
                continuous_scene = &continuous_scenes[DIRECTION_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                     neighbor_load_tile_row(continuous_scene,
                        continuous_scene->tile_width + x,
                        y + left_y_offset,
                        section_width);
                } else {
                    fill_tile_row(x, y, section_width);
                }
            } else {
                continuous_scene = &continuous_scenes[DIRECTION_BOTTOM_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_row(continuous_scene,
                        continuous_scene->tile_width + x + bottom_x_offset,
                        (y - image_tile_height) + left_y_offset,
                        section_width);
                } else {
                    fill_tile_row(x, y, section_width);
                }
            }
        }
        width -= section_width;
        x += section_width;
        if (!width) return;
    }
    if (x < image_tile_width) {
        section_width = MIN(width, (image_tile_width - x));
        if (y > SCREEN_OOB_TOP) {
            continuous_scene = &continuous_scenes[DIRECTION_TOP];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_row(continuous_scene,
                    x + top_x_offset,
                    continuous_scene->tile_height + y,
                    section_width);

            } else {
                fill_tile_row(x, y, section_width);
            }
        } else if (y < image_tile_height) {
            // use current scene
            if (metatile_bank) {
                load_metatile_row(metatile_ptr, x, y, section_width, metatile_bank);
            } else {
                load_tile_row(image_ptr, x, y, section_width, image_tile_width, image_tile_height, fill_tile_id, image_bank);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_row(continuous_scene,
                    x + bottom_x_offset,
                    (y - image_tile_height),
                    section_width);
            } else {
                fill_tile_row(x, y, section_width);
            }
        }
        width -= section_width;
        x += section_width;
        if (!width) return;
    }
    if (y > SCREEN_OOB_TOP) {
        if (top_x_offset + continuous_scenes[DIRECTION_TOP].tile_width > image_tile_width){
            continuous_scene = &continuous_scenes[DIRECTION_TOP];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_row(continuous_scene,
                    x + top_x_offset,
                    continuous_scene->tile_height + y,
                    width);
            } else {
                fill_tile_row(x, y, width);
            }
        } else if (right_y_offset > 0){
            continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_row(continuous_scene,
                    (x - image_tile_width),
                    y + right_y_offset,
                    width);
            } else {
                fill_tile_row(x, y, width);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_TOP_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_row(continuous_scene,
                    (x - image_tile_width) + top_x_offset,
                    continuous_scene->tile_height + y + right_y_offset,
                    width);
            } else {
                fill_tile_row(x, y, width);
            }
        }
    } else if (y < image_tile_height) {
        continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
        if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
            neighbor_load_tile_row(continuous_scene,
                (x - image_tile_width),
                y + right_y_offset,
                width);
        } else {
            fill_tile_row(x, y, width);
        }
    } else {
        if (bottom_x_offset + continuous_scenes[DIRECTION_BOTTOM].tile_width > image_tile_width){
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_row(continuous_scene,
                    x + bottom_x_offset,
                    (y - image_tile_height),
                    width);
            } else {
                fill_tile_row(x, y, width);
            }
        } else if (right_y_offset + continuous_scenes[DIRECTION_RIGHT].tile_height > image_tile_height){
            continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_row(continuous_scene,
                    (x - image_tile_width),
                    y + right_y_offset,
                    width);
            } else {
                fill_tile_row(x, y, width);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_row(continuous_scene,
                    (x - image_tile_width) + bottom_x_offset,
                    (y - image_tile_height) + right_y_offset,
                    width);
            } else {
                fill_tile_row(x, y, width);
            }
        }
    }
}

void load_tile_col_continuous(UBYTE x, UBYTE y, UBYTE height) {
    // Used for continuous scene row rendering to adjust for different scene sizes and offsets
    bkg_address_offset = ((UWORD)get_bkg_xy_addr((x + bkg_offset_x), (y + bkg_offset_y))) - 0x9800;
    continuous_scene_t* continuous_scene;
    BYTE top_x_offset = (continuous_scene_enabled & DIRECTION_TOP_FLAG) ? continuous_scenes[DIRECTION_TOP].offset : 0;
    BYTE left_y_offset = (continuous_scene_enabled & DIRECTION_LEFT_FLAG) ? continuous_scenes[DIRECTION_LEFT].offset : 0;
    BYTE bottom_x_offset = (continuous_scene_enabled & DIRECTION_BOTTOM_FLAG) ? continuous_scenes[DIRECTION_BOTTOM].offset : 0;
    BYTE right_y_offset = (continuous_scene_enabled & DIRECTION_RIGHT_FLAG) ? continuous_scenes[DIRECTION_RIGHT].offset : 0;
    UBYTE section_height;
    if (y > SCREEN_OOB_TOP){
        section_height = MIN(height, (UBYTE)(0 - y));
        if (x > SCREEN_OOB_LEFT) {
            if (top_x_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_TOP];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_col(continuous_scene,
                        x + top_x_offset,
                        (UBYTE)(continuous_scene->tile_height + y),
                        section_height);
                } else {
                    fill_tile_col(x, y, section_height);
                }
            } else if (left_y_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_col(continuous_scene,
                        continuous_scene->tile_width + x,
                        (UBYTE)(y + left_y_offset),
                        section_height);
                } else {
                    fill_tile_col(x, y, section_height);
                }
            } else {
                continuous_scene = &continuous_scenes[DIRECTION_TOP_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_col(continuous_scene,
                        continuous_scene->tile_width + x + top_x_offset,
                        (UBYTE)(continuous_scene->tile_height + y + left_y_offset),
                        section_height);
                } else {
                    fill_tile_col(x, y, section_height);
                }
            }
        } else if (x < image_tile_width) {
            continuous_scene = &continuous_scenes[DIRECTION_TOP];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_col(continuous_scene,
                    x + top_x_offset,
                    (UBYTE)(continuous_scene->tile_height + y),
                    section_height);
            } else {
                fill_tile_col(x, y, section_height);
            }
        } else {
            if (top_x_offset + continuous_scenes[DIRECTION_TOP].tile_width > image_tile_width){
                continuous_scene = &continuous_scenes[DIRECTION_TOP];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_col(continuous_scene,
                        x + top_x_offset,
                        (UBYTE)(continuous_scene->tile_height + y),
                        section_height);
                } else {
                    fill_tile_col(x, y, section_height);
                }
            } else if (right_y_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_col(continuous_scene,
                        (x - image_tile_width),
                        (UBYTE)(y + right_y_offset),
                        section_height);
                } else {
                    fill_tile_col(x, y, section_height);
                }
            } else {
                continuous_scene = &continuous_scenes[DIRECTION_TOP_RIGHT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_col(continuous_scene,
                        (x - image_tile_width) + top_x_offset,
                        (UBYTE)(continuous_scene->tile_height + y + right_y_offset),
                        section_height);
                } else {
                    fill_tile_col(x, y, section_height);
                }
            }
        }
        height -= section_height;
        y += section_height;
        if (!height) return;
    }
    if (y < image_tile_height) {
        section_height = MIN(height, (image_tile_height - y));
        if (x > SCREEN_OOB_LEFT) {
            continuous_scene = &continuous_scenes[DIRECTION_LEFT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
            neighbor_load_tile_col(continuous_scene,
                continuous_scene->tile_width + x,
                (y + left_y_offset),
                section_height);
            } else {
                fill_tile_col(x, y, section_height);
            }
        } else if (x < image_tile_width) {
            // use current scene
            if (metatile_bank) {
                load_metatile_col(metatile_ptr, x, y, section_height, metatile_bank);
            } else {
                load_tile_col(image_ptr, x, y, section_height, image_tile_width, image_tile_height, fill_tile_id, image_bank);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
            neighbor_load_tile_col(continuous_scene,
                (x - image_tile_width),
                (y + right_y_offset),
                section_height);
            } else {
                fill_tile_col(x, y, section_height);
            }
        }
        height -= section_height;
        y += section_height;
        if (!height) return;
    }
    if (x > SCREEN_OOB_LEFT) {
        if (bottom_x_offset > 0){
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_col(continuous_scene,
                    x + bottom_x_offset,
                    (UBYTE)(y - image_tile_height),
                    height);
            } else {
                fill_tile_col(x, y, height);
            }
        } else if ((left_y_offset + continuous_scenes[DIRECTION_LEFT].tile_height) > image_tile_height){
            continuous_scene = &continuous_scenes[DIRECTION_LEFT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_col(continuous_scene,
                    continuous_scene->tile_width + x,
                    (UBYTE)(y + left_y_offset),
                    height);
            } else {
                fill_tile_col(x, y, height);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM_LEFT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_col(continuous_scene,
                    continuous_scene->tile_width + x + bottom_x_offset,
                    (UBYTE)((y - image_tile_height) + left_y_offset),
                    height);
            } else {
                fill_tile_col(x, y, height);
            }
        }
    } else if (x < image_tile_width) {
        continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
        if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
            neighbor_load_tile_col(continuous_scene,
                x + bottom_x_offset,
                (UBYTE)(y - image_tile_height),
                height);
        } else {
            fill_tile_col(x, y, height);
        }
    } else {
        if (bottom_x_offset + continuous_scenes[DIRECTION_BOTTOM].tile_width > image_tile_width){
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_col(continuous_scene,
                    x + bottom_x_offset,
                    (UBYTE)(y - image_tile_height),
                    height);
            } else {
                fill_tile_col(x, y, height);
            }
        } else if (right_y_offset + continuous_scenes[DIRECTION_RIGHT].tile_height > image_tile_height){
            continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_col(continuous_scene,
                    (x - image_tile_width),
                    (UBYTE)(y + right_y_offset),
                    height);
            } else {
                fill_tile_col(x, y, height);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_col(continuous_scene,
                    (x - image_tile_width) + bottom_x_offset,
                    (UBYTE)((y - image_tile_height) + right_y_offset),
                    height);
            } else {
                fill_tile_col(x, y, height);
            }
        }
    }
}

#ifdef CGB

// ============================================================
// CGB: Metatile attribute rendering — neighbor scene (ROM tilemap lookup)
// ============================================================

// Render a row of CGB attributes from a neighbor scene's ROM tilemap via metatile_attr_ptr lookup.
static void load_neighbor_metatile_attr_row(const UBYTE* tilemap, UBYTE x, UBYTE y,
                                            UBYTE width, UBYTE source_width, UBYTE source_height, UBYTE bank) {
#if METATILE_SIZE == METATILE_SIZE_16
    UBYTE metatile_src_w = source_width >> 1;
    UWORD y_offset = ((UWORD)(y >> 1)) * metatile_src_w;
    width = width + x;
    for (x; x != width; x++) {
        UBYTE tile_id;
        if (x < source_width && y < source_height) {
            UBYTE mid = ReadBankedUBYTE(tilemap + y_offset + (x >> 1), bank);
            tile_id = ReadBankedUBYTE(metatile_attr_ptr + TILE_MAP_OFFSET(mid, x, y), metatile_attr_bank);
        } else {
            tile_id = ReadBankedUBYTE(metatile_attr_ptr + TILE_MAP_OFFSET(fill_tile_id, x, y), metatile_attr_bank);
        }
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset & 0xFFE0) + ((bkg_address_offset + 1) & 31);
    }
#else
    UWORD y_offset = (y * (UWORD)source_width);
    width = width + x;
    for (x; x != width; x++) {
        UBYTE tile_id;
        if (x < source_width && y < source_height) {
            UBYTE mid = ReadBankedUBYTE(tilemap + y_offset + x, bank);
            tile_id = ReadBankedUBYTE(metatile_attr_ptr + mid, metatile_attr_bank);
        } else {
            tile_id = ReadBankedUBYTE(metatile_attr_ptr + fill_tile_id, metatile_attr_bank);
        }
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset & 0xFFE0) + ((bkg_address_offset + 1) & 31);
    }
#endif
}

// Render a column of CGB attributes from a neighbor scene's ROM tilemap via metatile_attr_ptr lookup.
static void load_neighbor_metatile_attr_col(const UBYTE* tilemap, UBYTE x, UWORD y,
                                            UWORD height, UBYTE source_width, UBYTE source_height, UBYTE bank) {
#if METATILE_SIZE == METATILE_SIZE_16
    UBYTE metatile_src_w = source_width >> 1;
    UBYTE metatile_x = x >> 1;
    UWORD end_y = y + height;
    for (y; y != end_y; y++) {
        UBYTE tile_id;
        if (x < source_width && y < source_height) {
            UWORD rom_offset = ((UWORD)(y >> 1)) * metatile_src_w + metatile_x;
            UBYTE mid = ReadBankedUBYTE(tilemap + rom_offset, bank);
            tile_id = ReadBankedUBYTE(metatile_attr_ptr + TILE_MAP_OFFSET(mid, x, y), metatile_attr_bank);
        } else {
            tile_id = ReadBankedUBYTE(metatile_attr_ptr + TILE_MAP_OFFSET(fill_tile_id, x, y), metatile_attr_bank);
        }
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset + 32) & 1023;
    }
#else
    UWORD tile_offset = (y * (UINT16)source_width) + x;
    height = tile_offset + (height * (UINT16)source_width);
    for (tile_offset; tile_offset != height; tile_offset += (UINT16)source_width) {
        UBYTE tile_id;
        if (x < source_width && y < source_height) {
            UBYTE mid = ReadBankedUBYTE(tilemap + tile_offset, bank);
            tile_id = ReadBankedUBYTE(metatile_attr_ptr + mid, metatile_attr_bank);
        } else {
            tile_id = ReadBankedUBYTE(metatile_attr_ptr + fill_tile_id, metatile_attr_bank);
        }
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), tile_id);
        bkg_address_offset = (bkg_address_offset + 32) & 1023;
        y++;
    }
#endif
}

// ============================================================
// CGB: Metatile attribute fill — current scene
// ============================================================

// Fill a row section with CGB attribute bytes by resolving a metatile ID through metatile_attr_ptr.
static void fill_metatile_attr_row(UBYTE x_offset, UBYTE y_offset, UBYTE width, UBYTE metatile_id) {
    for (UBYTE x = 0; x != width; x++) {
#if METATILE_SIZE == METATILE_SIZE_16
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), ReadBankedUBYTE(metatile_attr_ptr + TILE_MAP_OFFSET(metatile_id, x + x_offset, y_offset), metatile_attr_bank));
#else
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), ReadBankedUBYTE(metatile_attr_ptr + metatile_id, metatile_attr_bank));
#endif
        bkg_address_offset = (bkg_address_offset & 0xFFE0) + ((bkg_address_offset + 1) & 31);
    }
}

// Fill a column section with CGB attribute bytes by resolving a metatile ID through metatile_attr_ptr.
static void fill_metatile_attr_col(UBYTE x_offset, UBYTE y_offset, UBYTE height, UBYTE metatile_id) {
    for (UBYTE y = 0; y != height; y++) {
#if METATILE_SIZE == METATILE_SIZE_16
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), ReadBankedUBYTE(metatile_attr_ptr + TILE_MAP_OFFSET(metatile_id, x_offset, y + y_offset), metatile_attr_bank));
#else
        set_vram_byte((UBYTE*)(0x9800 + bkg_address_offset), ReadBankedUBYTE(metatile_attr_ptr + metatile_id, metatile_attr_bank));
#endif
        bkg_address_offset = (bkg_address_offset + 32) & 1023;
    }
}

// ============================================================
// CGB: Attribute fill dispatch — current scene (metatile or plain)
// ============================================================

// Fill a row section with fill_tile_id attrs; treats it as a metatile ID when MetaTile is active.
void fill_tile_attr_row(UBYTE x_offset, UBYTE y_offset, UBYTE width) {
    if (metatile_attr_bank) {
        fill_metatile_attr_row(x_offset, y_offset, width, fill_tile_id);
    } else {
        fill_tile_row_plain(width, fill_tile_attr);
    }
}

// Fill a column section with fill_tile_id attrs; treats it as a metatile ID when MetaTile is active.
void fill_tile_attr_col(UBYTE x_offset, UBYTE y_offset, UBYTE height) {
    if (metatile_attr_bank) {
        fill_metatile_attr_col(x_offset, y_offset, height, fill_tile_id);
    } else {
        fill_tile_col_plain(height, fill_tile_attr);
    }
}

// ============================================================
// CGB: Attribute rendering dispatch — neighbor scene
// ============================================================

// Render a row of CGB attributes from a neighbor scene: uses metatile ROM lookup when MetaTile
// is active, otherwise reads attrs directly from the neighbor's banked attr tilemap.
static void neighbor_load_tile_attr_row(continuous_scene_t* continuous_scene, UBYTE x, UBYTE y, UBYTE width) {
    if (metatile_attr_bank) {
        load_neighbor_metatile_attr_row(continuous_scene->tilemap.ptr, x, y, width, continuous_scene->tile_width, continuous_scene->tile_height, continuous_scene->tilemap.bank);
    } else {
        load_tile_row(continuous_scene->cgb_tilemap_attr.ptr, x, y, width, continuous_scene->tile_width, continuous_scene->tile_height, fill_tile_attr, continuous_scene->cgb_tilemap_attr.bank);
    }
}

// Render a column of CGB attributes from a neighbor scene: uses metatile ROM lookup when MetaTile
// is active, otherwise reads attrs directly from the neighbor's banked attr tilemap.
static void neighbor_load_tile_attr_col(continuous_scene_t* continuous_scene, UBYTE x, UWORD y, UWORD height) {
    if (metatile_attr_bank) {
        load_neighbor_metatile_attr_col(continuous_scene->tilemap.ptr, x, y, height, continuous_scene->tile_width, continuous_scene->tile_height, continuous_scene->tilemap.bank);
    } else {
        load_tile_col(continuous_scene->cgb_tilemap_attr.ptr, x, y, height, continuous_scene->tile_width, continuous_scene->tile_height, fill_tile_attr, continuous_scene->cgb_tilemap_attr.bank);
    }
}

void load_tile_attribute_row_continuous(UBYTE x, UBYTE y, UBYTE width) {
    // Used for continuous scene row rendering to adjust for different scene sizes and offsets
    bkg_address_offset = ((UWORD)get_bkg_xy_addr((x + bkg_offset_x), (y + bkg_offset_y))) - 0x9800;
    continuous_scene_t* continuous_scene;
    BYTE top_x_offset = (continuous_scene_enabled & DIRECTION_TOP_FLAG) ? continuous_scenes[DIRECTION_TOP].offset : 0;
    BYTE left_y_offset = (continuous_scene_enabled & DIRECTION_LEFT_FLAG) ? continuous_scenes[DIRECTION_LEFT].offset : 0;
    BYTE bottom_x_offset = (continuous_scene_enabled & DIRECTION_BOTTOM_FLAG) ? continuous_scenes[DIRECTION_BOTTOM].offset : 0;
    BYTE right_y_offset = (continuous_scene_enabled & DIRECTION_RIGHT_FLAG) ? continuous_scenes[DIRECTION_RIGHT].offset : 0;
    UBYTE section_width;
    if (x > SCREEN_OOB_LEFT){
        section_width = MIN(width, (UBYTE)(0 - x));
        if (y > SCREEN_OOB_TOP) {
            if (top_x_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_TOP];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_row(continuous_scene,
                        x + top_x_offset,
                        continuous_scene->tile_height + y,
                        section_width);
                } else {
                    fill_tile_attr_row(x, y, section_width);
                }
            } else if (left_y_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_row(continuous_scene,
                        continuous_scene->tile_width + x,
                        y + left_y_offset,
                        section_width);
                } else {
                    fill_tile_attr_row(x, y, section_width);
                }
            } else {
                continuous_scene = &continuous_scenes[DIRECTION_TOP_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_row(continuous_scene,
                        continuous_scene->tile_width + x + top_x_offset,
                        continuous_scene->tile_height + y + left_y_offset,
                        section_width);
                } else {
                    fill_tile_attr_row(x, y, section_width);
                }
            }
        } else if (y < image_tile_height) {
            continuous_scene = &continuous_scenes[DIRECTION_LEFT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_row(continuous_scene,
                    continuous_scene->tile_width + x,
                    y + left_y_offset,
                    section_width);
            } else {
                fill_tile_attr_row(x, y, section_width);
            }
        } else {
            if (bottom_x_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_row(continuous_scene,
                        x + bottom_x_offset,
                        (y - image_tile_height),
                        section_width);
                } else {
                    fill_tile_attr_row(x, y, section_width);
                }
            } else if ((left_y_offset + continuous_scenes[DIRECTION_LEFT].tile_height) > image_tile_height){
                continuous_scene = &continuous_scenes[DIRECTION_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                     neighbor_load_tile_attr_row(continuous_scene,
                        continuous_scene->tile_width + x,
                        y + left_y_offset,
                        section_width);
                } else {
                    fill_tile_attr_row(x, y, section_width);
                }
            } else {
                continuous_scene = &continuous_scenes[DIRECTION_BOTTOM_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_row(continuous_scene,
                        continuous_scene->tile_width + x + bottom_x_offset,
                        (y - image_tile_height) + left_y_offset,
                        section_width);
                } else {
                    fill_tile_attr_row(x, y, section_width);
                }
            }
        }
        width -= section_width;
        x += section_width;
        if (!width) return;
    }
    if (x < image_tile_width) {
        section_width = MIN(width, (image_tile_width - x));
        if (y > SCREEN_OOB_TOP) {
            continuous_scene = &continuous_scenes[DIRECTION_TOP];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_row(continuous_scene,
                    x + top_x_offset,
                    continuous_scene->tile_height + y,
                    section_width);

            } else {
                fill_tile_attr_row(x, y, section_width);
            }
        } else if (y < image_tile_height) {
            // use current scene
            if (metatile_attr_bank) {
                load_metatile_row(metatile_attr_ptr, x, y, section_width, metatile_attr_bank);
            } else {
                load_tile_row(image_attr_ptr, x, y, section_width, image_tile_width, image_tile_height, fill_tile_attr, image_attr_bank);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_row(continuous_scene,
                    x + bottom_x_offset,
                    (y - image_tile_height),
                    section_width);
            } else {
                fill_tile_attr_row(x, y, section_width);
            }
        }
        width -= section_width;
        x += section_width;
        if (!width) return;
    }
    if (y > SCREEN_OOB_TOP) {
        if (top_x_offset + continuous_scenes[DIRECTION_TOP].tile_width > image_tile_width){
            continuous_scene = &continuous_scenes[DIRECTION_TOP];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_row(continuous_scene,
                    x + top_x_offset,
                    continuous_scene->tile_height + y,
                    width);
            } else {
                fill_tile_attr_row(x, y, width);
            }
        } else if (right_y_offset > 0){
            continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_row(continuous_scene,
                    (x - image_tile_width),
                    y + right_y_offset,
                    width);
            } else {
                fill_tile_attr_row(x, y, width);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_TOP_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_row(continuous_scene,
                    (x - image_tile_width) + top_x_offset,
                    continuous_scene->tile_height + y + right_y_offset,
                    width);
            } else {
                fill_tile_attr_row(x, y, width);
            }
        }
    } else if (y < image_tile_height) {
        continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
        if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
            neighbor_load_tile_attr_row(continuous_scene,
                (x - image_tile_width),
                y + right_y_offset,
                width);
        } else {
            fill_tile_attr_row(x, y, width);
        }
    } else {
        if (bottom_x_offset + continuous_scenes[DIRECTION_BOTTOM].tile_width > image_tile_width){
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_row(continuous_scene,
                    x + bottom_x_offset,
                    (y - image_tile_height),
                    width);
            } else {
                fill_tile_attr_row(x, y, width);
            }
        } else if (right_y_offset + continuous_scenes[DIRECTION_RIGHT].tile_height > image_tile_height){
            continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_row(continuous_scene,
                    (x - image_tile_width),
                    y + right_y_offset,
                    width);
            } else {
                fill_tile_attr_row(x, y, width);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_row(continuous_scene,
                    (x - image_tile_width) + bottom_x_offset,
                    (y - image_tile_height) + right_y_offset,
                    width);
            } else {
                fill_tile_attr_row(x, y, width);
            }
        }
    }
}

void load_tile_attribute_col_continuous(UBYTE x, UBYTE y, UBYTE height) {
    // Used for continuous scene row rendering to adjust for different scene sizes and offsets
    bkg_address_offset = ((UWORD)get_bkg_xy_addr((x + bkg_offset_x), (y + bkg_offset_y))) - 0x9800;
    continuous_scene_t* continuous_scene;
    BYTE top_x_offset = (continuous_scene_enabled & DIRECTION_TOP_FLAG) ? continuous_scenes[DIRECTION_TOP].offset : 0;
    BYTE left_y_offset = (continuous_scene_enabled & DIRECTION_LEFT_FLAG) ? continuous_scenes[DIRECTION_LEFT].offset : 0;
    BYTE bottom_x_offset = (continuous_scene_enabled & DIRECTION_BOTTOM_FLAG) ? continuous_scenes[DIRECTION_BOTTOM].offset : 0;
    BYTE right_y_offset = (continuous_scene_enabled & DIRECTION_RIGHT_FLAG) ? continuous_scenes[DIRECTION_RIGHT].offset : 0;
    UBYTE section_height;
    if (y > SCREEN_OOB_TOP){
        section_height = MIN(height, (UBYTE)(0 - y));
        if (x > SCREEN_OOB_LEFT) {
            if (top_x_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_TOP];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_col(continuous_scene,
                        x + top_x_offset,
                        (UBYTE)(continuous_scene->tile_height + y),
                        section_height);
                } else {
                    fill_tile_attr_col(x, y, section_height);
                }
            } else if (left_y_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_col(continuous_scene,
                        continuous_scene->tile_width + x,
                        (UBYTE)(y + left_y_offset),
                        section_height);
                } else {
                    fill_tile_attr_col(x, y, section_height);
                }
            } else {
                continuous_scene = &continuous_scenes[DIRECTION_TOP_LEFT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_col(continuous_scene,
                        continuous_scene->tile_width + x + top_x_offset,
                        (UBYTE)(continuous_scene->tile_height + y + left_y_offset),
                        section_height);
                } else {
                    fill_tile_attr_col(x, y, section_height);
                }
            }
        } else if (x < image_tile_width) {
            continuous_scene = &continuous_scenes[DIRECTION_TOP];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_col(continuous_scene,
                    x + top_x_offset,
                    (UBYTE)(continuous_scene->tile_height + y),
                    section_height);
            } else {
                fill_tile_attr_col(x, y, section_height);
            }
        } else {
            if (top_x_offset + continuous_scenes[DIRECTION_TOP].tile_width > image_tile_width){
                continuous_scene = &continuous_scenes[DIRECTION_TOP];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_col(continuous_scene,
                        x + top_x_offset,
                        (UBYTE)(continuous_scene->tile_height + y),
                        section_height);
                } else {
                    fill_tile_attr_col(x, y, section_height);
                }
            } else if (right_y_offset > 0){
                continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_col(continuous_scene,
                        (x - image_tile_width),
                        (UBYTE)(y + right_y_offset),
                        section_height);
                } else {
                    fill_tile_attr_col(x, y, section_height);
                }
            } else {
                continuous_scene = &continuous_scenes[DIRECTION_TOP_RIGHT];
                if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                    neighbor_load_tile_attr_col(continuous_scene,
                        (x - image_tile_width) + top_x_offset,
                        (UBYTE)(continuous_scene->tile_height + y + right_y_offset),
                        section_height);
                } else {
                    fill_tile_attr_col(x, y, section_height);
                }
            }
        }
        height -= section_height;
        y += section_height;
        if (!height) return;
    }
    if (y < image_tile_height) {
        section_height = MIN(height, (image_tile_height - y));
        if (x > SCREEN_OOB_LEFT) {
            continuous_scene = &continuous_scenes[DIRECTION_LEFT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
            neighbor_load_tile_attr_col(continuous_scene,
                continuous_scene->tile_width + x,
                (UBYTE)(y + left_y_offset),
                section_height);
            } else {
                fill_tile_attr_col(x, y, section_height);
            }
        } else if (x < image_tile_width) {
            // use current scene
            if (metatile_attr_bank) {
                load_metatile_col(metatile_attr_ptr, x, y, section_height, metatile_attr_bank);
            } else {
                load_tile_col(image_attr_ptr, x, y, section_height, image_tile_width, image_tile_height, fill_tile_attr, image_attr_bank);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
            neighbor_load_tile_attr_col(continuous_scene,
                (x - image_tile_width),
                (y + right_y_offset),
                section_height);
            } else {
                fill_tile_attr_col(x, y, section_height);
            }
        }
        height -= section_height;
        y += section_height;
        if (!height) return;
    }
    if (x > SCREEN_OOB_LEFT) {
        if (bottom_x_offset > 0){
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_col(continuous_scene,
                    x + bottom_x_offset,
                    (UBYTE)(y - image_tile_height),
                    height);
            } else {
                fill_tile_attr_col(x, y, height);
            }
        } else if ((left_y_offset + continuous_scenes[DIRECTION_LEFT].tile_height) > image_tile_height){
            continuous_scene = &continuous_scenes[DIRECTION_LEFT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_col(continuous_scene,
                    continuous_scene->tile_width + x,
                    (UBYTE)(y + left_y_offset),
                    height);
            } else {
                fill_tile_attr_col(x, y, height);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM_LEFT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_col(continuous_scene,
                    continuous_scene->tile_width + x + bottom_x_offset,
                    (UBYTE)((y - image_tile_height) + left_y_offset),
                    height);
            } else {
                fill_tile_attr_col(x, y, height);
            }
        }
    } else if (x < image_tile_width) {
        continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
        if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
            neighbor_load_tile_attr_col(continuous_scene,
                x + bottom_x_offset,
                (UBYTE)(y - image_tile_height),
                height);
        } else {
            fill_tile_attr_col(x, y, height);
        }
    } else {
        if (bottom_x_offset + continuous_scenes[DIRECTION_BOTTOM].tile_width > image_tile_width){
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_col(continuous_scene,
                    x + bottom_x_offset,
                    (UBYTE)(y - image_tile_height),
                    height);
            } else {
                fill_tile_attr_col(x, y, height);
            }
        } else if (right_y_offset + continuous_scenes[DIRECTION_RIGHT].tile_height > image_tile_height){
            continuous_scene = &continuous_scenes[DIRECTION_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_col(continuous_scene,
                    (x - image_tile_width),
                    (UBYTE)(y + right_y_offset),
                    height);
            } else {
                fill_tile_attr_col(x, y, height);
            }
        } else {
            continuous_scene = &continuous_scenes[DIRECTION_BOTTOM_RIGHT];
            if (continuous_scene->scene.ptr && continuous_scene->scene.bank){
                neighbor_load_tile_attr_col(continuous_scene,
                    (x - image_tile_width) + bottom_x_offset,
                    (UBYTE)((y - image_tile_height) + right_y_offset),
                    height);
            } else {
                fill_tile_attr_col(x, y, height);
            }
        }
    }
}
#endif

void scroll_load_row(UBYTE x, UBYTE y) {
    UBYTE width = SCREEN_TILE_REFRES_W;
    // DMG Row Load
    load_tile_row_continuous(x, y, width);
#ifdef CGB
    if (_is_CGB) {  // Color Row Load
        VBK_REG = 1;
        load_tile_attribute_row_continuous(x, y, width);
        VBK_REG = 0;
    }
#endif

}

/* Update pending (up to 5) rows */
void scroll_load_pending_row(void) {
    UBYTE width = MIN(pending_w_i, PENDING_BATCH_SIZE);
    // DMG Row Load
    load_tile_row_continuous(pending_w_x, pending_w_y, width);
#ifdef CGB
    if (_is_CGB) {  // Color Row Load
        VBK_REG = 1;
        load_tile_attribute_row_continuous(pending_w_x, pending_w_y, width);
        VBK_REG = 0;
    }
#endif

    pending_w_x += width;
    pending_w_i -= width;
}


void scroll_load_col(UBYTE x, UBYTE y, UBYTE height) {
    // DMG Column Load
    load_tile_col_continuous(x, y, height);
#ifdef CGB
    if (_is_CGB) {  // Color Column Load
        VBK_REG = 1;
        load_tile_attribute_col_continuous(x, y, height);
        VBK_REG = 0;
    }
#endif

}

void scroll_load_pending_col(void) {
    UBYTE height = MIN(pending_h_i, PENDING_BATCH_SIZE);
    // DMG Column Load
    load_tile_col_continuous(pending_h_x, pending_h_y, height);
#ifdef CGB
    if (_is_CGB) {  // Color Column Load
        VBK_REG = 1;
        load_tile_attribute_col_continuous(pending_h_x, pending_h_y, height);
        VBK_REG = 0;
    }
#endif

    pending_h_y += height;
    pending_h_i -= height;
}

