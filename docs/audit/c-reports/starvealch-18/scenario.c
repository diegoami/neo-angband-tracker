/* Synthetic native C town-shopping fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#18 (starvealch-18). */
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
#include "borg/borg-item-val.h"
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
static bool carried_no_food;

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

static int stock(int idx, int tval, const char *name, int number)
{
    struct store *s = &stores[idx];
    struct object *obj = new_item(tval, name, number);
    make_known(obj);
    if (!store_carry(s, obj, false)) return 0;
    printf("STOCK store=%d [%s] %dx %s\n", idx, f_info[s->feat].name, number, name);
    return 1;
}

int setup_tests(void **state)
{
    const char *profile = getenv("SA18_PROFILE");
    const char *mode    = getenv("SA18_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(18);
    if (!player_make_simple("Human", "Warrior", "StarveAlch18")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    character_dungeon = true;
    /* The starting Word of Recall scroll and Potion of Berserk Strength are
     * removed: at character level 1 the Borg otherwise first takes the
     * scroll to the home and sells the potion, which ends the shop choice
     * before any purchase is weighed.  In the starving modes the starting
     * Rations of Food are removed too, so the Borg carries no food. */
    {
        bool drop_food = !streq(mode, "fed");
        struct object *o = player->gear, *next;
        for (; o; o = next) {
            next = o->next;
            if (tval_is_scroll(o) || tval_is_potion(o)
                || (drop_food && tval_is_edible(o))) {
                bool none_left = false;
                struct object *gone = gear_object_for_use(player, o, o->number,
                    false, &none_left);
                object_delete(player->cave, NULL, &gone);
            }
        }
    }
    /* Fixture gold. */
    player->au = 500;
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;

    /* In the starving modes the character is also Hungry, set with the
     * game's own timed-effect call. */
    if (!streq(mode, "fed"))
        player_set_timed(player, TMD_FOOD, PY_FOOD_HUNGRY - 1, false, false);
    printf("TIMED food=%d hungry_below=%d\n", player->timed[TMD_FOOD], PY_FOOD_HUNGRY);

    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;

    /* Every store is emptied, then stocked: the Alchemist (store index 4)
     * always gets five Scrolls of Remove Hunger and five Potions of Cure
     * Light Wounds; the General Store (index 0) gets five Flasks of Oil,
     * plus five Rations of Food in the "general_food" mode only. */
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
        printf("STORE %d [%s]\n", i, f_info[s->feat].name);
    }
    if (!stock(4, TV_SCROLL, "Remove Hunger", 5)) { printf("SETUPFAIL store\n"); return 1; }
    if (!stock(4, TV_POTION, "Cure Light Wounds", 5)) { printf("SETUPFAIL store\n"); return 1; }
    if (!stock(0, TV_FLASK, "Flask of Oil", 5)) { printf("SETUPFAIL store\n"); return 1; }
    if (streq(mode, "general_food")
        && !stock(0, TV_FOOD, "Ration of Food", 5)) { printf("SETUPFAIL store\n"); return 1; }

    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    carried_no_food = (borg.trait[BI_FOOD] == 0);
    printf("SETUP seed=18 mode=%s class=%s clevel=%d gold=%d food=%d hungry=%d weak=%d light=%d\n",
           mode, player->class->name, player->lev, borg.trait[BI_GOLD],
           borg.trait[BI_FOOD], borg.trait[BI_ISHUNGRY], borg.trait[BI_ISWEAK],
           borg.trait[BI_LIGHT]);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_shop(void *state)
{
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty)
            printf("PACK %d qty=%d desc=[%s]\n", i, borg_items[i].iqty, borg_items[i].desc);
    for (int k = 0; k < z_info->store_max; ++k)
        for (int n = 0; n < z_info->store_inven_max; ++n)
            if (borg_shops[k].ware[n].iqty)
                printf("SHOP %d ware %d qty=%d cost=%d desc=[%s]\n", k, n,
                       borg_shops[k].ware[n].iqty, (int)borg_shops[k].ware[n].cost,
                       borg_shops[k].ware[n].desc);
    printf("TRACK power=%d do_best=%d goal_item=%d\n", (int)borg.power,
           borg.goal.do_best ? 1 : 0, borg.goal.item);
    /* The production town decision: choose a shop to visit. */
    bool chose = borg_choose_shop();
    bool has = chose && borg.goal.shop >= 0 && borg.goal.ware >= 0;
    const char *what = has ? borg_shops[borg.goal.shop].ware[borg.goal.ware].desc : "(nothing)";
    borg_item *w = has ? &borg_shops[borg.goal.shop].ware[borg.goal.ware] : NULL;
    bool remove_hunger = w && w->tval == TV_SCROLL && w->sval == sv_scroll_satisfy_hunger;
    bool food = w && w->tval == TV_FOOD;
    printf("DECISION chose_shop=%d goal_shop=%d goal_ware=%d shop_name=[%s] what=[%s]\n",
           chose ? 1 : 0, borg.goal.shop, borg.goal.ware,
           borg.goal.shop >= 0 ? f_info[stores[borg.goal.shop].feat].name : "(none)", what);
    printf("OUTCOME buys_food=%d buys_remove_hunger=%d\n", food ? 1 : 0,
           remove_hunger ? 1 : 0);
    /* Correct contract: a Borg in town with gold that carries no food sets
     * out to buy food or, when the General Store has none, Remove Hunger
     * from the Alchemist; a Borg that carries food chooses some purchase. */
    if (carried_no_food)
        require(food || remove_hunger);
    else
        require(has);
    ok;
}

const char *suite_name = "native-c/starvealch-18";
struct test tests[] = {
    {"borg_in_town_buys_food_or_remove_hunger", scenario_shop},
    {NULL, NULL}};
