/* Synthetic native C spastic-search reverse-flow fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#146 (spastic-146). */
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
static int H = 24, W = 24;

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
    const char *profile = getenv("SP146_PROFILE");
    const char *mode    = getenv("SP146_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(146);
    if (!player_make_simple("Human", "Warrior", "Spastic146")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* A closed room (granite all round), no stairs, no monsters: 22x22 for
     * the main cases; a full-size 196x64 room for the control, where every
     * wall is farther than the search range of a level-20 Borg (25). */
    if (streq(mode, "far")) { H = 66; W = 198; }
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    if (streq(mode, "corner"))
        player_place(cave, player, loc(1, 1));       /* in a corner */
    else
        player_place(cave, player, loc(W / 2, H / 2)); /* mid-room */
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
    printf("SETUP seed=146 mode=%s class=%s clevel=%d depth=%d borg=(%d,%d)\n",
           mode, player->class->name, player->lev, player->depth, borg.c.x, borg.c.y);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static bool spastic_call(const char *tag)
{
    uint16_t before = messages_num();
    borg_flush();
    bool rc = borg_flow_spastic(true);
    keycode_t key = borg_inkey(false);
    printf("DECISION call=%s rc=%d key=%d borg=(%d,%d)\n", tag, rc ? 1 : 0,
           (int)key, borg.c.x, borg.c.y);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s %s\n", tag, message_str((uint16_t)i));
    return rc;
}

static int scenario_spastic(void *state)
{
    const char *mode = getenv("SP146_MODE");
    int moves = 0, searched = 0;

    /* A bored Borg looking for secret doors calls the search three times
     * in a row from the same grid. */
    for (int call = 0; call < 3; ++call) {
        char tag[16];
        strnfmt(tag, sizeof(tag), "call%d", call + 1);
        if (spastic_call(tag)) ++moves;
    }
    for (int i = 0; i < 9; ++i)
        if (borg_grids[borg.c.y + ddy_ddd[i]][borg.c.x + ddx_ddd[i]].xtra) ++searched;
    printf("STATE own_grid_searched=%d\n", searched ? 1 : 0);
    printf("OUTCOME moved=%d\n", moves ? 1 : 0);
    if (streq(mode, "far")) {
        require(moves == 0); /* no wall within range: nothing to search */
    } else {
        /* fails before: the search never takes a step */
        require(moves > 0);
    }
    ok;
}

const char *suite_name = "native-c/spastic-146";
struct test tests[] = {
    {"spastic_search_walks_to_a_wall", scenario_spastic},
    {NULL, NULL}};
