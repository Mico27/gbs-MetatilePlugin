#pragma bank 255

#include <string.h>
#include <stdlib.h>
#include <gbdk/platform.h>
#include "system.h"
#include "vm.h"
#include "gbs_types.h"
#include "events.h"
#include "input.h"
#include "math.h"
#include "actor.h"
#include "scroll.h"
#include "game_time.h"
#include "actor_behavior.h"
#include "states/platform.h"
#include "data/states_defines.h"
#include "meta_tiles.h"
#include "collision.h"
#include "data_manager.h"
#include "macro.h"
#include "data/game_globals.h"

UBYTE actor_behavior_ids[MAX_ACTORS];
UBYTE actor_states[MAX_ACTORS];
WORD actor_vel_x[MAX_ACTORS];
WORD actor_vel_y[MAX_ACTORS];
UBYTE actor_counter_a[MAX_ACTORS];
UBYTE actor_counter_b[MAX_ACTORS];
UBYTE actor_linked_actor_idx[MAX_ACTORS];

UBYTE current_behavior;
WORD new_actor_x;
WORD new_actor_y;
WORD col_tx;
WORD col_ty;
WORD current_actor_x;
point16_t tmp_point;

void actor_behavior_init(void) BANKED {
    memset(actor_behavior_ids, 0, sizeof(actor_behavior_ids));
	memset(actor_states, 0, sizeof(actor_states));
	memset(actor_vel_x, 0, sizeof(actor_vel_x));
	memset(actor_vel_y, 0, sizeof(actor_vel_y));
	memset(actor_counter_a, 0, sizeof(actor_counter_a));
	memset(actor_counter_b, 0, sizeof(actor_counter_b));
	memset(actor_linked_actor_idx, 0, sizeof(actor_linked_actor_idx));
}

static UWORD check_collision(UWORD start_x, UWORD start_y, rect16_t *bounds, col_check_dir_e check_dir) {    
    switch (check_dir) {
        case CHECK_DIR_LEFT:  // Check left (bottom left)
            col_tx = SUBPX_TO_TILE(start_x + bounds->left);
            col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
            if (tile_at(col_tx, col_ty) & COLLISION_RIGHT) {
                return TILE_TO_SUBPX(col_tx + 1) - bounds->left;
            }
            return start_x;
        case CHECK_DIR_RIGHT:  // Check right (bottom right)
            col_tx = SUBPX_TO_TILE(start_x + bounds->right);
            col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
            if (tile_at(col_tx, col_ty) & COLLISION_LEFT) {
                return TILE_TO_SUBPX(col_tx) - (bounds->right + 1);
            }
            return start_x;
        case CHECK_DIR_UP:  // Check up (middle up)
            col_ty = SUBPX_TO_TILE(start_y + bounds->top);
            col_tx = SUBPX_TO_TILE(start_x + ((bounds->left + bounds->right) >> 1));
            if (tile_at(col_tx, col_ty) & COLLISION_BOTTOM) {
                return TILE_TO_SUBPX(col_ty + 1) - bounds->top;
            }
            return start_y;
        case CHECK_DIR_DOWN:  // Check down (right bottom and left bottom)
            col_ty = SUBPX_TO_TILE(start_y + bounds->bottom);
            col_tx = SUBPX_TO_TILE(start_x + bounds->left);
            if (tile_at(col_tx, col_ty) & COLLISION_TOP) {
                return TILE_TO_SUBPX(col_ty) - (bounds->bottom + 1);
            }			
			col_tx = SUBPX_TO_TILE(start_x + bounds->right);
			if (tile_at(col_tx, col_ty) & COLLISION_TOP) {
                return TILE_TO_SUBPX(col_ty) - (bounds->bottom + 1);
            }
            return start_y;
    }
    return start_x;
}

static UWORD check_pit(UWORD start_x, UWORD start_y, rect16_t *bounds, col_check_dir_e check_dir) {
     WORD tx, ty;
    switch (check_dir) {
        case CHECK_DIR_LEFT:  // Check left (bottom left)
            tx = SUBPX_TO_TILE(start_x + bounds->left);
            ty = SUBPX_TO_TILE(start_y + bounds->bottom) + 1;
            if (!(tile_at(tx, ty) & COLLISION_TOP)) {
                return TILE_TO_SUBPX(tx + 1) - bounds->left;
            }
            return start_x;
        case CHECK_DIR_RIGHT:  // Check right (bottom right)
            tx = SUBPX_TO_TILE(start_x + bounds->right);
            ty = SUBPX_TO_TILE(start_y + bounds->bottom) + 1;
            if (!(tile_at(tx, ty) & COLLISION_TOP)) {
                return TILE_TO_SUBPX(tx) - (bounds->right + 1);
            }
            return start_x;       
    }
    return start_x;
}

static void apply_gravity(UBYTE actor_idx) {
	actor_vel_y[actor_idx] += (plat_grav >> 8);
	actor_vel_y[actor_idx] = MIN(actor_vel_y[actor_idx], (plat_max_fall_vel >> 8));
}

static void apply_velocity(UBYTE actor_idx, actor_t * actor) {
	//Apply velocity
	new_actor_y =  actor->pos.y + actor_vel_y[actor_idx];
	new_actor_x =  actor->pos.x + actor_vel_x[actor_idx];
	if (CHK_FLAG(actor->flags, ACTOR_FLAG_COLLISION)){
		//Tile Collision
		actor->pos.x = check_collision(new_actor_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_actor_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
		if (actor->pos.x != new_actor_x){
			actor_vel_x[actor_idx] = -actor_vel_x[actor_idx];
		}
		actor->pos.y = check_collision(actor->pos.x, new_actor_y, &actor->bounds, ((actor->pos.y > new_actor_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
	} else {
		actor->pos.x = new_actor_x;
		actor->pos.y = new_actor_y;
	}
}

static void apply_velocity_avoid_fall(UBYTE actor_idx, actor_t * actor) {
	//Apply velocity
	new_actor_y =  actor->pos.y + actor_vel_y[actor_idx];
	new_actor_x =  actor->pos.x + actor_vel_x[actor_idx];
	if (CHK_FLAG(actor->flags, ACTOR_FLAG_COLLISION)){
		//Tile Collision
		actor->pos.y = check_collision(actor->pos.x, new_actor_y, &actor->bounds, ((actor->pos.y > new_actor_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
		if (new_actor_y != actor->pos.y){
			actor->pos.x = check_pit(new_actor_x, actor->pos.y, &actor->bounds, ((actor_vel_x[actor_idx] > 0) ? CHECK_DIR_RIGHT : CHECK_DIR_LEFT));
			if (actor->pos.x != new_actor_x){
				actor_vel_x[actor_idx] = -actor_vel_x[actor_idx];
				return;
			}
		}
		actor->pos.x = check_collision(new_actor_x, actor->pos.y, &actor->bounds, ((actor_vel_x[actor_idx] > 0) ? CHECK_DIR_RIGHT : CHECK_DIR_LEFT));
		if (actor->pos.x != new_actor_x){
			actor_vel_x[actor_idx] = -actor_vel_x[actor_idx];
		}
		
	} else {
		actor->pos.x = new_actor_x;
		actor->pos.y = new_actor_y;
	}
}

void actor_behavior_update(void) BANKED {
	for (UBYTE i = 0; i < MAX_ACTORS; i++){
		actor_t * actor = (actors + i);
		if (!CHK_FLAG(actor->flags, ACTOR_FLAG_ACTIVE)){
			continue;
		}
		current_behavior = actor_behavior_ids[i];
		
		switch(current_behavior){	
			case 0:
			break;
			case 1: //Goomba
			switch(actor_states[i]){
				case 0: //Init
					if (((SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_counter_a[i] = 0;
						actor_counter_b[i] = 0;
						actor_vel_y[i] = 0;
						actor_vel_x[i] = -16;
					}
					break;
				case 1: //Main state					
					if (!(actor_counter_b[i] & 3)){
						current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
						if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 							
							break;
						}
						apply_gravity(i);
						apply_velocity(i, actor);
						//Animation
						if (actor_vel_x[i] < 0) {
							actor_set_dir(actor, DIR_LEFT, TRUE);
						} else if (actor_vel_x[i] > 0) {
							actor_set_dir(actor, DIR_RIGHT, TRUE);
						} else {
							actor_set_anim_idle(actor);
						}
						if (new_actor_y == actor->pos.y){
							actor_states[i] = 3;
							actor_vel_x[i] = actor_vel_x[i] >> 2;						
						}
					}
					actor_counter_b[i]++;
					break;
				case 2: //Squished state
					if (actor_counter_a[i] == 0){
						actor_reset_anim(actor);
						actor_vel_y[i] = 0;
						actor_vel_x[i] = 0;
						CLR_FLAG(actor->flags, ACTOR_FLAG_COLLISION);
					}
					actor_counter_a[i]++;
					if (actor_counter_a[i] > 30){
						actor_states[i] = 255; 
					}
					break;
				case 3: //Falling
					current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						break;
					}
					apply_gravity(i);
					apply_velocity(i, actor);
					//Animation
					if (actor_vel_x[i] < 0) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else if (actor_vel_x[i] > 0) {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					} else {
						actor_set_anim_idle(actor);
					}
					if (new_actor_y != actor->pos.y){
						actor_states[i] = 1;	
						actor_vel_x[i] = actor_vel_x[i] << 2;							
					}
					break;
				case 255: //Dead
					SET_FLAG(actor->flags, ACTOR_FLAG_COLLISION);
					actor_counter_a[i] = 0;
					deactivate_actor(actor);
					break;
			}		
			break;	
			case 2: //Green Koopa
			switch(actor_states[i]){
				case 0:
					if (((SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 						
						actor_vel_y[i] = 0;
						actor_vel_x[i] = -16;
					}
					break;
				case 1: //Main state
					if (!(actor_counter_b[i] & 3)){
						current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
						if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 							
							break;
						}					
						apply_gravity(i);
						apply_velocity(i, actor);
						//Animation
						if (actor_vel_x[i] < 0) {
							actor_set_dir(actor, DIR_LEFT, TRUE);
						} else if (actor_vel_x[i] > 0) {
							actor_set_dir(actor, DIR_RIGHT, TRUE);
						} else {
							actor_set_anim_idle(actor);
						}
						if (new_actor_y == actor->pos.y){
							actor_states[i] = 2;
							actor_vel_x[i] = actor_vel_x[i] >> 2;						
						}
					}
					actor_counter_b[i]++;
					break;
				case 2: //Falling
					current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						break;
					}					
					apply_gravity(i);
					apply_velocity(i, actor);
					//Animation
					if (actor_vel_x[i] < 0) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else if (actor_vel_x[i] > 0) {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					} else {
						actor_set_anim_idle(actor);
					}
					if (new_actor_y != actor->pos.y){
						actor_states[i] = 1;	
						actor_vel_x[i] = actor_vel_x[i] << 2;							
					}
					break;
				case 255://dead
					deactivate_actor(actor);
					break;
			}
			break;			
			case 3: //Red Koopa
			switch(actor_states[i]){
				case 0:
					if (((SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 						
						actor_vel_y[i] = 0;
						actor_vel_x[i] = -16;
					}
					break;
				case 1: //Main state
					if (!(actor_counter_b[i] & 3)){
						current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
						if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
							break;
						}					
						apply_gravity(i);
						apply_velocity_avoid_fall(i, actor);
						//Animation
						if (actor_vel_x[i] < 0) {
							actor_set_dir(actor, DIR_LEFT, TRUE);
						} else if (actor_vel_x[i] > 0) {
							actor_set_dir(actor, DIR_RIGHT, TRUE);
						} else {
							actor_set_anim_idle(actor);
						}
					}
					actor_counter_b[i]++;
					break;
				case 255://dead
					deactivate_actor(actor);
					break;
			}
			break;			
			case 4: //Koopa shell
			switch(actor_states[i]){
				case 0:
					if (((SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_counter_a[i] = 0;		
						actor_vel_y[i] = 0;
						actor_vel_x[i] = 0;									
						actor_states[i] = 1;
					}
					break;
				case 1: //tucked state
					current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						break;
					}
					apply_gravity(i);
					apply_velocity(i, actor);
					break;
				case 2: //init kicked state
					actor_counter_a[i] = 0;		
					actor_states[i] = 3;
				case 3: //kicked state player iframe
					if (actor_counter_a[i] > 15){
						actor_states[i] = 4; 
					} else {
						actor_counter_a[i]++;
					}
				case 4: //kicked state
					current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						actor_states[i] = 255; 
						break;
					}
					apply_gravity(i);
					//Apply velocity
					WORD new_y =  actor->pos.y + actor_vel_y[i];
					WORD new_x =  actor->pos.x + actor_vel_x[i];
					if (CHK_FLAG(actor->flags, ACTOR_FLAG_COLLISION)){
						//Tile Collision
						actor->pos.x = check_collision(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
						if (actor->pos.x != new_x){
							actor_vel_x[i] = -actor_vel_x[i];							
							UBYTE tile_id = sram_map_data[METATILE_MAP_OFFSET(col_tx, col_ty)];	
							switch(tile_id){
								case 128://brick	
								case 129://coin block
								case 156://powerup block	
								case 157://1up block
								if(plat_events[HIT_BLOCK].script_addr != 0){
									hit_metatile_id = tile_id;
									hit_metatile_x = col_tx;
									hit_metatile_y = col_ty;
									hit_metatile_source = i;
									script_execute(plat_events[HIT_BLOCK].script_bank, plat_events[HIT_BLOCK].script_addr, 0, 0);
								}
								break;
								default:
								if (actor->script.bank){
									script_execute(actor->script.bank, actor->script.ptr, 0, 1, 8);
								}
								break;
							}
						}
						actor->pos.y = check_collision(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
					} else {
						actor->pos.x = new_x;
						actor->pos.y = new_y;
					}
					//Actor Collision		
					actor_t * hit_actor = actor_overlapping_bb(&actor->bounds, &actor->pos, actor);
					if (hit_actor && hit_actor->script.bank){							
						script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 2);
					}
					break;
				case 255: //Dead
					deactivate_actor(actor);
					break;
			}
			break;
			case 5: //Powerup
			apply_gravity(i);
			apply_velocity(i, actor);	
			break;	
			case 6://Horizontal projectile (Bowser fire, bullet bill, tatanga attack 1)
			switch(actor_states[i]){
				case 0: //Init
					if (((SUBPX_TO_PX(actor->pos.x)) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						if (actor_vel_x[i] == 0){
							actor_vel_x[i] = -12;
						}
						if (actor->script.bank){
							script_execute(actor->script.bank, actor->script.ptr, 0, 1, 8);
						}
					}
					break;
				case 1: //Main state
					current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						break;
					}
					actor->pos.x =  actor->pos.x + actor_vel_x[i];
					break;
				case 255: //Dead
					deactivate_actor(actor);
					break;
			}		
			break;			
			case 7://Pyrahna Plant
			switch(actor_states[i]){
				case 0: //Init
					if (((SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_counter_a[i] = 30;
						actor_vel_y[i] = 0;
					}
					break;
				case 1: //Main state
					current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						break;
					}
					if (actor_vel_y[i] > 0){
						actor->pos.y += 16;
						actor_vel_y[i]--;
					}
					else if ((SUBPX_TO_TILE(actor->pos.y) - 2) != SUBPX_TO_TILE(PLAYER.pos.y)){ //dont pop out if player is on top
						actor_counter_a[i]--;
						if (actor_counter_a[i] <= 0){
							actor_counter_a[i] = 120;
							actor_vel_y[i] = 24;
							actor_states[i] = 2; 
						}
					}
					break;
				case 2: //Out state
					if (((SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x) > BEHAVIOR_DEACTIVATION_THRESHOLD){ 
						break;
					}
					if (actor_vel_y[i] > 0){
						actor->pos.y -= 16;
						actor_vel_y[i]--;
					}
					actor_counter_a[i]--;
					if (actor_counter_a[i] <= 0){
						actor_counter_a[i] = 180;
						actor_vel_y[i] = 24;
						actor_states[i] = 1; 
					}
					break;
				case 255: //Dead
					deactivate_actor(actor);
					break;
			}		
			break;			
			case 8://Bouncing entity
			switch(actor_states[i]){
				case 0: //Init
					if (((SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ actor_states[i] = 1; }
					break;
				case 1: //Main state
					current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD){ 
						break;
					}
					actor_vel_y[i] += (plat_grav >> 10);
					actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 10));
					//Apply velocity
					WORD new_y =  actor->pos.y + actor_vel_y[i];
					WORD new_x =  actor->pos.x + actor_vel_x[i];
					//Tile Collision
					actor->pos.x = check_collision(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
					if (actor->pos.x != new_x){
						actor_vel_x[i] = -actor_vel_x[i];
					}
					actor->pos.y = check_collision(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
					if (actor->pos.y < new_y){
						actor_vel_y[i] = -30;
					} else if (actor->pos.y > new_y){
						actor_vel_y[i] = 0;
					}
					//Animation
					if (actor_vel_x[i] < 0) {
						actor_set_dir(actor, DIR_LEFT, TRUE);
					} else if (actor_vel_x[i] > 0) {
						actor_set_dir(actor, DIR_RIGHT, TRUE);
					} else {
						actor_set_anim_idle(actor);
					}
					break;
				case 255: //Dead
					deactivate_actor(actor);
					break;
			}		
			break;				
			case 9://Fire ball
			switch(actor_states[i]){
				case 0: //Init
					if (((SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 						
					}
					break;
				case 1: //Main state
					current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD || SUBPX_TO_TILE(actor->pos.y) > image_tile_height){ 
						break;
					}
					actor_vel_y[i] += (plat_grav >> 9);
					actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));
					//Apply velocity
					WORD new_y =  actor->pos.y + actor_vel_y[i];
					WORD new_x =  actor->pos.x + actor_vel_x[i];
					//Tile Collision
					actor->pos.x = check_collision(new_x, actor->pos.y, &actor->bounds, ((actor->pos.x > new_x) ? CHECK_DIR_LEFT : CHECK_DIR_RIGHT));
					if (actor->pos.x != new_x){
						script_execute(actor->script.bank, actor->script.ptr, 0, 1, 2);
						actor_states[i] = 255;
						break;
					}
					actor->pos.y = check_collision(actor->pos.x, new_y, &actor->bounds, ((actor->pos.y > new_y) ? CHECK_DIR_UP : CHECK_DIR_DOWN));
					if (actor->pos.y < new_y){
						actor_vel_y[i] = -40;
					} else if (actor->pos.y > new_y){
						actor_vel_y[i] = 0;
					}
					//Actor Collision						
					actor_t * hit_actor = actor_overlapping_bb(&actor->bounds, &actor->pos, actor);
					if (hit_actor && hit_actor->script.bank && actor->collision_group != hit_actor->collision_group){						
						script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 4);
						script_execute(actor->script.bank, actor->script.ptr, 0, 1, 2);
						actor_states[i] = 255;
					}
					break;
				case 255: //Dead
					deactivate_actor(actor);
					break;
			}		
			break;			
			case 10://Knocked enemy
			switch(actor_states[i]){
				case 0: //Init
					if (((SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x) < BEHAVIOR_ACTIVATION_THRESHOLD){ 
						actor_states[i] = 1; 
						actor_vel_y[i] = -40;
						actor_vel_x[i] = 0;
						CLR_FLAG(actor->flags, ACTOR_FLAG_COLLISION);
					}
					break;
				case 1: //Main state
					current_actor_x = (SUBPX_TO_PX(actor->pos.x) + 8) - draw_scroll_x;
					if (current_actor_x > BEHAVIOR_DEACTIVATION_THRESHOLD || current_actor_x < BEHAVIOR_DEACTIVATION_LOWER_THRESHOLD || SUBPX_TO_TILE(actor->pos.y) > image_tile_height){ 
						actor_states[i] = 255; 
						break;
					}
					actor_vel_y[i] += (plat_grav >> 10);
					actor_vel_y[i] = MIN(actor_vel_y[i], (plat_max_fall_vel >> 8));
					//Apply velocity
					actor->pos.y =  actor->pos.y + actor_vel_y[i];					
					break;
				case 255: //Dead
					SET_FLAG(actor->flags, ACTOR_FLAG_COLLISION);
					actor_reset_anim(actor);
					deactivate_actor(actor);
					break;
			}		
			break;				
			case 11: //Hit block bump
			switch(actor_states[i]){
				case 0:
					//Actor Collision	
					for (UBYTE j = 1; j < MAX_ACTORS; j++){
						actor_t * hit_actor = (actors + j);
						if (!CHK_FLAG(hit_actor->flags, ACTOR_FLAG_ACTIVE) || hit_actor == actor || !CHK_FLAG(hit_actor->flags, ACTOR_FLAG_COLLISION)){
							continue;
						}
						if (bb_intersects(&actor->bounds, &actor->pos, &hit_actor->bounds, &hit_actor->pos)) {
							
							if ((hit_actor->pos.x > actor->pos.x && actor_vel_x[j] < 0) ||
								(hit_actor->pos.x < actor->pos.x && actor_vel_x[j] > 0)) {								
								actor_vel_x[j] = -actor_vel_x[j];
							}
							actor_vel_y[j] = -60;
							if (hit_actor->script.bank){
								script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 2);
							}
						}
					}
					actor_states[i] = 255;
					break;
			}
			break;	
		}			
	}
}

void vm_set_actor_behavior(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE behavior_id = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_behavior_ids[actor_idx] = behavior_id;
}

void vm_get_actor_behavior(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
	script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG1)] = actor_behavior_ids[actor_idx];
}

void vm_set_actor_state(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE state_id = *(uint8_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_states[actor_idx] = state_id;
}

void vm_get_actor_state(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
	script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG1)] = actor_states[actor_idx];
}

void vm_set_actor_velocity_x(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    WORD vel_x = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_vel_x[actor_idx] = vel_x;
}

void vm_set_actor_velocity_y(SCRIPT_CTX * THIS) OLDCALL BANKED {
    UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    WORD vel_y = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_vel_y[actor_idx] = vel_y;
}

void vm_set_actor_linked_actor_idx(SCRIPT_CTX * THIS) OLDCALL BANKED {
	UBYTE actor_idx = *(uint8_t *)VM_REF_TO_PTR(FN_ARG0);
    UBYTE linked_actor_idx = *(int16_t *)VM_REF_TO_PTR(FN_ARG1);
    actor_linked_actor_idx[actor_idx] = linked_actor_idx;
}