/* Synthetic native C sleeping-monster ranged-attack fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#63 (hitchance-63). */
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
#include "trap.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
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
    borg_kills[k].awake = 0;
    mon_clear_timed(mon, MON_TMD_SLEEP, MON_TMD_FLG_NOMESSAGE);
    mon_inc_timed(mon, MON_TMD_SLEEP, 100, MON_TMD_FLG_NOMESSAGE);
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("HC63_PROFILE");
    const char *mode    = getenv("HC63_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(63);
    if (!player_make_simple("Human", "Warrior", "HitChance63")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[13] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* A staircase two steps west and, except in the control, a second one
     * of the same kind 25 steps east.  Up staircases, or down staircases in
     * the "twodown" case. */
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
    /* The Borg has seen both staircases. */
    track_less.num = track_more.num = 0;
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    /* Some time into the level; no anti-summon corridor in use. */
    borg.time.now = 1000;
    borg.time.level = borg.time.now;
    borg.time.antisummon = 0;
    borg.time.town = borg.time.now;
    /* Full hit points. */
    player->chp = player->mhp;
    borg_notice(true);
    borg.avoidance = player->chp;
    /* A monster asleep six grids east, in view. */
    struct monster *foe = t_add_monster(cave, loc(16, 12), streq(mode, "wretch") ? "boil-covered wretch" : "Cave troll");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    if (observe(foe, "foe")) return 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=63 mode=%s class=%s clevel=%d hp=%d depth=%d \n",
           mode, player->class->name, player->lev, player->chp, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_wake(void *state)
{
    const char *mode = getenv("HC63_MODE");
    borg_flush();
    borg_notice(true);
    int k = 1;
    int partial = borg_danger_one_kill(borg_kills[k].pos.y, borg_kills[k].pos.x, 1, k, true, false);
    int full    = borg_danger_one_kill(borg_kills[k].pos.y, borg_kills[k].pos.x, 1, k, true, true);
    printf("KILL k=%d r=%d name=%s pos=(%d,%d) awake=%d power=%d\n", k, borg_kills[k].r_idx,
           r_info[borg_kills[k].r_idx].name, borg_kills[k].pos.x, borg_kills[k].pos.y,
           borg_kills[k].awake, borg_kills[k].power);
    printf("STATE avoidance=%d half_avoidance=%d hp=%d ac=%d\n", borg.avoidance,
           borg.avoidance / 2, borg.trait[BI_CURHP], borg.trait[BI_ARMOR]);
    printf("TRACK partial_danger=%d full_danger=%d\n", partial, full);
    /* The target list borg_attack() builds for the one monster in view,
     * and the attacking flag it sets before scoring. */
    borg_attacking = true;
    borg_temp_n    = 1;
    borg_temp_x[0] = borg_kills[k].pos.x;
    borg_temp_y[0] = borg_kills[k].pos.y;
    int score = borg_launch_bolt(0, 12, BORG_ATTACK_MISSILE, 10, 0);
    printf("DECISION bolt_score=%d\n", score);
    printf("OUTCOME shoots=%d\n", score > 0 ? 1 : 0);
    /* The partial estimate scales each blow by a hit chance; it must never
     * exceed the full-damage figure for the same monster and grid. */
    printf("OUTCOME partial_danger=%d full_danger=%d partial_le_full=%d\n",
           partial, full, partial <= full ? 1 : 0);
    require(partial <= full);
    if (streq(mode, "wretch"))
        require(score > 0);
    else
        require(score <= 0); /* fails before: the sleeping troll is shot at and woken */
    ok;
}

const char *suite_name = "native-c/hitchance-63";
struct test tests[] = {
    {"ranged_attack_on_a_sleeping_monster", scenario_wake},
    {NULL, NULL}};
