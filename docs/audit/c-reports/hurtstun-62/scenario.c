/* Synthetic native C melee danger fixture for HURT blows, excluded from the
 * source change.  Issue: diegoami/neo-angband-tracker#62 (hurtstun-62). */
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
#define PX 10
#define PY 12

static const char *race_name = NULL;

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 30, MAXIMISE);
    obj->number = number;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

int setup_tests(void **state)
{
    const char *profile = getenv("HS62_PROFILE");
    const char *mode    = getenv("HS62_MODE");
    if (!profile || !mode) return 1;
    if (streq(mode, "mystic")) race_name = "mystic";            /* 4 x KICK:HURT:10d2 */
    else if (streq(mode, "ant")) race_name = "soldier ant";     /* BITE:HURT:1d2 */
    else if (streq(mode, "orc")) race_name = "cave orc";        /* HIT:HURT:1d8 */
    else if (streq(mode, "giant")) race_name = "hill giant";    /* 2 x HIT:HURT:4d8 */
    else { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(62);
    if (!player_make_simple("Human", "Warrior", "HurtStun62")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    player_exp_gain(player, player_exp[29] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    /* Hasted (+10 speed), so the mystic's kicks are rated below the 2000
     * danger cap and the kept +400 test shows in the result. */
    player->timed[TMD_FAST] = 100;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    borg.c = player->grid;
    borg.time.level = borg.time.now;
    borg_notice(true);
    borg.avoidance = borg.trait[BI_CURHP];
    /* One awake monster right next to the Borg (east). */
    struct monster *mon = t_add_monster(cave, loc(PX + 1, PY), race_name);
    if (!mon) { printf("SETUPFAIL mon\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill\n"); return 1; }
    borg_kills[k].awake = true;
    printf("SETUP seed=62 mode=%s class=%s clevel=%d depth=%d hp=%d ac=%d speed=%d monster=%s\n", mode,
           player->class->name, player->lev, player->depth, borg.trait[BI_CURHP],
           borg.trait[BI_ARMOR], borg.trait[BI_SPEED], race_name);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_danger(void *state)
{
    int k = borg_grids[PY][PX + 1].kill;
    struct monster_race *r_ptr = &r_info[borg_kills[k].r_idx];
    int blows = 0, low_sides = 0, live_kick = 0;
    for (int b = 0; b < z_info->mon_blows_max && r_ptr->blow[b].method; ++b) {
        int d_dice = r_ptr->blow[b].dice.dice, d_side = r_ptr->blow[b].dice.sides;
        bool hurt = streq(r_ptr->blow[b].effect->name, "HURT");
        printf("BLOW %d method=%s effect=%s dice=%dd%d\n", b, r_ptr->blow[b].method->name,
               r_ptr->blow[b].effect->name, d_dice, d_side);
        blows++;
        /* HURT blows with d_side < 3 reach the removed test's second half;
         * of those, d_dice > 5 also take the kept +400. */
        if (hurt && d_side < 3) low_sides++;
        if (hurt && d_side < 3 && d_dice > 5) live_kick++;
    }
    int part = borg_danger_one_kill(borg.c.y, borg.c.x, 1, k, true, false);
    int full = borg_danger_one_kill(borg.c.y, borg.c.x, 1, k, true, true);
    int grid = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    int grid_full = borg_danger(borg.c.y, borg.c.x, 1, true, true);
    printf("STATE blows=%d low_sides=%d live_kick=%d avoidance=%d\n", blows, low_sides, live_kick,
           borg.avoidance);
    printf("DECISION one_kill=%d one_kill_full=%d grid=%d grid_full=%d\n", part, full, grid,
           grid_full);
    printf("OUTCOME danger=%d full=%d\n", grid, grid_full);
    ok;
}

const char *suite_name = "native-c/hurtstun-62";
struct test tests[] = {
    {"hurt_blow_danger", scenario_danger},
    {NULL, NULL}};
