/* Synthetic native C town-shopping fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#23 (urgentshop-23). */
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
    const char *profile = getenv("US23_PROFILE");
    const char *mode    = getenv("US23_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(23);
    if (!player_make_simple("Human", "Warrior", "UrgentShop23")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    character_dungeon = true;
    /* The starting Word of Recall scroll and Potion of Berserk Strength are
     * removed: at character level 1 the Borg otherwise first takes the
     * scroll to the home and sells the potion (steps 1 and 2 of the shop
     * choice), which ends the decision before any purchase is weighed. */
    {
        struct object *o = player->gear, *next;
        for (; o; o = next) {
            next = o->next;
            if (tval_is_scroll(o) || tval_is_potion(o)) {
                bool none_left = false;
                struct object *gone = gear_object_for_use(player, o, o->number,
                    false, &none_left);
                object_delete(player->cave, NULL, &gone);
            }
        }
    }
    /* Fixture gold: enough for the potions on sale. */
    player->au = 500;
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;

    /* The urgent condition is set with the game's own timed-effect call. */
    if (streq(mode, "poisoned"))
        player_inc_timed(player, TMD_POISONED, 10, false, false, false);
    else if (streq(mode, "bleeding"))
        player_inc_timed(player, TMD_CUT, 10, false, false, false);
    printf("TIMED poisoned=%d cut=%d\n", player->timed[TMD_POISONED], player->timed[TMD_CUT]);

    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;

    /* Every store is emptied, then stocked the same way in every mode:
     * Cure Light Wounds potions at the Alchemy shop and one book at the
     * Bookseller (store index 3). */
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
        printf("STORE %d [%s]\n", i, f_info[s->feat].name);
    }
    if (!stock(4, TV_POTION, "Cure Light Wounds", 5)) { printf("SETUPFAIL store\n"); return 1; }
    if (!stock(3, TV_MAGIC_BOOK, "[First Spells]", 1)) { printf("SETUPFAIL store\n"); return 1; }

    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=23 mode=%s class=%s clevel=%d gold=%d food=%d light=%d poisoned=%d cut=%d aclw=%d\n",
           mode, player->class->name, player->lev, borg.trait[BI_GOLD],
           borg.trait[BI_FOOD], borg.trait[BI_LIGHT], borg.trait[BI_ISPOISONED],
           borg.trait[BI_ISCUT], borg.trait[BI_ACLW]);
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
    bool buys_cure = has && borg_shops[borg.goal.shop].ware[borg.goal.ware].tval == TV_POTION;
    printf("DECISION chose_shop=%d goal_shop=%d goal_ware=%d shop_name=[%s] what=[%s]\n",
           chose ? 1 : 0, borg.goal.shop, borg.goal.ware,
           borg.goal.shop >= 0 ? f_info[stores[borg.goal.shop].feat].name : "(none)", what);
    printf("TRACK goal_item=%d item=[%s]\n", borg.goal.item,
           borg.goal.item >= 0 ? borg_items[borg.goal.item].desc : "(none)");
    printf("OUTCOME buys_cure=%d\n", buys_cure ? 1 : 0);
    /* Correct contract: a Borg in town with gold and Cure Light Wounds on
     * sale at the Alchemy shop sets out to buy the potion. */
    require(buys_cure);
    ok;
}

const char *suite_name = "native-c/urgentshop-23";
struct test tests[] = {
    {"borg_in_town_buys_cure_potion", scenario_shop},
    {NULL, NULL}};
