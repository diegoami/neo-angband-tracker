/* Synthetic native C light-map fixture for issue #644 (lightmap-644).
 * Excluded from the source fix.  The borg's light map used the light's
 * strength (BI_LIGHT) as its radius; the engine lights one grid less
 * (calc_lighting() uses radius = ABS(light) - 1).  A monster standing in
 * the dark ring just outside the real light is "expected to be visible",
 * is not, and is forgotten.
 *
 * The level: a dark dead-end corridor, one grid wide, running east.
 * The borg walked the corridor and stands two grids from its end; a
 * soldier ant is in the dark, past the torch's real light.
 *
 * Modes:
 *  forget    the ant two grids east, awake, on the corridor's last floor:
 *            the game cannot see it (dark, no infravision).  With the
 *            borg's radius the grid is marked BORG_LIGHT, the record is
 *            "viewable", every floor neighbour of the dead end is lit,
 *            and borg_follow_kill() deletes the record as one that "did
 *            not really move".  With the engine radius the grid is dark,
 *            the record is not viewable, and it is kept.
 *  adjacent  control: the ant one grid east, inside the real light.  Both
 *            radii light the grid, the game sees the ant, the screen scan
 *            re-observes it, and the record is kept unchanged.
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
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-light.h"
#include "borg/borg-cave-util.h"
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

/* Dungeon map for the evidence replay (tools/c-evidence/evidence-map.c):
 * the level with the game's own symbols. */
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

/* The corridor: floor on row 12 from x=1 to x=12 (dead end), rock
 * elsewhere.  The borg walks in from the west. */
#define CORR_Y 12
#define CORR_X0 1
#define CORR_X1 12

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

int setup_tests(void **state)
{
    const char *profile = getenv("LM644_PROFILE");
    mode = getenv("LM644_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(644);
    if (!player_make_simple("Human", "Warrior", "LightMap644")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* A dark arena, sculpted into a dead-end corridor.  No grid is given
     * SQUARE_GLOW, so the torch is the only light. */
    cave = t_build_arena(H, W);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (y != CORR_Y || x < CORR_X0 || x > CORR_X1)
                square_set_feat(cave, loc(x, y), FEAT_GRANITE);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, CORR_Y));
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
     * the new-level reset (old_depth 0 vs depth 5), which initialises the
     * old-panel and old-location state every later update depends on. */
    borg_update();
    if (borg.status.old_depth != borg.trait[BI_CDEPTH]) { printf("SETUPFAIL arrival old_depth=%d depth=%d\n", borg.status.old_depth, borg.trait[BI_CDEPTH]); return 1; }
    /* The borg's known map: the corridor it walked while it was lit.
     * The light map itself is left entirely to the production code. */
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY;
    }
    track_less.num = track_more.num = 0;
    borg_notice(true);
    /* The soldier ant, awake, east of the player. */
    struct loc at = streq(mode, "adjacent") ? loc(11, CORR_Y) : loc(12, CORR_Y);
    struct monster *mon = t_add_monster(cave, at, "Soldier ant");
    if (!mon) { printf("SETUPFAIL monster\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE);
    update_mon(mon, cave, true);
    int visible = mflag_has(mon->mflag, MFLAG_VISIBLE) ? 1 : 0;
    /* Scenario validity: a Human has no infravision, the torch is the
     * only light, and only the adjacent ant is genuinely visible. */
    if (borg.trait[BI_INFRA] != 0) { printf("SETUPFAIL infra=%d\n", borg.trait[BI_INFRA]); return 1; }
    if (borg.trait[BI_LIGHT] != 2) { printf("SETUPFAIL light=%d\n", borg.trait[BI_LIGHT]); return 1; }
    if (visible != (streq(mode, "adjacent") ? 1 : 0)) { printf("SETUPFAIL visible=%d\n", visible); return 1; }
    /* Note the monster the way the borg would have while it was lit
     * (it stood one grid further west), then age the record one turn so
     * the missing-monster pass checks on it. */
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[k].awake = 1;
    borg.time.now++;
    borg_update_entrypoint(true);
    printf("SETUP seed=644 mode=%s class=%s clevel=%d hp=%d depth=%d torch=1 infra=%d light=%d monster=(%d,%d) visible=%d kill=%d\n",
           mode, player->class->name, player->lev, player->chp, player->depth,
           borg.trait[BI_INFRA], borg.trait[BI_LIGHT], at.x, at.y, visible, k);
    return 0;
}

static int scenario_light(void *state)
{
    struct loc at = streq(mode, "adjacent") ? loc(11, CORR_Y) : loc(12, CORR_Y);

    /* Force the production view and light recalculations. */
    borg_do_update_view = true;
    borg_do_update_lite = true;

    /* One production per-turn update: screen scan, view, light map and
     * the missing-monster pass that may delete kill records. */
    borg_update();

    /* The borg's light map at the monster's grid. */
    borg_grid *ag = &borg_grids[at.y][at.x];
    printf("LIGHTINFO grid=(%d,%d) light=%d glow=%d view=%d dark=%d\n", at.x, at.y,
           (ag->info & BORG_LIGHT) ? 1 : 0, (ag->info & BORG_GLOW) ? 1 : 0,
           (ag->info & BORG_VIEW) ? 1 : 0, (ag->info & BORG_DARK) ? 1 : 0);
    /* The borg's kill list after the pass. */
    int kills = 0;
    for (int k = 1; k < borg_kills_nxt; ++k) {
        borg_kill *kill = &borg_kills[k];
        if (!kill->r_idx) continue;
        kills++;
        printf("KILL k=%d race=%s pos=(%d,%d) awake=%d\n", k,
               r_info[kill->r_idx].name, kill->pos.x, kill->pos.y, kill->awake);
    }
    evidence_map("After the update pass: @ = the Borg, a = the soldier ant");
    printf("STATE kills=%d light=%d\n", kills, (ag->info & BORG_LIGHT) ? 1 : 0);
    int kept = kills > 0 ? 1 : 0;
    printf("OUTCOME kept=%d\n", kept);
    /* Corrected contract: a monster that walked out of the light stays
     * in the borg's memory, and the adjacent one is seen and kept. */
    require(kept == 1);
    ok;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/lightmap-644";
struct test tests[] = {
    {"light_map_record", scenario_light},
    {NULL, NULL}};
