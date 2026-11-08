/* Synthetic native C weak-from-hunger fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#80 (shopline-125). */
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
#include "obj-tval.h"
#include "obj-util.h"
#include "player.h"
#include "store.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-think.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-misc.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
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
    object_prep(obj, kind, 10, MAXIMISE);
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
    const char *profile = getenv("SL125_PROFILE");
    const char *mode    = getenv("SL125_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(125);
    if (!player_make_simple("Human", "Warrior", "ShopLine125")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    player_exp_gain(player, player_exp[10] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp;
    player->csp = player->msp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory();
    borg_do_spell = true;
    borg_notice(true);
    /* The map update has not run: size the detection arrays for a two by
     * two panel map, as it would. */
    borg.panels.x = 2; borg.panels.y = 2;
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=125 mode=%s class=%s clevel=%d hp=%d/%d sp=%d/%d food=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp,
           player->csp, player->msp, player->timed[TMD_FOOD], player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int store_index(int feat)
{
    for (int i = 0; i < z_info->store_max; i++)
        if (stores[i].feat == feat) return i;
    return -1;
}

static void put_shop(int idx, int x, int y)
{
    square_set_feat(cave, loc(x, y), stores[idx].feat);
    borg_grids[y][x].feat  = stores[idx].feat;
    borg_grids[y][x].store = idx;
    track_shop_x[idx] = x;
    track_shop_y[idx] = y;
}

static int scenario_shop(void *state)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *mode = getenv("SL125_MODE");
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);

    /* The General Store is the goal, 20 grids east of the Borg in its row.
     * Another shop entrance, the Armoury, is in the way: next to the Borg
     * in the same row ('inline'), or two rows above the line ('offrow',
     * control). */
    int goal  = store_index(FEAT_STORE_GENERAL);
    int other = store_index(FEAT_STORE_ARMOR);
    if (goal < 0 || other < 0) { printf("SETUPFAIL stores\n"); return 1; }
    put_shop(goal, 30, 12);
    if (streq(mode, "inline"))
        put_shop(other, 11, 12);
    else
        put_shop(other, 11, 10);
    borg.goal.shop = goal;
    borg.goal.ware = borg.goal.item = -1;
    printf("STATE borg=(%d,%d) goal_shop=%d at (30,12) other_shop=%d at (%d,%d) depth=%d\n", borg.c.x, borg.c.y,
           goal, other, 11, streq(mode, "inline") ? 12 : 10, borg.trait[BI_CDEPTH]);

    bool rc = borg_flow_shop_entry(goal);
    keycode_t key = borg_inkey(false);
    printf("DECISION rc=%d key=%d\n", rc ? 1 : 0, (int)key);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    /* The key is a direction: '6' is east, onto the other shop's door when
     * it is in the way. */
    printf("OUTCOME steps_onto_other_shop=%d\n", (rc && key == '6' && streq(mode, "inline")) ? 1 : 0);
    require(rc);
    if (streq(mode, "inline"))
        require(key != '6'); /* fails before: the straight line runs through the other door */
    ok;
}

const char *suite_name = "native-c/shopline-125";
struct test tests[] = {
    {"flow_avoids_other_shop_doors", scenario_shop},
    {NULL, NULL}};
