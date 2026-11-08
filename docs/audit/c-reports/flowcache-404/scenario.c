/* Synthetic native C diagnostic for the Borg flow danger cache (issue 404),
 * excluded from the source fix.  One case per process (FC404_CASE).
 *
 * Measurement only: borg_danger() and borg_note() are wrapped at link time
 * (-Wl,--wrap) to count/record calls; the wrappers forward to the real
 * functions unchanged and verify that borg_danger() never touches the RNG. */
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
#include "z-rand.h"
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "z-virt.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-caution.h"
#include "borg/borg-danger.h"
#include "borg/borg-escape.h"
#include "borg/borg-flow-dark.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-flow.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-power.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* geometry constants, described below */
#define SX 13
#define SY 7
#define GX 14
#define MX 15
#define MY 8
#define TX 17
#define FRONTIER 17

/* ---- link-time measurement wrappers ------------------------------- */
extern int __real_borg_danger(int y, int x, int c, bool average, bool full_damage);
extern void __real_borg_note(const char *what);

#define DLOG_MAX 4096
static struct { int y, x, p; } dlog[DLOG_MAX];
static int dcalls;          /* borg_danger() calls since the last reset */
static int rng_touched;     /* borg_danger() calls that changed RNG state */

static uint32_t rng_hash(void)
{
    uint32_t h = state_i * 2654435761u ^ Rand_value;
    for (int i = 0; i < RAND_DEG; i++) h = (h ^ STATE[i]) * 16777619u;
    return h;
}

int __wrap_borg_danger(int y, int x, int c, bool average, bool full_damage)
{
    uint32_t before = rng_hash();
    int p = __real_borg_danger(y, x, c, average, full_damage);
    if (rng_hash() != before) rng_touched++;
    if (dcalls < DLOG_MAX) { dlog[dcalls].y = y; dlog[dcalls].x = x; dlog[dcalls].p = p; }
    dcalls++;
    return p;
}

#define NOTE_MAX 64
static char notes[NOTE_MAX][200];
static int nnotes;
void __wrap_borg_note(const char *what)
{
    if (nnotes < NOTE_MAX && (strstr(what, "Flowing") || strstr(what, "Desperate")
            || strstr(what, "monster") || strstr(what, "Expiring")))
    {
        strncpy(notes[nnotes], what, sizeof(notes[0]) - 1);
        notes[nnotes][sizeof(notes[0]) - 1] = 0;
        nnotes++;
    }
    __real_borg_note(what);
}

static int danger_calls_at(int x, int y)
{
    int n = 0;
    for (int i = 0; i < dcalls && i < DLOG_MAX; i++)
        if (dlog[i].x == x && dlog[i].y == y) n++;
    return n;
}

static void print_corridor_calls(const char *tag)
{
    if (getenv("FC404_ALLCALLS")) {
        printf("ALLCALLS %s:", tag);
        for (int i = 0; i < dcalls && i < DLOG_MAX; i++)
            printf(" (%d,%d)=%d", dlog[i].x, dlog[i].y, dlog[i].p);
        printf("\n");
    }
    printf("CALLS %s total=%d corridor:", tag, dcalls);
    int shown = 0;
    for (int i = 0; i < dcalls && i < DLOG_MAX; i++)
        if (dlog[i].y == SY && dlog[i].x >= 12 && dlog[i].x <= 18 && shown++ < 24)
            printf(" (%d,%d)=%d", dlog[i].x, dlog[i].y, dlog[i].p);
    printf("\n");
}

static void reset_counters(void) { dcalls = 0; nnotes = 0; }

/* Grids the previous (emergency) flow left marked as checked. */
static unsigned char snap[40][40];
static void snapshot_know(void)
{
    memset(snap, 0, sizeof(snap));
    for (int y = 0; y < 40; y++) for (int x = 0; x < 40; x++)
        snap[y][x] = borg_data_know->data[y][x] ? 1 : 0;
}
static int calls_in_snapshot(void)
{
    int n = 0;
    for (int i = 0; i < dcalls && i < DLOG_MAX; i++)
        if (snap[dlog[i].y][dlog[i].x]) n++;
    return n;
}

/* ---- geometry ------------------------------------------------------- *
 * Row y=7 is a one-wide corridor x=8..34.  The up staircase is at (13,7),
 * G=(14,7) is the first corridor grid east of it.  A grey mold sits in a
 * niche at (15,8): it is adjacent to (14,7), (15,7) and (16,7), and not to
 * (13,7) or (17,7).  Beyond x=FRONTIER the Borg's map is still unknown. */

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("FC404_PROFILE");
    if (!profile) return 1;
    printf("SETUPSTEP profile\n");
    setvbuf(stdout, NULL, _IONBF, 0);
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Priest", "FlowCache")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    screen.offset_x = screen.offset_y = 0;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 1;
    character_dungeon = true;
    for (int pass = 0; pass < 4; pass++) {
        for (struct object *obj = player->gear; obj; obj = obj->next) {
            if (obj->tval != TV_LIGHT) continue;
            if (player->body.slots[wield_slot(obj)].obj == obj) continue;
            inven_wield(obj, wield_slot(obj));
            break;
        }
    }
    player_place(cave, player, loc(GX, SY));
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    borg_init();
    borg_update();
    borg_alloc_detection();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory();
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s mhp=%d light=%d\n", player->class->name,
        player->mhp, player->state.cur_light);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

/* ---- world construction ---------------------------------------------- */
static void clear_modes(void)
{
    borg.status.desperate = false; borg.lunal_mode = false;
    borg.munchkin_mode = false; borg.status.digging = false;
}

/* wall_x >= 0 puts a granite block in the corridor at that column. */
static void build_world(bool mold, int wall_x, int chp)
{
    for (int y = 1; y < 39; y++) for (int x = 1; x < 39; x++)
        square_set_feat(cave, loc(x, y), FEAT_GRANITE);
    for (int x = 8; x <= 34; x++) square_set_feat(cave, loc(x, SY), FEAT_FLOOR);
    square_set_feat(cave, loc(MX, MY), FEAT_FLOOR);
    if (wall_x >= 0) square_set_feat(cave, loc(wall_x, SY), FEAT_GRANITE);
    square_set_feat(cave, loc(SX, SY), FEAT_LESS);
    player_place(cave, player, loc(GX, SY));
    player->chp = chp; player->csp = player->msp;
    borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    for (int y = 0; y < 40; y++) for (int x = 0; x < 40; x++) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
        if (x >= FRONTIER && y >= 6 && y <= 8) {
            /* three unexplored columns, then nothing the Borg has seen */
            borg_grids[y][x].feat = (x < FRONTIER + 3) ? FEAT_NONE : FEAT_GRANITE;
            borg_grids[y][x].info &= ~(BORG_OKAY | BORG_VIEW);
        }
    }
    update_view(cave, player);
    /* The staircase is tracked. */
    track_less.num = 1; track_less.x[0] = SX; track_less.y[0] = SY;
    track_more.num = 0;
    if (mold) {
        struct monster *m = t_add_monster(cave, loc(MX, MY), "grey mold");
        if (!m) { printf("SETUPFAIL mold\n"); exit(2); }
        update_mon(m, cave, true);
        mflag_on(m->mflag, MFLAG_VISIBLE);
        if (!observe_kill_diff(MY, MX, 0, 0)) { printf("SETUPFAIL observe\n"); exit(2); }
        borg_kills[borg_grids[MY][MX].kill].awake = 1;
    }
    clear_modes();
    borg_danger_wipe = false;
    borg_flow_clear();
    reset_counters();
}

static void move_borg(int x, int y)
{
    /* player_place() does not clear the grid the player leaves; without this
     * the game map marks both grids as the player's and borg_update() would
     * take the last one it scans. */
    square_set_mon(cave, player->grid, 0);
    player_place(cave, player, loc(x, y));
    borg.c = player->grid;
    update_view(cave, player);
}

/* ---- cache probes ------------------------------------------------------ */
static int know_at(int x, int y) { return borg_data_know->data[y][x]; }
static int icky_at(int x, int y) { return borg_data_icky->data[y][x]; }
static int cost_at(int x, int y) { return borg_data_cost->data[y][x]; }

static int fear_limit(void)
{
    /* The same bravery arithmetic the flow code applies on the normal path
     * (no scary monster, not level 50, not ignoring, fresh level). */
    return borg.avoidance * 3 / 10;
}

static void print_cache(const char *tag)
{
    printf("STATE %s_know_G=%d %s_icky_G=%d %s_know_15=%d %s_icky_15=%d %s_know_16=%d %s_icky_16=%d\n",
        tag, know_at(GX, SY), tag, icky_at(GX, SY),
        tag, know_at(15, SY), tag, icky_at(15, SY),
        tag, know_at(16, SY), tag, icky_at(16, SY));
}

static void set_mode(const char *mode)
{
    clear_modes();
    if (streq(mode, "desperate")) borg.status.desperate = true;
    else if (streq(mode, "lunal")) borg.lunal_mode = true;
    else if (streq(mode, "munchkin")) borg.munchkin_mode = true;
    else if (streq(mode, "digging")) borg.status.digging = true;
    else if (!streq(mode, "normal")) { printf("SETUPFAIL mode %s\n", mode); exit(2); }
}

/* Spread a flow from the staircase (the retreat shape). */
static void flow_from_stair(const char *mode)
{
    set_mode(mode);
    borg_flow_clear();
    borg_flow_enqueue_grid(SY, SX);
    borg_flow_spread(250, false, false, false, -1, false);
    clear_modes();
}

/* Spread a flow from the unexplored frontier toward the Borg (the
 * exploration shape). */
static void flow_from_frontier_depth(const char *mode, int depth)
{
    set_mode(mode);
    borg_flow_clear();
    borg_flow_enqueue_grid(SY, TX);
    borg_flow_spread(depth, false, false, false, -1, false);
    clear_modes();
}
static void flow_from_frontier(const char *mode) { flow_from_frontier_depth(mode, 250); }

static int route_to_stair(void) { return cost_at(SX, SY) < 250; }

static int pending_key(void)
{
    int k = 0, last = 0;
    while ((k = borg_inkey(true)) != 0) last = k;
    borg_flush();
    return last;
}

/* ---- cases -------------------------------------------------------------- */
/* cache-<mode>-<spread|enqueue>: an emergency mode flow, then a normal flow.
 * Contract: a normal flow must not enter a grid whose present danger is
 * above the bravery limit, whatever mode the cache entry was created in. */
static int case_cache(const char *mode, const char *entry, bool mold)
{
    build_world(mold, -1, 6);
    int p14 = __real_borg_danger(SY, GX, 1, true, false);
    int p15 = __real_borg_danger(SY, 15, 1, true, false);
    int p16 = __real_borg_danger(SY, 16, 1, true, false);
    int pT = __real_borg_danger(SY, TX, 1, true, false);
    printf("GEOMETRY stair=(%d,%d) G=(%d,%d) mold=(%d,%d) target=(%d,%d)\n", SX, SY, GX, SY, MX, MY, TX, SY);
    printf("RATING chp=%d limit=%d danger_G=%d danger_15=%d danger_16=%d danger_target=%d\n",
        player->chp, fear_limit(), p14, p15, p16, pT);

    /* Phase 1: the emergency flow (the previous retreat). */
    borg_danger_wipe = true; /* as observed in the full run: wipe on the emergency spread */
    reset_counters();
    if (streq(entry, "spread")) {
        flow_from_stair(mode);
    } else {
        set_mode(mode);
        borg_flow_clear();
        borg_flow_enqueue_grid(SY, GX);
        borg_flow_spread(250, false, false, false, -1, false);
        clear_modes();
    }
    int d1 = dcalls;
    print_cache("p1");
    printf("STATE p1_danger_calls=%d p1_cost_G=%d\n", d1, cost_at(GX, SY));

    /* Phase 2: a normal flow with no wipe in between. */
    snapshot_know();
    printf("STATE p1_wipe_pending=%d\n", borg_danger_wipe ? 1 : 0);
    reset_counters();
    int entered;
    if (streq(entry, "spread")) {
        flow_from_frontier("normal");
        entered = route_to_stair();
        printf("STATE p2_route_to_stair=%d p2_cost_G=%d p2_cost_15=%d\n", entered, cost_at(GX, SY), cost_at(15, SY));
    } else {
        set_mode("normal");
        borg_flow_clear();
        borg_flow_enqueue_grid(SY, GX);
        clear_modes();
        entered = (cost_at(GX, SY) == 0);
        printf("STATE p2_enqueued_G=%d\n", entered);
    }
    print_cache("p2");
    print_corridor_calls("phase2");
    printf("STATE p2_danger_calls=%d p2_danger_calls_G=%d p2_calls_on_p1_grids=%d rng_touched=%d\n", dcalls,
        danger_calls_at(GX, SY), calls_in_snapshot(), rng_touched);
    printf("OUTCOME entered=%d p1_icky_G=%d p2_icky_G=%d\n", entered, icky_at(GX, SY), 0);
    if (!mold) {
        /* Safe corridor: the route must exist whichever build runs. */
        require(p14 <= fear_limit());
        require(entered);
        ok;
    }
    require(p14 > fear_limit());     /* the setup really is dangerous */
    require(!entered);               /* contract: a dangerous grid is never entered by a normal flow */
    ok;
}

/* control-<kind>: flows whose result must not change with the correction. */
static int case_control(const char *kind)
{
    bool mold = !streq(kind, "safe") && !streq(kind, "blocked");
    int wall = streq(kind, "blocked") ? 16 : -1;
    build_world(mold, wall, 6);
    printf("GEOMETRY stair=(%d,%d) G=(%d,%d) mold=%s wall=%d\n", SX, SY, GX, SY, mold ? "yes" : "no", wall);
    int p14 = __real_borg_danger(SY, GX, 1, true, false);
    printf("RATING chp=%d limit=%d danger_G=%d\n", player->chp, fear_limit(), p14);

    if (streq(kind, "safe") || streq(kind, "dangerous") || streq(kind, "blocked")) {
        /* Ordinary normal flow, nothing emergency before it. */
        borg_danger_wipe = true;
        reset_counters();
        flow_from_frontier("normal");
        int route = route_to_stair();
        print_cache("n");
        printf("STATE route_to_stair=%d cost_G=%d danger_calls=%d danger_calls_G=%d rng_touched=%d\n",
            route, cost_at(GX, SY), dcalls, danger_calls_at(GX, SY), rng_touched);
        printf("OUTCOME route=%d icky_G=%d icky_16=%d\n", route, icky_at(GX, SY), icky_at(16, SY));
        if (streq(kind, "safe")) { require(route); require(!icky_at(GX, SY)); }
        else { require(!route); }
        /* the flow from the frontier meets (16,7) first: that is where it stops */
        if (streq(kind, "dangerous")) require(icky_at(16, SY));
        ok;
    }
    if (streq(kind, "retreat")) {
        /* Emergency retreat from the dangerous grid: it must still run
         * straight through the danger it is retreating from. */
        borg_danger_wipe = true;
        reset_counters();
        flow_from_stair("desperate");
        int reach = cost_at(GX, SY) < 250;
        printf("STATE retreat_cost_G=%d retreat_reaches_G=%d danger_calls=%d\n", cost_at(GX, SY), reach, dcalls);
        printf("OUTCOME reaches=%d\n", reach);
        require(reach);
        ok;
    }
    if (streq(kind, "reuse")) {
        /* Cache still used for entries that were genuinely evaluated. */
        borg_danger_wipe = true;
        flow_from_frontier("normal");
        reset_counters();
        borg_danger_wipe = false;
        flow_from_frontier("normal");
        printf("STATE second_normal_danger_calls=%d\n", dcalls);
        printf("OUTCOME second_calls=%d\n", dcalls);
        ok;
    }
    printf("SETUPFAIL control %s\n", kind);
    return 1;
}

/* cost-<mode>: a normal flow caches honest verdicts, an emergency flow runs,
 * then a normal flow runs again.  Counts the danger evaluations of the last
 * flow: the honest entries should not have to be recomputed. */
static int case_cost(const char *mode)
{
    build_world(false, -1, 6);
    borg_danger_wipe = true;
    flow_from_frontier("normal");           /* honest verdicts cached */
    int n1 = dcalls;
    reset_counters();
    flow_from_stair(mode);                  /* emergency flow over the same ground */
    int e = dcalls;
    reset_counters();
    flow_from_frontier("normal");
    int route = route_to_stair();
    printf("STATE first_normal_calls=%d emergency_calls=%d second_normal_calls=%d route=%d\n", n1, e, dcalls, route);
    printf("OUTCOME second_normal_calls=%d route=%d\n", dcalls, route);
    require(route);
    ok;
}

/* costmixed-<mode>: like cost-<mode>, but the honest normal flow is shallow
 * (depth 5, as the exploration flows are) and the emergency flow is deep, so
 * it crosses ground the normal flow never evaluated. */
static int case_costmixed(const char *mode)
{
    build_world(false, -1, 6);
    borg_danger_wipe = true;
    flow_from_frontier_depth("normal", 5);
    int n1 = dcalls;
    reset_counters();
    flow_from_stair(mode);
    int e = dcalls;
    reset_counters();
    flow_from_frontier_depth("normal", 5);
    printf("STATE first_normal_calls=%d emergency_calls=%d second_normal_calls=%d\n", n1, e, dcalls);
    printf("OUTCOME second_normal_calls=%d\n", dcalls);
    ok;
}

/* retreat blocked by an old normal-flow verdict: pre-existing behaviour,
 * identical in both builds. */
static int case_retreat_after_normal(void)
{
    build_world(true, -1, 6);
    borg_danger_wipe = true;
    flow_from_frontier("normal");               /* normal flow marks the corridor icky */
    int icky = icky_at(16, SY);
    flow_from_stair("desperate");               /* later retreat sees the old icky mark */
    int reach = cost_at(16, SY) < 250;
    printf("STATE normal_icky_16=%d later_retreat_reaches_16=%d later_retreat_reaches_G=%d\n", icky, reach, cost_at(GX, SY) < 250);
    printf("OUTCOME reaches_16=%d\n", reach);
    ok;
}

/* prod-<state>: the production path of the full run.  The Borg stands next
 * to the mold, retreats with the production desperate stair flow onto the
 * staircase, then production exploration (borg_flow_dark) runs with no
 * danger wipe in between.  state decides what the Borg knows about the mold
 * at the second step. */
static int case_prod(const char *state)
{
    build_world(true, -1, 6);
    int p14 = __real_borg_danger(SY, GX, 1, true, false);
    printf("GEOMETRY stair=(%d,%d) G=(%d,%d) mold=(%d,%d) frontier_x=%d\n", SX, SY, GX, SY, MX, MY, FRONTIER);
    printf("RATING chp=%d limit=%d danger_G=%d kills=%d\n", player->chp, fear_limit(), p14, borg_kills_cnt);

    /* Retreat step: production flow exactly as borg_caution() calls it. */
    borg_danger_wipe = true;
    reset_counters();
    borg.status.desperate = true;
    bool fled = borg_flow_stair_less(GOAL_FLEE, false);
    borg.status.desperate = false;
    int key1 = pending_key();
    printf("STATE retreat_ok=%d retreat_key=%d retreat_danger_calls=%d know_G=%d icky_G=%d\n",
        fled, key1, dcalls, know_at(GX, SY), icky_at(GX, SY));
    require(fled);

    /* The Borg is now on the staircase. */
    move_borg(SX, SY);
    borg.goal.type = 0;
    int kill_idx = borg_grids[MY][MX].kill;
    if (streq(state, "visible")) {
        borg.time.now += 1;
        borg_kills[kill_idx].when = borg.time.now;     /* seen again this turn */
        borg_kills[kill_idx].seen = true;
    } else if (streq(state, "remembered")) {
        borg.time.now += 60;                            /* out of sight for 60 turns */
    } else if (streq(state, "expired")) {
        borg.time.now += 2500;                          /* unseen beyond the expiry */
    } else {
        printf("SETUPFAIL state %s\n", state); return 1;
    }
    if (!streq(state, "visible")) {
        /* The mold is really out of the player's sight now: the game no
         * longer reports it, so the update cannot see it again. */
        struct monster *m = square_monster(cave, loc(MX, MY));
        mflag_off(m->mflag, MFLAG_VISIBLE);
        mflag_off(m->mflag, MFLAG_VIEW);
        /* The production per-turn update: it re-scans the map, and expires
         * or follows monsters that were not seen this turn.  Declare the
         * level unchanged so it is not treated as a new level (which would
         * wipe the danger cache for an unrelated reason). */
        borg.status.old_depth = borg.trait[BI_CDEPTH];
        reset_counters();
        borg_update();
        for (int i = 0; i < nnotes; i++) printf("UPDATENOTE %s\n", notes[i]);
        printf("STATE update_wiped_cache=%d\n", borg_danger_wipe ? 1 : 0);
    }
    int kill_alive = borg_grids[MY][MX].kill != 0 && borg_kills[borg_grids[MY][MX].kill].r_idx != 0;
    printf("STATE mold_remembered=%d wipe_pending=%d danger_G_now=%d\n", kill_alive,
        borg_danger_wipe ? 1 : 0, __real_borg_danger(SY, GX, 1, true, false));

    reset_counters();
    borg.goal.type = 0;
    bool moved = borg_flow_dark(true);
    int key2 = pending_key();
    printf("STATE explore_ok=%d explore_key=%d explore_danger_calls=%d explore_danger_calls_G=%d rng_touched=%d know_G=%d icky_G=%d\n",
        moved, key2, dcalls, danger_calls_at(GX, SY), rng_touched, know_at(GX, SY), icky_at(GX, SY));
    for (int i = 0; i < nnotes; i++) printf("NOTE %s\n", notes[i]);
    /* '6' is the keypress for a step east, into the mold's reach. */
    int stepped_east = (key2 == '6');
    printf("OUTCOME stepped_east=%d mold_remembered=%d\n", stepped_east, kill_alive);
    if (streq(state, "expired")) {
        /* Nothing is left to detect: both builds must agree, and the Borg
         * must explore east.  This also shows that the exploring flow works
         * in this geometry, so a blocked step in the other cases is the
         * danger check and not an unrelated failure of the flow. */
        require(moved);
        require(stepped_east);
        ok;
    }
    require(!stepped_east);
    ok;
}

/* caution: the retreat decision itself is made by the production
 * borg_caution() (not a hand-made call to the flow). */
static int case_caution(void)
{
    build_world(true, -1, 6);
    borg_danger_wipe = true;
    reset_counters();
    bool acted = borg_caution();
    int key1 = pending_key();
    printf("STATE caution_acted=%d caution_key=%d desperate_after=%d know_G=%d icky_G=%d\n", acted, key1,
        borg.status.desperate ? 1 : 0, know_at(GX, SY), icky_at(GX, SY));
    for (int i = 0; i < nnotes; i++) printf("NOTE %s\n", notes[i]);
    if (!acted) { printf("OUTCOME caution_acted=0\n"); return 1; }
    move_borg(SX, SY);
    borg.goal.type = 0;
    int idx = borg_grids[MY][MX].kill;
    borg.time.now += 1; borg_kills[idx].when = borg.time.now; borg_kills[idx].seen = true;
    reset_counters();
    bool moved = borg_flow_dark(true);
    int key2 = pending_key();
    printf("STATE explore_ok=%d explore_key=%d\n", moved, key2);
    for (int i = 0; i < nnotes; i++) printf("NOTE %s\n", notes[i]);
    printf("OUTCOME stepped_east=%d\n", key2 == '6');
    require(key1 == '4');
    require(key2 != '6');
    ok;
}

static int scenario(void *state)
{
    const char *c = getenv("FC404_CASE");
    if (!c) c = "cache-desperate-spread";
    printf("CASE %s\n", c);
    char buf[64]; strncpy(buf, c, sizeof(buf) - 1); buf[sizeof(buf) - 1] = 0;
    if (!strncmp(c, "cache-", 6) || !strncmp(c, "safe-", 5)) {
        bool mold = !strncmp(c, "cache-", 6);
        char *mode = buf + (mold ? 6 : 5), *dash = strchr(mode, '-');
        if (!dash) return 1;
        *dash = 0;
        return case_cache(mode, dash + 1, mold);
    }
    if (!strncmp(c, "costmixed-", 10)) return case_costmixed(c + 10);
    if (!strncmp(c, "cost-", 5)) return case_cost(c + 5);
    if (!strncmp(c, "control-", 8)) return case_control(c + 8);
    if (!strcmp(c, "retreat-after-normal")) return case_retreat_after_normal();
    if (!strcmp(c, "caution")) return case_caution();
    if (!strncmp(c, "prod-", 5)) return case_prod(c + 5);
    printf("SETUPFAIL case %s\n", c);
    return 1;
}
const char *suite_name = "native-c/flowcache-404";
struct test tests[] = {{"danger_cache_survives_emergency_mode", scenario}, {NULL, NULL}};
