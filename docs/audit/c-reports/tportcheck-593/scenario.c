/* Synthetic native C teleport landing-check fixture, excluded from the
 * source fix. Issue: diegoami/neo-angband-tracker#593 (tportcheck-593). */
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
#include "borg/borg-escape.h"
#include "borg/borg-fight-defend.h"
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
#define H 66
#define W 198

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
    const char *profile = getenv("TC593_PROFILE");
    const char *mode    = getenv("TC593_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(593);
    if (!player_make_simple("Human", "Warrior", "TportCheck593")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(99, 33));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 40;
    player_exp_gain(player, player_exp[38] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    /* Teleportation scrolls and a Staff of *Destruction*; in the control
     * also a Staff of Teleportation. */
    known_item(TV_SCROLL, "Teleportation", 5);
    struct object *dest = known_item(TV_STAFF, "*Destruction*", 1);
    dest->pval = 5;
    if (streq(mode, "control")) {
        struct object *tele = known_item(TV_STAFF, "Teleportation", 1);
        tele->pval = 5;
    }
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
    struct monster *foe = t_add_monster(cave, loc(100, 34), getenv("TC593_FOE") ? getenv("TC593_FOE") : "ancient blue dragon");
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
     * read first; current HP is then set to 300 (at least the 275 the
     * Destruction defence needs) and must stay below D, so the Borg is in
     * real danger (D above its avoidance). */
    set_hp(player->mhp, player->mhp);
    int d = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    if (d <= 300) { printf("SETUPFAIL danger too low d=%d\n", d); return 1; }
    set_hp(300, player->mhp);
    printf("SETUP seed=593 mode=%s class=%s clevel=%d hp=%d/%d danger=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp, d);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_defend(void *state)
{
    const char *mode = getenv("TC593_MODE");
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("STATE danger=%d curhp=%d avoidance=%d ateleport=%d aescape=%d\n", danger,
           borg.trait[BI_CURHP], borg.avoidance, borg.trait[BI_ATELEPORT], borg.trait[BI_AESCAPE]);
    /* The production landing check, as the Destruction defence calls it. */
    bool landing_ok = borg_caution_teleport(75, 2);
    /* The production defence decision. */
    bool defended = borg_defend(danger);
    keycode_t key = borg_inkey(false);
    printf("DECISION landing_ok=%d defended=%d key=%d\n", landing_ok ? 1 : 0, defended ? 1 : 0, (int)key);
    uint16_t after = messages_num();
    int destruction = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Destruction")) destruction = 1;
    }
    printf("OUTCOME landing_ok=%d uses_destruction=%d\n", landing_ok ? 1 : 0, destruction);
    /* With a safe landing available, the landing check must run and say so,
     * and the Borg must not destroy the area instead of teleporting. */
    require(landing_ok);
    require(!destruction);
    ok;
}

const char *suite_name = "native-c/tportcheck-593";
struct test tests[] = {
    {"teleport_landing_check_runs_with_scrolls", scenario_defend},
    {NULL, NULL}};
