/* Synthetic native C emergency-escape fixture, excluded from the source
 * fix. Issue: diegoami/neo-angband-tracker#590 (deepdescent-590). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "object.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-prefs.h"
#include "ui-term.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-stairs.h"
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
#define H 24
#define W 40

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
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

/* Set the game's and the Borg's hit points. */
static void set_hp(int cur, int max)
{
    player->mhp = max;
    player->chp = cur;
    borg_notice(true);
    borg.avoidance = player->chp;
}

int setup_tests(void **state)
{
    const char *profile = getenv("DD590_PROFILE");
    const char *mode    = getenv("DD590_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(590);
    if (!player_make_simple("Human", "Warrior", "DeepDescent590")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 10;
    player_exp_gain(player, player_exp[8] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    /* Escapes carried: a Deep Descent scroll and Phase Door scrolls; in the
     * control also a Teleport Level scroll. No teleport, no potions. */
    known_item(TV_SCROLL, "Deep Descent", 1);
    known_item(TV_SCROLL, "Phase Door", 5);
    if (streq(mode, "control")) known_item(TV_SCROLL, "Teleport Level", 1);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true); borg_notice_player();
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    /* One awake black ogre adjacent (south-east). */
    struct monster *foe = t_add_monster(cave, loc(11, 13), "black ogre");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    mflag_on(foe->mflag, MFLAG_AWARE); update_mon(foe, cave, true);
    if (!mflag_has(foe->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(foe->grid.y, foe->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[k].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    /* Fixture intervention: hit points. The danger D at the Borg's grid is
     * read first; current HP (and so the avoidance) is then set to D / 2, so
     * D exceeds 1.5 times the avoidance: the "about to die" escape (Danger
     * Level 1). */
    set_hp(player->mhp, player->mhp);
    int d = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    set_hp(d / 2, player->mhp);
    printf("SETUP seed=590 mode=%s class=%s clevel=%d hp=%d/%d danger=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp, d);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_escape(void *state)
{
    const char *mode = getenv("DD590_MODE");
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("STATE danger=%d curhp=%d avoidance=%d\n", danger, borg.trait[BI_CURHP], borg.avoidance);
    bool rc = borg_caution();
    keycode_t key = borg_inkey(false);
    printf("DECISION rc=%d key=%d\n", rc ? 1 : 0, (int)key);
    uint16_t after = messages_num();
    int dd = 0, phase = 0, tlev = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Deep Descent")) dd = 1;
        if (strstr(m, "Phase Door")) phase = 1;
        if (strstr(m, "Teleport Level")) tlev = 1;
    }
    printf("OUTCOME reads_deep_descent=%d reads_phase=%d reads_teleport_level=%d\n", dd, phase, tlev);
    if (streq(mode, "control"))
        /* Teleport Level is tried first in both builds. */
        require(tlev && !dd);
    else
        /* Deep Descent is no emergency escape; the Borg must not spend the
         * turn on it. */
        require(!dd);
    ok;
}

const char *suite_name = "native-c/deepdescent-590";
struct test tests[] = {
    {"no_deep_descent_as_emergency_escape", scenario_escape},
    {NULL, NULL}};
