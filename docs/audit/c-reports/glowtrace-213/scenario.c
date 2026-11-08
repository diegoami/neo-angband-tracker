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
static const char *glow_mode = "main";
static int scenario_door(void *state);
static int scenario_torchlit(void *state);
static int scenario_unlit(void *state);

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
    /* GLOW213_MODE: "main" (default), "door" (unchanged control: an open
     * door in the same glowing arena), "torchlit" (unchanged control: an
     * arena with no permanent glow, so only the Borg's own light source
     * lights nearby floors), "unlit" (the same arena: a floor outside the
     * Borg's light radius). */
    if (getenv("GLOW213_MODE")) glow_mode = getenv("GLOW213_MODE");
    if (!streq(glow_mode, "torchlit") && !streq(glow_mode, "unlit"))
        for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
            sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    if (streq(glow_mode, "door"))
        square_set_feat(cave, loc(10, 12), FEAT_OPEN);
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
    printf("SETUP seed=213 class=%s depth=%d hp=%d mode=%s\n",
           player->class->name, player->depth, player->chp, glow_mode);
    if (streq(glow_mode, "door")) {
        tests[0].name = "door_and_wall_flags_unchanged";
        tests[0].func = scenario_door;
    } else if (streq(glow_mode, "torchlit")) {
        tests[0].name = "torchlit_floor_flags";
        tests[0].func = scenario_torchlit;
    } else if (streq(glow_mode, "unlit")) {
        tests[0].name = "unlit_floor_marked_dark";
        tests[0].func = scenario_unlit;
    }
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

/* Control: non-floor grids (an open door and a permanent wall) in the
 * same glowing arena. The correction touches only the Floors branch, so
 * these grids' flags must be identical before and after. Only the door
 * and wall grids are printed. */
static int scenario_door(void *state)
{
    borg_update();
    unsigned door = borg_grids[12][10].info, wall = borg_grids[0][5].info;
    printf("STATE door_feat=%d door_info=%u door_glow=%d door_dark=%d wall_info=%u wall_glow=%d\n",
           (int)borg_grids[12][10].feat, door, (door & BORG_GLOW) != 0,
           (door & BORG_DARK) != 0, wall, (wall & BORG_GLOW) != 0);
    printf("OUTCOME door_is_open=%d wall_glow=%d\n",
           borg_grids[12][10].feat == FEAT_OPEN ? 1 : 0, (wall & BORG_GLOW) != 0);
    require(borg_grids[12][10].feat == FEAT_OPEN);
    require(!(wall & BORG_GLOW));
    ok;
}

/* Control: an arena with no permanent glow. The floor next to the Borg
 * is lit only by its own light source (square_light 1, no SQUARE_GLOW).
 * A torch-lit floor is not a glowing floor: it must not be flagged
 * BORG_GLOW, and the flow toward light, which takes its targets only
 * from BORG_GLOW grids, must find none. Only the torch-lit grid and the
 * flow-toward-light list are printed; the log must be identical before
 * and after. */
static int scenario_torchlit(void *state)
{
    borg_update();
    struct loc nearg = loc(11, 12);
    unsigned ni = borg_grids[12][11].info;
    printf("STATE near_isglow=%d near_light=%d near_islit=%d near_glow=%d near_dark=%d near_light_flag=%d\n",
           square_isglow(cave, nearg) ? 1 : 0, square_light(cave, nearg),
           square_islit(cave, nearg) ? 1 : 0, (ni & BORG_GLOW) != 0,
           (ni & BORG_DARK) != 0, (ni & BORG_LIGHT) != 0);
    borg_flow_light(GOAL_XTRA);
    printf("OUTCOME torchlit_marked_glow=%d glow_n=%d\n",
           (ni & BORG_GLOW) != 0, (int)borg_glow_n);
    require(!(ni & BORG_GLOW));
    require(borg_glow_n == 0);
    ok;
}

/* The same arena with no permanent glow: a known floor far outside the
 * Borg's light radius is neither glowing nor lit, so the Floors branch
 * marks it BORG_DARK. The original never ran that branch. */
static int scenario_unlit(void *state)
{
    borg_update();
    struct loc farg = loc(3, 3);
    unsigned fi = borg_grids[3][3].info;
    printf("STATE far_isglow=%d far_light=%d far_islit=%d far_glow=%d far_dark=%d\n",
           square_isglow(cave, farg) ? 1 : 0, square_light(cave, farg),
           square_islit(cave, farg) ? 1 : 0, (fi & BORG_GLOW) != 0,
           (fi & BORG_DARK) != 0);
    printf("OUTCOME far_dark=%d\n", (fi & BORG_DARK) != 0);
    require(!(fi & BORG_GLOW));
    require(fi & BORG_DARK);
    ok;
}

const char *suite_name = "native-c/glowtrace-213";
struct test tests[] = {
    {"glow_marked_and_flow_light_works", scenario_glow},
    {NULL, NULL}};
