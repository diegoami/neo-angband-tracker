/* Synthetic native C glow-marking fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#213 (glowtrace-213). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-light.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow-misc.h"
#include "borg/borg-flow.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-power.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("GLOW213_PROFILE");
    if (!profile) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(213);
    if (!player_make_simple("Human", "Warrior", "GlowTrace213")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 9;
    character_dungeon = true;
    /* A uniformly permanently-glowing arena floor: every interior grid the
     * game reports as lit (LIGHTING via SQUARE_GLOW) regardless of the
     * borg's own torch. */
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH; update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    memcpy(safe_items, borg_items, QUIVER_END * sizeof(borg_item));
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* The whole glowing arena is in the player's sight; memorize it into
     * the knowledge cave exactly as a viewed room would be, so the map
     * update sees known floors out to the walls. */
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x)
        square_set_feat(player->cave, loc(x, y), square(cave, loc(x, y))->feat);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=213 class=%s depth=%d hp=%d\n",
           player->class->name, player->depth, player->chp);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_glow(void *state)
{
    /* The game's own lighting must reach the borg's grid flags through
     * the production map update. The arena floor is permanently glowing;
     * a grid far outside the borg's own torch radius must end up flagged
     * BORG_GLOW, walls must not, and the flow-toward-light consumer must
     * find glowing grids to flow to. */
    borg_update();

    bool distant = (borg_grids[3][3].info & BORG_GLOW) != 0;
    bool wall    = (borg_grids[0][5].info & BORG_GLOW) != 0;
    bool near    = (borg_grids[12][11].info & BORG_GLOW) != 0;
    printf("STATE distant_glow=%d near_glow=%d wall_glow=%d marked=%d lit=%d pcmem=%d info33=%u info1211=%u\n",
           distant, near, wall, (borg_grids[3][3].info & BORG_MARK) != 0,
           square_islit(cave, loc(3, 3)) ? 1 : 0,
           (int)square(player->cave, loc(3, 3))->feat,
           (unsigned)borg_grids[3][3].info, (unsigned)borg_grids[12][11].info);

    require(!wall);    /* perm walls are not floors: never flagged glowing */
    require(near);     /* glowing floor beside the borg */
    require(distant);  /* glowing floor far outside the torch radius */

    /* Consumer: the flow toward light builds its target list purely from
     * BORG_GLOW grids and bails before building any flow when the list is
     * empty. The borg stands on glowing floor, so the one-step tail of the
     * flow may decline to move; the list is the decision-relevant part. */
    borg_flow_light(GOAL_XTRA);
    printf("OUTCOME glow_n=%d\n", (int)borg_glow_n);
    require(borg_glow_n > 0);
    ok;
}

const char *suite_name = "native-c/glowtrace-213";
struct test tests[] = {
    {"glow_marked_and_flow_light_works", scenario_glow},
    {NULL, NULL}};
