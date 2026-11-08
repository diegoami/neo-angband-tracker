/* Synthetic native C detection-carryover diagnostic, excluded from the source fix. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "cmds.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("DC219_PROFILE");
    const char *mode = getenv("DC219_MODE");
    if (!profile) return 1;
    printf("SETUPSTEP profile\n");
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Warrior", "DetectCarryover")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_place(cave, player, loc(11, 11));
    calc_inventory(player);
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    borg.status.active = true;
    borg_update_entrypoint(true);
    borg_update();
    /* Fixture intervention: the per-panel detection flags in the state a
     * detect-objects cast on the previous level left behind (panel 0 is
     * the current panel). The wall flag is planted too: it is cleared by
     * the same loop on a level change and serves as the witness that the
     * pass ran. */
    borg_detect_obj[0][0] = true;
    borg_detect_wall[0][0] = true;
    if (!mode || streq(mode, "new-level")) {
        borg.status.old_depth = 30;
        borg.trait[BI_CDEPTH] = 31;
    } else {
        borg.status.old_depth = 30;
        borg.trait[BI_CDEPTH] = 30;
    }
    printf("SETUP olddepth=%d curdepth=%d panels=%d,%d\n",
        borg.status.old_depth, borg.trait[BI_CDEPTH], borg.panels.x, borg.panels.y);
    printf("SETUP seed=1000 class=%s level=%d hp=%d/%d\n",
        player->class->name, player->lev, player->chp, player->mhp);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("DC219_MODE");
    if (!mode) mode = "new-level";
    printf("MODE %s\n", mode);

    /* The production update pass: on a depth change it clears the
     * per-panel detection flags. */
    borg_update();
    int obj = borg_detect_obj[0][0] ? 1 : 0;
    int wall = borg_detect_wall[0][0] ? 1 : 0;
    printf("STATE obj=%d wall=%d\n", obj, wall);
    printf("OUTCOME obj_cleared=%d wall_cleared=%d\n", obj ? 0 : 1, wall ? 0 : 1);
    if (streq(mode, "new-level")) {
        /* The carryover case: the object-detection flag from the previous
         * level must be cleared with the others. The original leaves it
         * set, so the new level's panel still counts as detected. */
        require(obj == 0);
        require(wall == 0);
    } else {
        /* Control: no depth change; the flags are untouched in both
         * variants. Complete logs are byte-identical. */
        require(obj == 1);
        require(wall == 1);
    }
    ok;
}
const char *suite_name = "native-c/detectcarry-219";
struct test tests[] = {{"detection_flags_cleared_on_new_level", scenario}, {NULL, NULL}};
