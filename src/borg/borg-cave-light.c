/**
 * \file  borg-cave-light.c
 * \brief handle lighted squares in the cave
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2007-9 Andi Sidwell, Chris Carr, Ed Graham, Erik Osheim
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband License":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

#include "borg-cave-light.h"

#ifdef ALLOW_BORG

#include "borg.h"
#include "borg-cave-util.h"
#include "borg-cave-view.h"
#include "borg-cave.h"
#include "borg-projection.h"
#include "borg-trait.h"

/*
 * Maintain a set of grids marked as "BORG_LIGHT"
 */

int16_t borg_light_n = 0;
uint8_t borg_light_x[AUTO_LIGHT_MAX];
uint8_t borg_light_y[AUTO_LIGHT_MAX];

/*
 * Maintain a set of grids marked as "BORG_GLOW"
 */

int16_t borg_glow_n = 0;

uint8_t borg_glow_x[AUTO_LIGHT_MAX];
uint8_t borg_glow_y[AUTO_LIGHT_MAX];

/*
 * XXX XXX XXX
 *
 * This macro allows us to efficiently add a grid to the "lite" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "lite" array, and we are never
 * called when the "lite" array is full.
 */
#define borg_cave_light_hack(y, x)                                             \
    borg_grids[y][x].info |= BORG_LIGHT;                                       \
    borg_light_y[borg_light_n] = (y);                                          \
    borg_light_x[borg_light_n] = (x);                                          \
    borg_light_n++

/*
 * Update the set of grids "illuminated" by the player's lite.
 *
 * See "update_LIGHT" in "cave.c" for complete documentation
 *
 * It is very important that the "player grid" be the first grid in the
 * array of "BORG_LIGHT" grids, since this is assumed in several places.
 */
void borg_update_light(void)
{
    int i, x, y, radius;

    /*** Clear old grids ***/

    /* Clear them all */
    for (i = 0; i < borg_light_n; i++) {
        y = borg_light_y[i];
        x = borg_light_x[i];

        /* Mark the grid as not "lite" */
        borg_grids[y][x].info &= ~BORG_LIGHT;
    }

    /* None left */
    borg_light_n = 0;

    /* Player has no lite */
    if (borg_items[INVEN_LIGHT].iqty <= 0)
        return;

    /*** Collect the new "lite" grids ***/

    /* Player grid */
    borg_cave_light_hack(borg.c.y, borg.c.x);

    /* The engine lights one grid less than the light's strength:
     * calc_lighting() uses radius = ABS(light) - 1 and add_light()
     * lights every grid with distance() no greater than that radius
     * which has line of sight from the player (see cave-view.c).
     * Using the strength itself made the borg expect to see monsters
     * standing in the dark ring just outside its real light, and
     * forget them when they were not on screen.  A Necromancer
     * standing in unlight sees by the unlight instead, so keep the
     * radius the borg always used for that case.
     */
    radius = borg.trait[BI_LIGHT] - 1;

    /* Never below zero */
    if (radius < 0)
        radius = 0;

    /* Necromancers see by unlight */
    if (borg.trait[BI_CLASS] == CLASS_NECROMANCER
        && borg.trait[BI_LIGHT] <= 1) {
        radius = borg.trait[BI_LIGHT];
    }

    /* Scan the box around the player, lighting what the engine lights */
    for (y = borg.c.y - radius; y <= borg.c.y + radius; y++) {
        for (x = borg.c.x - radius; x <= borg.c.x + radius; x++) {
            /* Stay on the map */
            if (y < 0 || y >= AUTO_MAX_Y || x < 0 || x >= AUTO_MAX_X)
                continue;

            /* The player grid is already lit (and must stay first) */
            if (y == borg.c.y && x == borg.c.x)
                continue;

            /* The engine lights distance() <= radius */
            if (distance(borg.c, loc(x, y)) > radius)
                continue;

            /* Don't propagate the light through walls */
            if (!borg_los(borg.c.y, borg.c.x, y, x))
                continue;

            /* This grid is "torch lit" */
            borg_cave_light_hack(y, x);
        }
    }
}

#endif
