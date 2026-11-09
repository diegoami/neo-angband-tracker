/* Synthetic native C spastic-search walk fixture, excluded from the source
 * fixes.  Stacked measurement for diegoami/neo-angband-tracker#147
 * (spasticscore-147) with #146 (spastic-146) applied, so that the search can
 * choose grids other than the Borg's own and the score decides where the
 * Borg walks. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "object.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "mon-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-misc.h"
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
/* A wide room, so that the corners (which have an extra wall diagonal)
 * are beyond the search's reach at character level 20 (cost 25). */
static int H = 22, W = 70;
/* Start grid: against the north wall in the middle of the room. */
#define SX 35
#define SY 2

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

int setup_tests(void **state)
{
    const char *profile = getenv("SP147S_PROFILE");
    const char *mode    = getenv("SP147S_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(147);
    if (!player_make_simple("Human", "Warrior", "Stacked147")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* A closed 22x70 room, no stairs, no monsters.  The Borg starts
     * against the granite north wall in the middle of the room (mode
     * "wall"), or three grids south of that, out in the room (control
     * "approach"). */
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    /* The test arena's outer wall is permanent rock; give the room an
     * ordinary granite north wall (row 1), as dungeon rooms have. */
    for (int x = 1; x < W - 1; ++x)
        square_set_feat(cave, loc(x, 1), FEAT_GRANITE);
    player_place(cave, player, loc(SX, streq(mode, "approach") ? SY + 3 : SY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *lantern = known_item(TV_LIGHT, "Lantern", 1);
    lantern->timeout = 2000;
    inven_wield(lantern, wield_slot(lantern));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    borg_alloc_detection();
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    /* Just arrived on the level. */
    borg.time.level = borg.time.now;
    borg.status.active = true;
    printf("SETUP seed=147 mode=%s class=%s clevel=%d depth=%d borg=(%d,%d)\n",
           mode, player->class->name, player->lev, player->depth, borg.c.x, borg.c.y);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int searches, repeats, moves, first_pick_x = -1, first_pick_y = -1,
    first_pick_count = -1;

/* One bored call of the production search, then the fixture carries out
 * the step it queued (if any), as the game would. */
static void spastic_call(int call)
{
    uint16_t before = messages_num();
    int x = borg.c.x, y = borg.c.y;
    int own0 = borg_grids[y][x].xtra;
    int tx = -1, ty = -1;
    /* Observation aid: blank the committed flow, so that after the call
     * the grid at flow cost 0 is the grid this call chose (none if it
     * committed nothing). */
    memset(borg_data_flow, 255, sizeof(borg_data));
    borg_flush();
    bool rc = borg_flow_spastic(true);
    keycode_t key = borg_inkey(false);
    int own1 = borg_grids[y][x].xtra;
    for (int yy = 0; yy < AUTO_MAX_Y; ++yy) for (int xx = 0; xx < AUTO_MAX_X; ++xx)
        if (borg_data_flow->data[yy][xx] == 0) { tx = xx; ty = yy; }
    printf("DECISION call=%d borg=(%d,%d) rc=%d key=%d own_count=%d->%d chose=(%d,%d) chose_count=%d\n",
           call, x, y, rc ? 1 : 0, (int)key, own0, own1, tx, ty,
           tx >= 0 ? borg_grids[ty][tx].xtra : -1);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %d %s\n", call, message_str((uint16_t)i));
    if (own1 > own0) {
        printf("SEARCH call=%d at=(%d,%d) count_before=%d\n", call, x, y, own0);
        ++searches;
        if (own0 > 0) ++repeats;
    } else if (searches && first_pick_x < 0 && tx >= 0 && (tx != x || ty != y)) {
        first_pick_x = tx; first_pick_y = ty;
        first_pick_count = borg_grids[ty][tx].xtra;
    }
    if (rc && key >= '1' && key <= '9' && key != '5') {
        int dir = key - '0';
        struct loc to = loc(x + ddx[dir], y + ddy[dir]);
        if (square_ispassable(cave, to)) {
            /* Fixture step: the queued move, carried out. */
            monster_swap(player->grid, to);
            borg.c = player->grid;
            ++moves;
            printf("STEP call=%d to=(%d,%d)\n", call, borg.c.x, borg.c.y);
        }
    }
}

static int scenario_spastic(void *state)
{
    const char *mode = getenv("SP147S_MODE");
    bool approach = streq(mode, "approach");

    /* Twelve bored calls in a row; the control stops at its first
     * search, before any search count could differ between grids. */
    for (int call = 1; call <= (approach ? 16 : 12); ++call) {
        spastic_call(call);
        if (approach && searches) break;
    }
    printf("OUTCOME searches=%d repeat_searches=%d moves=%d first_pick_x=%d first_pick_y=%d first_pick_count=%d end_x=%d end_y=%d\n",
           searches, repeats, moves, first_pick_x, first_pick_y,
           first_pick_count, borg.c.x, borg.c.y);
    if (approach) {
        /* walks to the wall and searches there once */
        require(moves > 0 && searches == 1);
    } else {
        /* after its first search the Borg walks to a grid nobody has
         * searched, and never searches a grid twice */
        require(moves > 0 && first_pick_count == 0 && repeats == 0);
    }
    ok;
}

const char *suite_name = "native-c/spasticscore-147-stacked";
struct test tests[] = {
    {"spastic_search_walks_to_an_unsearched_grid", scenario_spastic},
    {NULL, NULL}};
