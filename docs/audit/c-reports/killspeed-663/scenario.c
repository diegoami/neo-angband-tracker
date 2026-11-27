/* Synthetic native C monster-speed fixture for issue #663 (killspeed-663).
 * Excluded from the source fix.  The C tracker copies the monster's base
 * mspeed into kill->speed on both tracking paths; the engine, before
 * giving a monster energy, adds 10 for a hasted monster and subtracts
 * twice the Slow effect level from a slowed one.  The borg's danger
 * calculation and the retreat comparisons (kill->speed > BI_SPEED) thus
 * use the speed the monster had before Haste and Slow, so a visibly
 * hasted monster outruns the borg sooner than the danger and retreat
 * math predicts.
 *
 * The level: an open, lit arena, no other monsters, a Human Warrior.
 * A floating eye (stock race) stands three grids east, awake and
 * visible; the production tracking updates observe it like any other
 * turn.  The expected net speed is computed from the live monster with
 * the engine's own formula, so the assertion does not hardcode numbers:
 *
 *  hasted_new      the eye is hasted when first seen (new-record path)
 *  hasted_refresh  the eye is seen plain, then hasted (existing-record
 *                  refresh path)
 *  slowed          the eye is slowed (one effect level)
 *  pers              the hasted eye is seen once, then goes out of
 *                  sight: the last observed net speed must persist
 *  plain           control: no timed effect (byte-identical)
 *  expired         control: the haste expires before the eye is seen
 *                  again (byte-identical)
 */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
#include "mon-timed.h"
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
#include "ui-input.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-think.h"
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
#define EYE_X 23
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
static int mode_valid(void)
{
    return streq(mode, "hasted_new") || streq(mode, "hasted_refresh")
        || streq(mode, "slowed") || streq(mode, "persists_offscreen")
        || streq(mode, "plain") || streq(mode, "expired");
}

/* The engine's net speed: +10 hasted, -2 x Slow effect level slowed. */
static int expected_net_speed(struct monster *mon)
{
    int net = mon->mspeed;
    if (mon->m_timed[MON_TMD_FAST]) net += 10;
    if (mon->m_timed[MON_TMD_SLOW])
        net -= 2 * monster_effect_level(mon, MON_TMD_SLOW);
    return (net > 0) ? net : 0;
}

static struct monster *place_eye(bool hasted, bool slowed)
{
    struct monster *mon = t_add_monster(cave, loc(EYE_X, EYE_Y), "floating eye");
    if (!mon) { printf("SETUPFAIL monster\n"); return NULL; }
    mflag_on(mon->mflag, MFLAG_AWARE);
    if (hasted)
        mon_inc_timed(mon, MON_TMD_FAST, 30,
            MON_TMD_FLG_NOMESSAGE | MON_TMD_FLG_NOFAIL);
    if (slowed)
        mon_inc_timed(mon, MON_TMD_SLOW, 20,
            MON_TMD_FLG_NOMESSAGE | MON_TMD_FLG_NOFAIL);
    update_mon(mon, cave, true);
    if (!mflag_has(mon->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return NULL; }
    return mon;
}

int setup_tests(void **state)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *profile = getenv("MP663_PROFILE");
    mode = getenv("MP663_MODE");
    if (!profile || !mode || !mode_valid()) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(663);
    if (!player_make_simple("Human", "Warrior", "KillSpeed663")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* An open, lit arena floor; sight is line-of-sight only. */
    cave = t_build_arena(H, W);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(20, EYE_Y));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 4000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_MANA | PU_TORCH | PU_INVEN | PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    player->chp = player->mhp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg.c = player->grid;
    borg.status.active = true;
    borg.ready_morgoth = 0;
    borg.time.level = borg.time.town = borg.time.now;
    borg_do_spell = true;
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    borg.avoidance = player->chp;
    /* The borg arrives on the level: the first production update takes
     * the new-level reset, which later state depends on. */
    borg_update();
    if (borg.status.old_depth != borg.trait[BI_CDEPTH]) { printf("SETUPFAIL arrival old_depth=%d depth=%d\n", borg.status.old_depth, borg.trait[BI_CDEPTH]); return 1; }
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY;
    }
    track_less.num = track_more.num = 0;
    borg_notice(true);
    printf("SETUP seed=663 mode=%s class=%s clevel=%d hp=%d p_speed=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp,
           borg.trait[BI_SPEED], player->depth);
    return 0;
}

static int scenario_kill_speed(void *state)
{
    struct monster *mon;
    int k, seen_passes = 1;

    /* The first observation: the eye as the mode stages it. */
    bool start_hasted = streq(mode, "hasted_new")
        || streq(mode, "persists_offscreen");
    mon = place_eye(start_hasted, streq(mode, "slowed"));
    if (!mon) return 1;

    /* Pass 1: the production screen scan creates the kill record. */
    borg_update();

    /* The existing-record refresh path: seen plain first, then hasted. */
    if (streq(mode, "hasted_refresh")) {
        k = borg_grids[EYE_Y][EYE_X].kill;
        if (k <= 0) { printf("SETUPFAIL first_pass\n"); return 1; }
        mon_inc_timed(mon, MON_TMD_FAST, 30,
            MON_TMD_FLG_NOMESSAGE | MON_TMD_FLG_NOFAIL);
        borg_update();
        seen_passes = 2;
    }

    /* Off-screen: the eye leaves sight; the last observation must
     * persist (the borg cannot re-read a monster it cannot see). */
    if (streq(mode, "persists_offscreen")) {
        mflag_off(mon->mflag, MFLAG_VISIBLE);
        seen_passes = 2;
        borg_update();
    }

    k = borg_grids[EYE_Y][EYE_X].kill;
    if (k <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kill *kill = &borg_kills[k];
    if (kill->r_idx != mon->race->ridx) { printf("SETUPFAIL race kill=%s\n", r_info[kill->r_idx].name); return 1; }

    /* The engine's net speed for the live monster, and the tracked
     * speed that feeds the danger and retreat comparisons. */
    int net = expected_net_speed(mon);
    printf("SPEED race=%s base=%d fast=%d slow=%d net=%d tracked=%d passes=%d\n",
           r_info[kill->r_idx].name, mon->mspeed,
           mon->m_timed[MON_TMD_FAST] ? 1 : 0,
           mon->m_timed[MON_TMD_SLOW] ? 1 : 0,
           net, kill->speed, seen_passes);
    printf("STATE tracked=%d net=%d player=%d faster_than_me=%d\n",
           kill->speed, net, borg.trait[BI_SPEED],
           kill->speed > borg.trait[BI_SPEED] ? 1 : 0);
    /* The production danger of the eye's own grid: the energy input is
     * the tracked speed, so the danger moves with the timed state. */
    int danger = borg_danger(EYE_Y, EYE_X, 1, true, false);
    printf("DANGER at_eye=%d tracked_speed=%d\n", danger, kill->speed);
    printf("OUTCOME tracked=%d net=%d\n", kill->speed, net);
    evidence_map("After the tracking passes: @ = the Borg, e = the floating eye");

    /* Corrected contract: the tracked speed is the observed net speed
     * (haste +10, Slow -2 x level), also when the effect started after
     * the first sighting, and also off-screen (last observation). */
    require(kill->speed == net);
    ok;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/killspeed-663";
struct test tests[] = {
    {"tracked_speed_matches_observed_timed_state", scenario_kill_speed},
    {NULL, NULL}};
