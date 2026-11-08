/* Synthetic native C clock-overflow fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#246 (clockreset-246). */
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
#include "borg/borg-cave.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-power.h"
#include "borg/borg-think-dungeon.h"
#include "borg/borg-trait.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("CR246_PROFILE");
    if (!profile) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(246);
    if (!player_make_simple("Human", "Warrior", "ClockReset246")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 9;
    character_dungeon = true;
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
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x)
        square_set_feat(player->cave, loc(x, y), square(cave, loc(x, y))->feat);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=246 class=%s depth=%d hp=%d\n",
           player->class->name, player->depth, player->chp);
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
    uint16_t show = total < 10 ? total : 10;
    for (uint16_t i = 0; i < show; ++i)
        printf("NOTE %02d %s\n", (int)i, message_str(i));
}

static int scenario_threshold(void *state)
{
    /* The borg clock sits exactly on the abort threshold (BORG_TIME_MAX -
     * 3000). The abort fired about 2900 turns earlier than the graceful
     * reset threshold, so the original deactivates the borg here; the
     * correction resets the clock in place and keeps playing. */
    borg.time.now = BORG_TIME_MAX - 3000;
    {
        uint16_t total = messages_num();
        for (uint16_t i = 0; i < total; ++i) (void)message_str(i);
    }

    borg_think_dungeon();

    printf("STATE time_after=%d active=%d\n", (int)borg.time.now,
           borg.status.active ? 1 : 0);
    print_notes();
    printf("OUTCOME kept_playing=%d time_reset=%d\n",
           borg.status.active ? 1 : 0,
           borg.time.now < 1000 ? 1 : 0);
    require(borg.time.now < 1000);
    require(borg.status.active);
    ok;
}

const char *suite_name = "native-c/clockreset-246";
struct test tests[] = {
    {"threshold_resets_and_keeps_playing", scenario_threshold},
    {NULL, NULL}};
