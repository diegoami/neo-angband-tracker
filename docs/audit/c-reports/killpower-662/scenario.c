/* Synthetic native C kill-power fixture for issue #662 (killpower-662).
 * Excluded from the source fix.  Both monster-tracking paths estimated a
 * monster's hit points by routing the known (cheated) true hp through a
 * rounded percentage and back: pct = 100 * hp / maxhp, then power =
 * maxhp * pct / 100.  The double rounding loses small remainders twice,
 * so a floating eye at 1 of 11 hp was scored as 9% and then 0 hp - no
 * health worth attacking - while the game keeps a monster alive at
 * exactly 0 hp (its death test is hp < 0).
 *
 * The level: an open lit arena floor, no other monsters.  A Human
 * Warrior stands still; a floating eye (stock race, maxhp 11) is two
 * grids east, awake and visible.
 *
 * Modes set the eye's true hp and run two production borg_update()
 * passes: the first observation creates the kill record (the
 * new-record path), the second refreshes it (the existing-record path).
 *  full        control, eye at full health (byte-identical before/after)
 *  one_of_hp   the reported 1-hp boundary: power must be 1, was 0
 *  above_edge  2 hp, just above the rounding edge: power must be 2, was 1
 *  zero_alive  exactly 0 hp while alive: power 0 in both variants, the
 *              faithful reading of the engine's hp < 0 death test
 */
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
#include "borg/borg-cave.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-projection.h"
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

static void evidence_map(const char *caption)
{
    printf("MAP %s\n", caption);
    for (int y = 0; y < cave->height; ++y) {
        char row[256];
        int x;
        for (x = 0; x < cave->width && x < 255; ++x) {
            struct loc grid = loc(x, y);
            wchar_t c = f_info[square(cave, grid)->feat].d_char;
            struct monster *mon = square_monster(cave, grid);
            if (mon)
                c = mon->race->d_char;
            if (loc_eq(grid, player->grid))
                c = L'@';
            row[x] = (c > 31 && c < 127) ? (char)c : '?';
        }
        row[x] = 0;
        printf("MAPROW %s\n", row);
    }
    printf("MAPEND\n");
}

static term screen;
#define H 24
#define W 40
#define EYE_X 21
#define EYE_Y 12

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

static const char *mode;
static int mode_hp(void)
{
    if (streq(mode, "full")) return -1;      /* full health */
    if (streq(mode, "one_of_hp")) return 1;
    if (streq(mode, "above_edge")) return 2;
    if (streq(mode, "zero_alive")) return 0;
    return -2;                               /* unknown mode */
}

int setup_tests(void **state)
{
    const char *profile = getenv("KP662_PROFILE");
    mode = getenv("KP662_MODE");
    if (!profile || !mode || mode_hp() == -2) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(662);
    if (!player_make_simple("Human", "Warrior", "KillPower662")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* An open lit arena floor. */
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(20, EYE_Y));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 4000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    player->chp = player->mhp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg.c = player->grid;
    borg.status.active = true;
    borg.ready_morgoth = 0;
    borg.time.level = borg.time.town = borg.time.now;
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    borg.avoidance = player->chp;
    /* The borg arrives on the level: the first production update takes
     * the new-level reset, which later state depends on. */
    borg_update();
    if (borg.status.old_depth != borg.trait[BI_CDEPTH]) { printf("SETUPFAIL arrival old_depth=%d depth=%d\n", borg.status.old_depth, borg.trait[BI_CDEPTH]); return 1; }
    /* The borg's known map: the lit arena floor. */
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY;
    }
    track_less.num = track_more.num = 0;
    borg_notice(true);
    printf("SETUP seed=662 mode=%s class=%s clevel=%d hp=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp, player->depth);
    return 0;
}

static int scenario_kill_power(void *state)
{
    struct monster *mon = t_add_monster(cave, loc(EYE_X, EYE_Y), "floating eye");
    if (!mon) { printf("SETUPFAIL monster\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE);
    update_mon(mon, cave, true);
    if (!mflag_has(mon->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    if (mon->maxhp < 3) { printf("SETUPFAIL maxhp=%d\n", mon->maxhp); return 1; }

    /* Set the eye's true hp before the first observation. */
    int want = (mode_hp() < 0) ? mon->maxhp : mode_hp();
    mon->hp = want;

    /* Pass 1: the production screen scan creates the kill record
     * (borg_update_kill_new). */
    borg_update();
    /* Pass 2: the second scan refreshes the existing record
     * (borg_update_kill_old). */
    borg_update();

    int k = borg_grids[EYE_Y][EYE_X].kill;
    if (k <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kill *kill = &borg_kills[k];
    if (kill->r_idx != mon->race->ridx) { printf("SETUPFAIL race kill=%s\n", r_info[kill->r_idx].name); return 1; }

    printf("KILLINFO k=%d race=%s maxhp=%d game_hp=%d borg_power=%d injury=%d cap=%d\n",
           k, r_info[kill->r_idx].name, mon->maxhp, mon->hp,
           kill->power, kill->injury, kill->power * 2);
    evidence_map("After two update passes: @ = the Borg, o = the floating eye");
    printf("STATE power_ok=%d borg_power=%d game_hp=%d\n",
           (kill->power == mon->hp) ? 1 : 0, kill->power, mon->hp);
    int power_ok = (kill->power == mon->hp) ? 1 : 0;
    printf("OUTCOME power_ok=%d borg_power=%d\n", power_ok, kill->power);
    /* Corrected contract: the tracked power equals the monster's true
     * hp on both tracking paths. */
    require(power_ok == 1);
    ok;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/killpower-662";
struct test tests[] = {
    {"kill_power_matches_hp", scenario_kill_power},
    {NULL, NULL}};
