/* Synthetic native C Phase Door landing-model fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#595 (phasemodel-595). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "object.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-escape.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 40
#define W 60

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 10, MAXIMISE);
    obj->number = number;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static int observe(struct monster *mon, const char *tag)
{
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!mflag_has(mon->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL %s visible\n", tag); return 1; }
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL %s observe\n", tag); return 1; }
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL %s kill_index\n", tag); return 1; }
    borg_kills[k].awake = 1;
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("PM595_PROFILE");
    const char *mode    = getenv("PM595_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(595);
    if (!player_make_simple("Human", "Warrior", "PhaseModel595")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_place(cave, player, loc(30, 20));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 17;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    known_item(TV_SCROLL, "Phase Door", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    borg_equipment(); borg_inventory(); borg_notice(true); borg_notice_player();
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW | BORG_GLOW;
    }
    update_view(cave, player);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    /* "ring": grey molds (they never move) on every grid at distance 6 from
     * the Borg, so only landings 5 to 7 squares away are next to one;
     * "control": no monsters. */
    int molds = 0;
    if (streq(mode, "ring")) {
        for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x) {
            if (distance(player->grid, loc(x, y)) != 6) continue;
            struct monster *m = t_add_monster(cave, loc(x, y), "grey mold");
            if (!m) { printf("SETUPFAIL mold\n"); return 1; }
            if (observe(m, "mold")) return 1;
            molds++;
        }
    }
    /* Fixture intervention: few hit points, so a landing next to a mold is
     * "scary" (its danger exceeds them). */
    player->chp = 3; borg_notice(true); borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=595 mode=%s class=%s clevel=%d hp=%d/%d molds=%d\n", mode,
           player->class->name, player->lev, player->chp, player->mhp, molds);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_phase(void *state)
{
    const char *mode = getenv("PM595_MODE");
    /* A Phase Door scroll is carried (borg_caution_phase needs BI_APHASE). */
    uint16_t before = messages_num();
    bool safe = borg_caution_phase(20, 2);
    uint16_t after = messages_num();
    int scary = -1;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        const char *c = strstr(m, "scary squares: ");
        if (c) scary = atoi(c + 15);
    }
    printf("OUTCOME safe=%d scary=%d\n", safe ? 1 : 0, scary);
    /* The game lands a Phase Door 9 to 11 squares away, where no mold is
     * adjacent: the check must find it safe, in both modes. */
    require(safe);
    ok;
}

const char *suite_name = "native-c/phasemodel-595";
struct test tests[] = {
    {"phase_check_judges_real_landings", scenario_phase},
    {NULL, NULL}};
