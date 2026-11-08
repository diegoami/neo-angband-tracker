/* Synthetic native C munchkin stair-range fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#245 (stairrange-245). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-power.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include "borg/borg-io.h"
#include "borg/borg-think-dungeon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("SR245_PROFILE");
    if (!profile) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(245);
    if (!player_make_simple("Human", "Warrior", "StairRange245")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 10;
    player->lev = 5;
    character_dungeon = true;
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* A known up staircase five grids north: outside the shrunk range of a
     * level-5 borg (3) but inside the base range (8). */
    square_set_feat(cave, loc(12, 7), FEAT_LESS);
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
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x)
        square_set_feat(player->cave, loc(x, y), square(cave, loc(x, y))->feat);
    /* The borg has tracked the up staircase. */
    track_less.x[0] = 12; track_less.y[0] = 7; track_less.num = 1;
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.status.active = true;
    borg_update_entrypoint(true);
    borg_cfg[BORG_MUNCHKIN_START] = true;
    printf("SETUP seed=245 class=%s depth=%d clevel=%d\n",
           player->class->name, player->depth, player->lev);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static void print_notes(void)
{
    uint16_t total = messages_num();
    uint16_t show = total < 12 ? total : 12;
    for (uint16_t i = 0; i < show; ++i)
        printf("NOTE %02d %s\n", (int)i, message_str(i));
}

static int scenario_range(void *state)
{
    /* A level-5 munchkin borg at depth 10 with a known up staircase five
     * grids away. The base stair range is 8; the buggy "increase" shrinks
     * it to 3, so no staircase counts as close and the borg never
     * power-climbs toward the stairs it can see. The correction keeps the
     * base range below level 10, and the climb decision fires. */
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg.munchkin_mode = false;
    borg.times_twitch = 0;
    borg.no_retreat = 0;
    borg.c = player->grid;
    borg.panels.x = 1;
    borg.panels.y = 2;
    borg_alloc_detection();
    borg.power = borg_power();
    {
        uint16_t total = messages_num();
        for (uint16_t i = 0; i < total; ++i) (void)message_str(i);
    }

    bool rc = borg_think_dungeon();

    printf("STATE clevel=%d depth=%d goal_less=%d fleeing=%d munchkin_mode=%d rc=%d\n",
           borg.trait[BI_CLEVEL], borg.trait[BI_CDEPTH],
           borg.goal.less ? 1 : 0, borg.goal.fleeing ? 1 : 0,
           borg.munchkin_mode ? 1 : 0, rc ? 1 : 0);
    print_notes();
    printf("OUTCOME climb_decision=%d\n", borg.goal.less ? 1 : 0);
    require(borg.trait[BI_CLEVEL] == 5);
    require(borg.trait[BI_CDEPTH] == 10);
    require(borg.goal.less);
    ok;
}

const char *suite_name = "native-c/stairrange-245";
struct test tests[] = {
    {"shrunk_range_blocks_climb", scenario_range},
    {NULL, NULL}};
