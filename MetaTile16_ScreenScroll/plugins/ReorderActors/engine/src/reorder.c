#pragma bank 255

#include "reorder.h"

#include <gbdk/platform.h>
#include "data_manager.h"
#include "system.h"
#include "gbs_types.h"
#include "events.h"
#include "actor.h"
#include "math.h"
#include "vm.h"

//Arrays
UBYTE bMarkForReorder[MAX_ACTORS];
UBYTE bMarkForTemp[MAX_ACTORS];

//Functions
void reorder_all(void) BANKED
{
    //Temp
    UBYTE i, j, tileCheckY;
    actor_t *actor;

    //Preprocess
    for(i = 1;i != MAX_ACTORS;i ++) //Exclude Player
    {
        bMarkForReorder[i] = 0;
        actor = (actors + i);
        if (actor->active)
        {
            //Deactivate
            bMarkForReorder[i] = 1;
            deactivate_actor(actor);
        }
    }

    //Depth Loop
    for(j = image_tile_height; j != 0; j--)
    {
        for(i = 1;i != MAX_ACTORS;i ++) //Exclude Player
        {
            actor = (actors + i);
            if (!actor->active && bMarkForReorder[i] == 1)
            {
                //Get
                tileCheckY = (actor->pos.y >> 4) >> 3;
                if (tileCheckY == j)
                {
                    //Activate
                    bMarkForReorder[i] = 0;
                    activate_actor(actor);
                }
            }
        }
    }
}

void temp_deactivate_all(void) BANKED
{
    //Temp
    UBYTE i;
    actor_t *actor;

    //Loop
    for(i = 1;i != MAX_ACTORS;i ++) //Exclude Player
    {
        bMarkForTemp[i] = 0;
        actor = (actors + i);
        if (actor->active)
        {
            //Deactivate
            bMarkForTemp[i] = 1;
            deactivate_actor(actor);
        }
    }
}
void temp_activate_all(void) BANKED
{
    //Temp
    UBYTE i;
    actor_t *actor;

    //Loop
    for(i = 1;i != MAX_ACTORS;i ++) //Exclude Player
    {
        actor = (actors + i);
        if (!actor->active && bMarkForTemp[i] == 1)
        {
            //Activate
            bMarkForTemp[i] = 0;
            activate_actor(actor);
        }
    }
}