/* Synthetic native C frame recorder for the flowcache-404 playable page,
 * excluded from the source fix.  Runs the same fixture world as scenario.c
 * and prints one FRAME json line per step (FC404_CASE selects the scene).
 *
 * Measurement only: borg_danger() and borg_note() are wrapped at link time
 * (-Wl,--wrap); the wrappers forward to the real functions unchanged. */
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

/* ---- frame recording ---------------------------------------------------- */
#define COL0 10
#define COL1 20
#define ROW0 6
#define ROW1 8

static int jstr_ok(const char *s) { return !strchr(s, '"') && !strchr(s, '\\'); }

static void grid_array(const char *name, int kind)
{
    printf("\"%s\":[", name);
    for (int y = ROW0; y <= ROW1; y++) for (int x = COL0; x <= COL1; x++) {
        int v = 0, f = borg_grids[y][x].feat;
        switch (kind) {
        case 0: v = know_at(x, y); break;
        case 1: v = icky_at(x, y); break;
        case 2: v = cost_at(x, y); break;
        case 3: v = (f == FEAT_FLOOR || f == FEAT_LESS)
                    ? __real_borg_danger(y, x, 1, true, false) : -1; break;
        default: /* terrain as the Borg believes it: 0 unknown, 1 floor, 2 up stair, 3 wall */
            v = (f == FEAT_NONE) ? 0 : (f == FEAT_FLOOR) ? 1 : (f == FEAT_LESS) ? 2 : 3; break;
        }
        printf("%s%d", (y == ROW0 && x == COL0) ? "" : ",", v);
    }
    printf("]");
}

/* One frame.  step ids and titles are decided by make_play.py. */
static void frame(const char *scene, const char *id, int step, int key, int okv, int flag, bool show_cost)
{
    printf("FRAME {\"scene\":\"%s\",\"case\":\"%s\",\"step\":%d,\"borg\":[%d,%d],\"limit\":%d,\"chp\":%d,",
        scene, id, step, borg.c.x, borg.c.y, fear_limit(), player->chp);
    printf("\"mold_known\":%d,\"key\":%d,\"ok\":%d,\"flag\":%d,\"wipe_pending\":%d,",
        borg_grids[MY][MX].kill != 0 ? 1 : 0, key, okv, flag, borg_danger_wipe ? 1 : 0);
    grid_array("terrain", 4); printf(",");
    grid_array("know", 0); printf(",");
    grid_array("icky", 1); printf(",");
    if (show_cost) { grid_array("cost", 2); printf(","); }
    grid_array("danger", 3); printf(",");
    printf("\"evaluated\":[");
    int shown = 0;
    for (int i = 0; i < dcalls && i < DLOG_MAX; i++) {
        if (dlog[i].x < COL0 || dlog[i].x > COL1 || dlog[i].y < ROW0 || dlog[i].y > ROW1) continue;
        printf("%s[%d,%d,%d]", shown++ ? "," : "", dlog[i].x, dlog[i].y, dlog[i].p);
    }
    printf("],\"danger_calls\":%d,\"notes\":[", dcalls);
    shown = 0;
    for (int i = 0; i < nnotes; i++)
        if (jstr_ok(notes[i])) printf("%s\"%s\"", shown++ ? "," : "", notes[i]);
    printf("]}\n");
}

/* walk-<state>: the Borg beside the mold retreats with the production
 * borg_caution(), stands on the staircase, then explores with the production
 * borg_flow_dark().  state: visible | remembered | expired. */
static int scene_walk(const char *state)
{
    build_world(true, -1, 6);
    frame("walk", state, 0, 0, 0, 0, false);

    borg_danger_wipe = true;
    reset_counters();
    bool acted = borg_caution();
    int key1 = pending_key();
    frame("walk", state, 1, key1, acted, 0, true);

    reset_counters();
    move_borg(SX, SY);
    borg.goal.type = 0;
    int idx = borg_grids[MY][MX].kill;
    if (!strcmp(state, "visible")) {
        borg.time.now += 1;
        borg_kills[idx].when = borg.time.now;
        borg_kills[idx].seen = true;
    } else {
        struct monster *m = square_monster(cave, loc(MX, MY));
        mflag_off(m->mflag, MFLAG_VISIBLE);
        mflag_off(m->mflag, MFLAG_VIEW);
        borg.time.now += !strcmp(state, "remembered") ? 60 : 2500;
        borg.status.old_depth = borg.trait[BI_CDEPTH];
        reset_counters();
        borg_update();
    }
    frame("walk", state, 2, 0, borg_grids[MY][MX].kill != 0, 0, false);

    reset_counters();
    borg.goal.type = 0;
    bool moved = borg_flow_dark(true);
    int key2 = pending_key();
    frame("walk", state, 3, key2, moved, 0, true);
    return 0;
}

/* modes-<mode>-<spread|enqueue>: an emergency-mode flow, then a normal flow. */
static int scene_modes(const char *mode, const char *entry)
{
    char id[64];
    snprintf(id, sizeof(id), "%s-%s", mode, entry);
    build_world(true, -1, 6);
    frame("modes", id, 0, 0, 0, 0, false);

    borg_danger_wipe = true;
    reset_counters();
    if (!strcmp(entry, "spread")) {
        flow_from_stair(mode);
    } else {
        set_mode(mode);
        borg_flow_clear();
        borg_flow_enqueue_grid(SY, GX);
        borg_flow_spread(250, false, false, false, -1, false);
        clear_modes();
    }
    frame("modes", id, 1, 0, 0, 0, true);

    reset_counters();
    int entered;
    if (!strcmp(entry, "spread")) {
        flow_from_frontier("normal");
        entered = route_to_stair();
    } else {
        set_mode("normal");
        borg_flow_clear();
        borg_flow_enqueue_grid(SY, GX);
        clear_modes();
        entered = (cost_at(GX, SY) == 0);
    }
    frame("modes", id, 2, 0, entered, 0, true);
    return 0;
}

static int scenario(void *state)
{
    const char *c = getenv("FC404_CASE");
    if (!c) c = "walk-visible";
    char buf[64]; strncpy(buf, c, sizeof(buf) - 1); buf[sizeof(buf) - 1] = 0;
    if (!strncmp(c, "walk-", 5)) { scene_walk(buf + 5); ok; }
    if (!strncmp(c, "modes-", 6)) {
        char *mode = buf + 6, *dash = strchr(mode, '-');
        if (!dash) return 1;
        *dash = 0;
        scene_modes(mode, dash + 1);
        ok;
    }
    printf("SETUPFAIL case %s\n", c);
    return 1;
}
const char *suite_name = "native-c/flowcache-404-play";
struct test tests[] = {{"record_frames", scenario}, {NULL, NULL}};
