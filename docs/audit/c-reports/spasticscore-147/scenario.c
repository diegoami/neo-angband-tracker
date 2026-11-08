/* Synthetic native C spastic-search scoring fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#147 (spasticscore-147). */
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
static int H = 22, W = 22;
/* The grid under test: against the north wall, away from the side walls. */
#define PX 10
#define PY 2

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
    const char *profile = getenv("SP147_PROFILE");
    const char *mode    = getenv("SP147_MODE");
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
    if (!player_make_simple("Human", "Warrior", "Spastic147")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* A closed 22x22 room, no stairs, no monsters.  The Borg starts
     * against the granite north wall: two grids west of the grid under test P
     * (mode "fresh"), one grid east of it (mode "searched"), or on P itself
     * (control "plain"). */
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    /* The test arena's outer wall is permanent rock; give the room an
     * ordinary granite north wall (row 1), as dungeon rooms have. */
    for (int x = 1; x < W - 1; ++x)
        square_set_feat(cave, loc(x, 1), FEAT_GRANITE);
    {
        int sx = PX;
        if (streq(mode, "fresh")) sx = PX - 2;
        else if (streq(mode, "searched")) sx = PX + 1;
        player_place(cave, player, loc(sx, PY));
    }
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

/* One bored call of the production search.  Reports whether the call
 * recorded a search (the Borg's grid search count went up). */
static bool spastic_call(const char *tag)
{
    uint16_t before = messages_num();
    int x = borg.c.x, y = borg.c.y;
    int own0 = borg_grids[y][x].xtra;
    int nw0 = borg_grids[y - 1][x - 1].xtra;
    borg_flush();
    bool rc = borg_flow_spastic(true);
    keycode_t key = borg_inkey(false);
    int own1 = borg_grids[y][x].xtra;
    printf("DECISION call=%s borg=(%d,%d) rc=%d key=%d own_count=%d->%d nw_count=%d searched=%d\n",
           tag, x, y, rc ? 1 : 0, (int)key, own0, own1, nw0, own1 > own0 ? 1 : 0);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s %s\n", tag, message_str((uint16_t)i));
    return own1 > own0;
}

static int scenario_spastic(void *state)
{
    const char *mode = getenv("SP147_MODE");
    int first = 0, at_p = 0;
    char tag[24];

    /* Phase 1 (not in the control): the bored Borg searches where it
     * stands until that grid is used up.  Each search raises the count of
     * all nine grids around it by 5. */
    if (!streq(mode, "plain")) {
        for (int call = 0; call < 8; ++call) {
            strnfmt(tag, sizeof(tag), "start%d", call + 1);
            if (spastic_call(tag)) ++first;
        }
        printf("STATE start_searches=%d\n", first);
        /* Fixture step: the Borg moves to P (two steps east, or one step
         * west), as it would while exploring; the walk is not under test. */
        monster_swap(player->grid, loc(PX, PY));
        borg.c = player->grid;
    }
    printf("STATE at_P=(%d,%d) P_count=%d P_northwest_count=%d\n", borg.c.x,
           borg.c.y, borg_grids[PY][PX].xtra, borg_grids[PY - 1][PX - 1].xtra);

    /* Phase 2: the same bored search from P. */
    for (int call = 0; call < 8; ++call) {
        strnfmt(tag, sizeof(tag), "P%d", call + 1);
        if (spastic_call(tag)) ++at_p;
    }
    printf("OUTCOME searches_at_P=%d P_count=%d\n", at_p, borg_grids[PY][PX].xtra);
    if (streq(mode, "fresh")) {
        /* P was never searched itself: searched three times, like the
         * control */
        require(at_p == 3);
    } else if (streq(mode, "searched")) {
        /* P already carries three searches (count 15): no more */
        require(at_p == 0);
    } else {
        require(at_p == 3);
    }
    ok;
}

const char *suite_name = "native-c/spasticscore-147";
struct test tests[] = {
    {"spastic_search_scores_the_candidate_grid", scenario_spastic},
    {NULL, NULL}};
