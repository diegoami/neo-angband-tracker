/* Synthetic native C town-shopping fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#60 (emptyslot-60). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "object.h"
#include "obj-desc.h"
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
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include "borg/borg-store.h"
#include "borg/borg-store-buy.h"
#include "borg/borg-think-store.h"
#include "borg/borg-cave.h"
#include "borg/borg-power.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

static void make_known(struct object *obj)
{
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    player_know_object(player, obj);
}

static struct object *new_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 0, MINIMISE);
    obj->number = number;
    return obj;
}

static int pack_count(void)
{
    int n = 0;
    for (struct object *o = player->gear; o; o = o->next)
        if (!object_is_equipped(player->body, o)) n++;
    return n;
}

int setup_tests(void **state)
{
    const char *profile = getenv("ES60_PROFILE");
    const char *mode    = getenv("ES60_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(60);
    /* Birth option "Start with a kit of useful gear" off: the game gives
     * one ration (pack) and one torch (wielded), nothing else. */
    player->opts.opt[OPT_birth_start_kit] = false;
    if (!player_make_simple("Human", "Warrior", "EmptySlot60")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    character_dungeon = true;
    printf("BIRTH start_kit=%d pack_items=%d gold=%d\n",
           OPT(player, birth_start_kit) ? 1 : 0, pack_count(), (int)player->au);
    for (struct object *o = player->gear; o; o = o->next) {
        char buf[80];
        object_desc(buf, sizeof(buf), o, ODESC_FULL, player);
        printf("GEAR %s [%s]\n", object_is_equipped(player->body, o) ? "worn" : "pack", buf);
    }

    /* 'empty': the starting ration is eaten with the real eat command,
     * leaving the pack empty.  'oil': the same, then the character picks
     * up one Flask of Oil (fixture-carried), so exactly one pack slot is
     * used.  In both modes the character has no food left. */
    {
        struct object *food = NULL;
        for (struct object *o = player->gear; o; o = o->next)
            if (tval_is_edible(o) && !object_is_equipped(player->body, o)) food = o;
        if (!food) { printf("SETUPFAIL nofood\n"); return 1; }
        player->timed[TMD_FOOD] = PY_FOOD_WEAK;
        cmdq_push(CMD_EAT);
        cmd_set_arg_item(cmdq_peek(), "item", food);
        cmdq_execute(CTX_GAME);
        printf("ATE pack_items=%d\n", pack_count());
    }
    if (streq(mode, "oil")) {
        struct object *oil = new_item(TV_FLASK, "Flask of Oil", 1);
        make_known(oil);
        inven_carry(player, oil, false, false);
    }
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;

    /* Every store is emptied, then the General Store is given five Rations
     * of Food so the stock is the same in every run. */
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
    }
    {
        struct store *gen = &stores[0];
        printf("STORE0 [%s]\n", f_info[gen->feat].name);
        struct object *obj = new_item(TV_FOOD, "Ration of Food", 5);
        make_known(obj);
        if (!store_carry(gen, obj, false)) { printf("SETUPFAIL store\n"); return 1; }
    }
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=60 mode=%s class=%s clevel=%d gold=%d food_items=%d light=%d\n",
           mode, player->class->name, player->lev, borg.trait[BI_GOLD],
           borg.trait[BI_FOOD], borg.trait[BI_LIGHT]);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_shop(void *state)
{
    const char *mode = getenv("ES60_MODE");
    int used = 0;
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty) {
            used++;
            printf("PACK %d qty=%d desc=[%s]\n", i, borg_items[i].iqty, borg_items[i].desc);
        }
    for (int k = 0; k < z_info->store_max; ++k)
        for (int n = 0; n < z_info->store_inven_max; ++n)
            if (borg_shops[k].ware[n].iqty)
                printf("SHOP %d ware %d qty=%d cost=%d desc=[%s]\n", k, n,
                       borg_shops[k].ware[n].iqty, (int)borg_shops[k].ware[n].cost,
                       borg_shops[k].ware[n].desc);
    int hole = borg_first_empty_inventory_slot();
    printf("TRACK pack_used=%d first_empty_slot=%d inventory_full=%d\n",
           used, hole, borg_inventory_full() ? 1 : 0);
    /* The production town decision: choose a shop to visit, which buys
     * food immediately when the Borg has none. */
    bool chose = borg_choose_shop();
    bool buys_food = chose && borg.goal.shop >= 0 && borg.goal.ware >= 0
        && borg_shops[borg.goal.shop].ware[borg.goal.ware].tval == TV_FOOD;
    printf("DECISION chose_shop=%d goal_shop=%d goal_ware=%d what=[%s]\n",
           chose ? 1 : 0, borg.goal.shop, borg.goal.ware,
           (chose && borg.goal.shop >= 0 && borg.goal.ware >= 0)
               ? borg_shops[borg.goal.shop].ware[borg.goal.ware].desc : "(nothing)");
    printf("OUTCOME buys_food=%d\n", buys_food ? 1 : 0);
    (void)mode;
    /* Correct contract: a starving Borg with gold in town, a ration on sale
     * and room in its pack sets out to buy the ration. */
    require(buys_food);
    ok;
}

const char *suite_name = "native-c/emptyslot-60";
struct test tests[] = {
    {"starving_borg_buys_food_with_room_in_pack", scenario_shop},
    {NULL, NULL}};
