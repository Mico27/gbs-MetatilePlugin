#pragma bank 255

#include "data/states_defines.h"
#include "states/topdown.h"

#include "actor.h"
#include "camera.h"
#include "collision.h"
#include "data_manager.h"
#include "game_time.h"
#include "input.h"
#include "trigger.h"
#include "math.h"
#include "vm.h"
#include "meta_tiles.h"
#include "continuous_scene.h"

#ifndef INPUT_TOPDOWN_INTERACT
#define INPUT_TOPDOWN_INTERACT INPUT_A
#endif

#define COUNTER_COLLISION_GROUP 32

UBYTE topdown_grid;
UBYTE player_collision_group;

void topdown_init(void) BANKED {
    camera_offset_x = 0;
    camera_offset_y = 0;
    camera_deadzone_x = 0;
    camera_deadzone_y = 0;

    if (topdown_grid == 16) {
        // Snap to 16px grid
        PLAYER.pos.x = SUBPX_SNAP_TILE16(PLAYER.pos.x);
        PLAYER.pos.y = SUBPX_SNAP_TILE16(PLAYER.pos.y) + TILE_TO_SUBPX(1);
    } else {
        PLAYER.pos.x = SUBPX_SNAP_TILE(PLAYER.pos.x);
        PLAYER.pos.y = SUBPX_SNAP_TILE(PLAYER.pos.y);
    }
}

static actor_t *actor_with_script_at_position(upoint16_t position) {
    actor_t *actor = actors_active_tail;

    const UWORD a_left   = position.x + PLAYER.bounds.left;
    const UWORD a_right  = position.x + PLAYER.bounds.right;
    const UWORD a_top    = position.y + PLAYER.bounds.top;
    const UWORD a_bottom = position.y + PLAYER.bounds.bottom;

    while (actor) {
        if (actor == &PLAYER || !actor->script.bank) {
            actor = actor->prev;
            continue;
        }
        if ((actor->pos.x + actor->bounds.left)   > a_right)  { actor = actor->prev; continue; }
        if ((actor->pos.x + actor->bounds.right)  < a_left)   { actor = actor->prev; continue; }
        if ((actor->pos.y + actor->bounds.top)    > a_bottom) { actor = actor->prev; continue; }
        if ((actor->pos.y + actor->bounds.bottom) < a_top)    { actor = actor->prev; continue; }
        return actor;
    }
    return NULL;
}

actor_t *actor_with_script_in_front_of_player_or_counter(void) BANKED {
    upoint16_t offset;
    offset.x = PLAYER.pos.x;
    offset.y = PLAYER.pos.y;
    point_translate_dir_word(&offset, PLAYER.dir, PX_TO_SUBPX(topdown_grid));
    actor_t *actor = actor_with_script_at_position(offset);

    //check for counter collision group
    if (!actor && tile_at(SUBPX_TO_TILE(offset.x), SUBPX_TO_TILE(offset.y)) & COUNTER_COLLISION_GROUP) {
        point_translate_dir_word(&offset, PLAYER.dir, PX_TO_SUBPX(topdown_grid));
        actor = actor_with_script_at_position(offset);
    }

    return actor;
}

void topdown_update(void) BANKED {
    actor_t *hit_actor;
    UBYTE tile_start, tile_end;
    direction_e new_dir = DIR_NONE;
    static UWORD max_pos = 0;

    // Is player on an 8x8px tile?
    if ((topdown_grid == 16 && ON_16PX_GRID(PLAYER.pos)) ||
        (topdown_grid == 8 && ON_8PX_GRID(PLAYER.pos))) {
        // Player landed on an tile
        // so stop movement for now
        player_moving = FALSE;

        //Check scene transition
        if (check_transition_to_scene_collision()) {
            return;
        }

        // Check for trigger collisions
        if (trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, FALSE)) {
            // Landed on a trigger
            return;
        }
#ifdef ENABLE_TOPDOWN_ENTER_METATILE
        if (metatile_overlap_at_intersection(&PLAYER.bounds, &PLAYER.pos)){
            // Landed on a metatile with an enter event
            return;
        }
#endif
        // Check input to set player movement
        if (INPUT_RECENT_LEFT) {
            player_moving = TRUE;
            new_dir = DIR_LEFT;

            // Check for collisions to left of player
            tile_start = SUBPX_TO_TILE(PLAYER.pos.y + PLAYER.bounds.top);
            tile_end   = SUBPX_TO_TILE(PLAYER.pos.y + PLAYER.bounds.bottom);
            UBYTE tile_x = SUBPX_TO_TILE(PLAYER.pos.x + PLAYER.bounds.left);
            if (tile_col_test_range_y(COLLISION_RIGHT | player_collision_group, tile_x - 1, tile_start, tile_end)) {
                player_moving = FALSE;
#ifdef ENABLE_TOPDOWN_LEFT_COLLISION_METATILE
                on_player_metatile_collision(tile_hit_x, tile_hit_y, DIR_LEFT);
#endif
            }
        } else if (INPUT_RECENT_RIGHT) {
            player_moving = TRUE;
            new_dir = DIR_RIGHT;

            // Check for collisions to right of player
            tile_start = SUBPX_TO_TILE(PLAYER.pos.y + PLAYER.bounds.top);
            tile_end   = SUBPX_TO_TILE(PLAYER.pos.y + PLAYER.bounds.bottom);
            UBYTE tile_x = SUBPX_TO_TILE(PLAYER.pos.x + PLAYER.bounds.right);
            if (tile_col_test_range_y(COLLISION_LEFT | player_collision_group, tile_x + 1, tile_start, tile_end)) {
                player_moving = FALSE;
#ifdef ENABLE_TOPDOWN_RIGHT_COLLISION_METATILE
                on_player_metatile_collision(tile_hit_x, tile_hit_y, DIR_RIGHT);
#endif
            }
        } else if (INPUT_RECENT_UP) {
            player_moving = TRUE;
            new_dir = DIR_UP;

            // Check for collisions below player
            tile_start = SUBPX_TO_TILE(PLAYER.pos.x + PLAYER.bounds.left);
            tile_end   = SUBPX_TO_TILE(PLAYER.pos.x + PLAYER.bounds.right);
            UBYTE tile_y = SUBPX_TO_TILE(PLAYER.pos.y + PLAYER.bounds.top);
            if (tile_col_test_range_x(COLLISION_BOTTOM | player_collision_group, tile_y - 1, tile_start, tile_end)) {
                player_moving = FALSE;
#ifdef ENABLE_TOPDOWN_UP_COLLISION_METATILE
                on_player_metatile_collision(tile_hit_x, tile_hit_y, DIR_UP);
#endif
            }
        } else if (INPUT_RECENT_DOWN) {
            player_moving = TRUE;
            new_dir = DIR_DOWN;

            // Check for collisions below player
            tile_start = SUBPX_TO_TILE(PLAYER.pos.x + PLAYER.bounds.left);
            tile_end   = SUBPX_TO_TILE(PLAYER.pos.x + PLAYER.bounds.right);
            UBYTE tile_y = SUBPX_TO_TILE(PLAYER.pos.y + PLAYER.bounds.bottom);
            if (tile_col_test_range_x(COLLISION_TOP | player_collision_group, tile_y + 1, tile_start, tile_end)) {
                player_moving = FALSE;
#ifdef ENABLE_TOPDOWN_DOWN_COLLISION_METATILE
                on_player_metatile_collision(tile_hit_x, tile_hit_y, DIR_DOWN);
#endif
            }
        }

        // Update direction animation
        if (new_dir != DIR_NONE) {
            actor_set_dir(&PLAYER, new_dir, player_moving);
        } else {
            actor_set_anim_idle(&PLAYER);
        }

        // Check for actor overlap
        hit_actor = actor_overlapping_player();
        if (hit_actor != NULL && (hit_actor->collision_group & COLLISION_GROUP_MASK)) {
            player_register_collision_with(hit_actor);
        }

        // Check if walked in to actor
        if (player_moving) {
            hit_actor = actor_in_front_of_player(topdown_grid, FALSE);
            if (hit_actor != NULL) {
                player_register_collision_with(hit_actor);
                actor_stop_anim(&PLAYER);
                player_moving = FALSE;
            }
        }

        if (INPUT_PRESSED(INPUT_TOPDOWN_INTERACT)) {
            hit_actor = actor_with_script_in_front_of_player_or_counter();
            if (hit_actor != NULL && !(hit_actor->collision_group & COLLISION_GROUP_MASK)) {
                actor_set_dir(hit_actor, FLIPPED_DIR(PLAYER.dir), FALSE);
                player_moving = FALSE;
                if (hit_actor->script.bank) {
                    script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 0);
                }
            }
        }

        // Calculate max position for movement clamping
        UBYTE tile_offset = (topdown_grid == 16) ? 2 : 1;
        if (PLAYER.dir == DIR_RIGHT) {
            max_pos = TILE_TO_SUBPX(SUBPX_TO_TILE(PLAYER.pos.x) + tile_offset);
        } else if (PLAYER.dir == DIR_LEFT) {
            max_pos = TILE_TO_SUBPX(SUBPX_TO_TILE(PLAYER.pos.x) - tile_offset);
        } else if (PLAYER.dir == DIR_DOWN) {
            max_pos = TILE_TO_SUBPX(SUBPX_TO_TILE(PLAYER.pos.y) + tile_offset);
        } else if (PLAYER.dir == DIR_UP) {
            max_pos = TILE_TO_SUBPX(SUBPX_TO_TILE(PLAYER.pos.y) - tile_offset);
        }
    }

    if (player_moving) {
        point_translate_dir(&PLAYER.pos, PLAYER.dir, PLAYER.move_speed);
        
        // Clamp to grid
        if (PLAYER.dir == DIR_RIGHT) {
            if (PLAYER.pos.x > max_pos && (PLAYER.pos.x - max_pos) < PLAYER.move_speed) {
                PLAYER.pos.x = max_pos;
            }
        } else if (PLAYER.dir == DIR_LEFT) {
            if (PLAYER.pos.x < max_pos && (max_pos - PLAYER.pos.x) < PLAYER.move_speed) {
                PLAYER.pos.x = max_pos;
            }
        } else if (PLAYER.dir == DIR_DOWN) {
            if (PLAYER.pos.y > max_pos && (PLAYER.pos.y - max_pos) < PLAYER.move_speed) {
                PLAYER.pos.y = max_pos;
            }
        } else if (PLAYER.dir == DIR_UP) {
            if (PLAYER.pos.y < max_pos && (max_pos - PLAYER.pos.y) < PLAYER.move_speed) {
                PLAYER.pos.y = max_pos;
            }
        }
        
    }
}
