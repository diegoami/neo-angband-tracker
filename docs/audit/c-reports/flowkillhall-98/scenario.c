/* Synthetic native C monster-hunt fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#98 (flowkillhall-98). */
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
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow.h"
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
    object_prep(obj, kind, 50, MAXIMISE);
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


/* Modes:
 *  leak      open room; a soldier ant two steps east is noted first, a
 *            second soldier ant five steps south is noted second.
 *  reverse   control: the same two ants, the far one noted first.
 *  hall      the Borg stands in a corridor; a wild dog (GROUP_AI) is out
 *            in the open room 15 steps east.
 *  hallctl   control: the Borg in the corridor, the wild dog also in the
 *            corridor 5 steps west.
 */
static const char *mode;

int setup_tests(void **state)
{
    const char *profile = getenv("FK98_PROFILE");
    mode = getenv("FK98_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(98);
    if (!player_make_simple("Human", "Warrior", "FlowKill98")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    bool corridor = streq(mode, "hall") || streq(mode, "hallctl");
    /* Corridor modes: granite fills columns 1..14 except row 12, leaving a
     * one-wide corridor that opens into the lit room at column 15. */
    if (corridor)
        for (int y = 1; y < H - 1; ++y) for (int x = 1; x <= 14; ++x)
            if (y != 12) square_set_feat(cave, loc(x, y), FEAT_GRANITE);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    borg_notice(true);
    borg.avoidance = player->chp;
    /* Monsters, noted by the Borg in the order listed. */
    struct { const char *race; struct loc at; } m[2];
    int n = 0;
    if (streq(mode, "leak")) {
        m[n].race = "Soldier ant"; m[n].at = loc(12, 12); n++;
        m[n].race = "Soldier ant"; m[n].at = loc(10, 17); n++;
    } else if (streq(mode, "reverse")) {
        m[n].race = "Soldier ant"; m[n].at = loc(10, 17); n++;
        m[n].race = "Soldier ant"; m[n].at = loc(12, 12); n++;
    } else if (streq(mode, "hall")) {
        m[n].race = "Wild dog"; m[n].at = loc(25, 12); n++;
    } else if (streq(mode, "hallctl")) {
        m[n].race = "Wild dog"; m[n].at = loc(5, 12); n++;
    } else { printf("SETUPFAIL mode\n"); return 1; }
    for (int i = 0; i < n; ++i) {
        struct monster *mon = t_add_monster(cave, m[i].at, m[i].race);
        if (!mon) { printf("SETUPFAIL monster %d\n", i); return 1; }
        if (observe(mon, m[i].race)) return 1;
    }
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=98 mode=%s class=%s clevel=%d hp=%d depth=%d corridor=%d\n",
           mode, player->class->name, player->lev, player->chp, player->depth, corridor ? 1 : 0);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_flow_kill(void *state)
{
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    for (int k = 1; k < borg_kills_nxt; ++k) {
        borg_kill *kill = &borg_kills[k];
        if (!kill->r_idx) continue;
        printf("KILL k=%d race=%s pos=(%d,%d) dist=%d ranged=%d group_ai=%d\n", k,
               r_info[kill->r_idx].name, kill->pos.x, kill->pos.y,
               distance(borg.c, kill->pos), kill->ranged_attack,
               rf_has(r_info[kill->r_idx].flags, RF_GROUP_AI) ? 1 : 0);
    }
    /* Production call: hunt a visible monster (viewable, nearness 250). */
    bool rc = borg_flow_kill(true, 250);
    keycode_t key = borg_inkey(false);
    printf("DECISION rc=%d key=%d goal=%d\n", rc ? 1 : 0, (int)key, borg.goal.type);
    for (int i = 0; i < borg_temp_n; ++i)
        printf("TRACK target (%d,%d)\n", borg_temp_x[i], borg_temp_y[i]);
    printf("STATE targets=%d hp=%d avoidance=%d\n", borg_temp_n,
           borg.trait[BI_CURHP], borg.avoidance);
    int flowed = rc ? 1 : 0;
    printf("OUTCOME flowed=%d\n", flowed);
    /* Corrected contract: in the open room the farther ant is still hunted
     * after the two-step one is skipped; from a corridor the Borg does not
     * leave it for a pack monster out in the open room. */
    int want = streq(mode, "hall") ? 0 : 1;
    require(flowed == want);
    ok;
}

const char *suite_name = "native-c/flowkillhall-98";
struct test tests[] = {
    {"hunt_decision_per_monster", scenario_flow_kill},
    {NULL, NULL}};
