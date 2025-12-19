#ifndef STATE_PLATFORM_H
#define STATE_PLATFORM_H

#include <gb/gb.h>
#include "events.h"
#include "gbs_types.h"

// Type Definitions -----------------------------------------------------------

typedef enum
{
    FALL_STATE = 0,
    GROUND_STATE,
    JUMP_STATE,
    DASH_STATE,
    LADDER_STATE,
    WALL_STATE,
    KNOCKBACK_STATE,
    BLANK_STATE,
    RUN_STATE,
    FLOAT_STATE,
} state_e;

typedef enum
{
    FALL_INIT = 0,
    FALL_END,
    GROUND_INIT,
    GROUND_END,
    JUMP_INIT,
    JUMP_END,
    DASH_INIT,
    DASH_END,
    LADDER_INIT,
    LADDER_END,
    WALL_INIT,
    WALL_END,
    KNOCKBACK_INIT,
    KNOCKBACK_END,
    BLANK_INIT,
    BLANK_END,
    DASH_READY,
    RUN_INIT,
    RUN_END,
    FLOAT_INIT,
    FLOAT_END,
	COIN_COLLECTED,
	HIT_BLOCK,
	ENTER_DOWN_PIPE,
	ENTER_UP_PIPE,
	ENTER_RIGHT_PIPE,
	ENTER_LEFT_PIPE,
	FELL_IN_PIT,
    CALLBACK_SIZE
} callback_e;

extern WORD plat_walk_vel;            // Maximum velocity while walking
extern WORD plat_run_vel;             // Maximum velocity while running
extern WORD plat_climb_vel;           // Maximum velocity while climbing
extern WORD plat_min_vel;             // Minimum velocity to apply to the player
extern WORD plat_walk_acc;            // Acceleration while walking
extern WORD plat_run_acc;             // Acceleration while running
extern WORD plat_dec;                 // Deceleration rate
extern WORD plat_jump_vel;            // Jump velocity applied on the first frame of jumping
extern WORD plat_grav;                // Gravity applied to the player
extern WORD plat_hold_grav;           // Gravity applied to the player while holding jump
extern WORD plat_max_fall_vel;        // Maximum fall velocity
extern UBYTE plat_camera_block;       // Limit the player's movement to the camera's edges
extern UBYTE plat_drop_through_active;// Drop-through is active
extern WORD plat_jump_hold_vel;       // Jump velocity applied while holding jump
extern UBYTE plat_jump_hold_frames;   // Maximum number for frames for additional jump velocity
extern UBYTE plat_extra_jumps;        // Number of jumps while in the air
extern WORD plat_jump_reduction;      // Reduce height each double jump
extern UBYTE plat_coyote_frames;      // Coyote Time maximum frames
extern UBYTE plat_jump_buffer_frames; // Jump Buffer maximum frames
extern UBYTE plat_wall_jump_max;      // Number of wall jumps in a row
extern UBYTE plat_wall_slide;         // Enables/Disables wall sliding
extern WORD plat_wall_slide_vel;      // Downwards velocity while clinging to the wall
extern WORD plat_wall_kick;           // Horizontal force for pushing off the wall
extern UBYTE plat_float_active;       // Float feature is active
extern WORD plat_float_vel;           // Speed of fall descent while floating
extern UBYTE plat_air_control;        // Enables/Disables air control
extern UBYTE plat_turn_control;       // Controls the amount of slippage when the player turns while running.
extern WORD plat_air_dec;             // air deceleration rate
extern WORD plat_turn_acc;            // Speed with which a character turns
extern WORD plat_run_boost;           // Additional jump height based on horizontal speed
extern UBYTE plat_dash_active;        // Dash feature is active
extern UBYTE plat_dash_from;          // Ground, air, ladders flags
extern UBYTE plat_dash_jump_from;     // Allow jumping from dash state
extern UBYTE plat_dash_mask;          // Choose if the player can dash through actors, triggers, and walls
extern WORD plat_dash_dist;           // Distance of the dash
extern UBYTE plat_dash_frames;        // Number of frames for dashing
extern UBYTE plat_dash_ready_frames;  // Frames before the player can dash again
extern UBYTE plat_dash_deadzone;      // Override camera x deadzone when in dash state
extern WORD plat_knockback_vel_x;     // Knockback velocity in the x direction
extern WORD plat_knockback_vel_y;     // Knockback velocity in the y direction
extern UBYTE plat_knockback_frames;   // Number of frames for knockback
extern WORD plat_blank_grav;          // Blank state gravity

extern UBYTE frame_coins;             // Coins collected during current frame
extern UBYTE hit_metatile_id;         // Id of the collided metatile
extern UBYTE hit_metatile_x;          // X position of the collided metatile
extern UBYTE hit_metatile_y;          // Y position of the collided metatile
extern UBYTE hit_metatile_source;     // actor Idx of the source of the hit metatile

extern script_event_t plat_events[CALLBACK_SIZE];

// State machine
extern state_e plat_state;            // Current platformer state
extern state_e plat_next_state;       // Next frame's platformer state

// Movement
extern WORD plat_vel_x;               // Tracks the player's x-velocity between frames
extern WORD plat_vel_y;               // Tracks the player's y-velocity between frames
extern WORD plat_delta_x;             // The subpixel offset to apply to the player's x position in move_and_collide
extern WORD plat_delta_y;             // The subpixel offset to apply to the player's y position in move_and_collide

// Counters
extern UBYTE plat_coyote_timer;       // Coyote Time Variable
extern UBYTE plat_jump_buffer_timer;  // Jump Buffer Variable
extern UBYTE plat_wall_coyote_timer;  // Wall Coyote Time Variable
extern UBYTE plat_hold_jump_timer;    // Jump input hold variable
extern UBYTE plat_extra_jumps_counter;// Current extra jump
extern UBYTE plat_wall_jump_counter;  // Current wall jump
extern UBYTE plat_knockback_timer;    // Current Knockback frame
extern UBYTE plat_drop_timer;         // The number of frames remaining to drop through platforms
extern UBYTE plat_nocontrol_h_timer;  // Turns off horizontal input, currently only for wall jumping

// Wall Jump
extern BYTE plat_wall_col;            // the wall type that the player is touching this frame
extern BYTE plat_last_wall_col;       // tracks the last wall the player touched

// Dash
extern UBYTE plat_dash_cooldown_timer;// tracks the current amount before the dash is ready
extern WORD plat_dash_per_frame;      // Takes overall dash distance and holds the amount per-frame
extern UBYTE plat_dash_currentframe;  // Tracks the current frame of the overall dash
extern BYTE plat_tap_timer;           // Number of frames since the last time left or right button was tapped

// Solid actors
extern actor_t *plat_attached_actor;  // The last actor the player hit, and that they were attached to
extern UBYTE plat_is_actor_attached;  // Keeps track of whether the player is currently on an actor
extern UWORD plat_attached_prev_x;    // Keeps track of the pos.x of the attached actor from the previous frame
extern UWORD plat_attached_prev_y;    // Keeps track of the pos.y of the attached actor from the previous frame
extern UWORD plat_temp_y;         // Temporary y position for the player when moving and colliding with an solid actor

// Jumping
extern WORD plat_jump_vel_per_frame;  // Holds the jump per frame value for the current jump
extern WORD plat_jump_reduction_vel;  // Holds the current jump reduction value based on the number of jumps without landing

// Camera
extern BYTE plat_camera_deadzone_x;   // Default camera deadzone x - used to restore camera after dash

// Ground
extern UBYTE plat_grounded;           // Tracks whether the player is on the ground or not
extern UBYTE plat_on_slope;           // Tracks whether the player is on a slope or not
extern UBYTE plat_slope_y;            // The y position of the slope the player is currently on

// Collision
extern UBYTE did_interact_actor;      // Tracks whether the player interacted with an actor this frame

// Ladders
extern UBYTE plat_ladder_block_h;     // Track if player has released horizontal input since joining the ladder
extern UBYTE plat_ladder_block_v;     // Track if player has released vertical input since leaving the ladder

// Animation
extern UBYTE plat_anim_dirty;         // Tracks whether the player animation has been modified from the default

// Variables for plugins
extern BYTE plat_run_stage;           // Tracks the stage of running based on the run type
extern UBYTE plat_jump_type;          // Tracks the type of jumping, from the ground, in the air, or off the wall

void platform_init(void) BANKED;
void platform_update(void) BANKED;

#endif
