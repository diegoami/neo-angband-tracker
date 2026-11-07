/* Synthetic native C call-light own-grid diagnostic, excluded from the source fix. */
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
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("CL231_PROFILE");
    const char *mode = getenv("CL231_MODE");
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
    if (!player_make_simple("Human", "Warrior", "CallLightOwnGrid")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    if (!mode || streq(mode, "own-open-door")) {
        /* The Borg stands in an open doorway. */
        square_set_feat(cave, loc(11, 11), FEAT_OPEN);
    }
    player_place(cave, player, loc(11, 11));
    calc_inventory(player);
    /* A burned-out torch puts the routine into its out-of-light path;
     * the neighbours are all lit so nothing else in the scan can fire.
     * Spare torches and the recall scroll are removed from the pack
     * (recorded fixture intervention) so the wear-stuff, refuel and
     * recall branches have nothing to act on before the scan. The
     * surgery and the torch fuel run before borg_init so the borg's
     * own copies see them. */
    for (int i = 0; i < z_info->pack_size; ++i) {
        struct object *o = player->upkeep->inven[i];
        if (o && (o->tval == TV_LIGHT || o->tval == TV_SCROLL)) o->number = 0;
    }
    {
        struct object *torch = slot_object(player, slot_by_name(player, "light"));
        if (!torch) { printf("SETUPFAIL torch\n"); return 1; }
        object_flavor_aware(player, torch);
        player_know_object(player, torch);
        torch->timeout = 0;
    }
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW | BORG_MARK | BORG_GLOW;
    }
    if (!mode || streq(mode, "own-open-door")) {
        /* Fixture intervention: the Borg's own grid carries no glow mark
         * in its map (the state the door case exists to repair by
         * cheating the game's glow). */
        borg_grids[11][11].info &= ~BORG_GLOW;
        borg_grids[11][11].info |= BORG_DARK;
    }
    printf("SETUP ownfeat=%d ownglow_before=%d isglow=%d\n",
        borg_grids[11][11].feat,
        borg_grids[11][11].info & BORG_GLOW ? 1 : 0,
        square_isglow(cave, loc(11, 11)) ? 1 : 0);
    {
        struct object *t = slot_object(player, slot_by_name(player, "light"));
        printf("SETUP player-torch-timeout=%d borg-timeout=%d\n",
            t ? (int)t->timeout : -1, borg_items[INVEN_LIGHT].timeout);
    }
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int drain_keys(keycode_t *keys, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) keys[len++] = key;
    return len;
}

static void print_keys(const keycode_t *keys, int len)
{
    for (int i = 0; i < len; ++i) {
        if (i) putchar(' ');
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');
}

static int scenario(void *state)
{
    const char *mode = getenv("CL231_MODE");
    keycode_t keys[64];
    int len;
    if (!mode) mode = "own-open-door";
    printf("MODE %s\n", mode);

    /* The production out-of-light routine: its call-light scan is the
     * section under test. */
    bool acted = borg_think_dungeon_light();
    len = drain_keys(keys, 64);
    printf("DECISION acted=%d keys=%d sequence=", acted, len);
    print_keys(keys, len);
    int ownglow = borg_grids[11][11].info & BORG_GLOW ? 1 : 0;
    printf("STATE ownglow=%d\n", ownglow);
    printf("OUTCOME ownglow=%d\n", ownglow);
    if (streq(mode, "own-open-door")) {
        /* The misgate case: the Borg's own grid is an open door and the
         * game says the door grid is lit. The door case tests the Borg's
         * own grid, but the scan never visits it, so the lit mark is
         * never repaired. The corrected scan visits the own grid and
         * marks it. */
        require(ownglow == 1);
    } else {
        /* Control: the own grid is a plain lit floor. Both variants
         * leave the marks alone. Complete logs are byte-identical. */
        require(ownglow == 1);
    }
    ok;
}
const char *suite_name = "native-c/calllight-231";
struct test tests[] = {{"own_grid_door_lit_mark_repaired", scenario}, {NULL, NULL}};
