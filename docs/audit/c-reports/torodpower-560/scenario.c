/* Synthetic native C Black Market shopping fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#560 (torodpower-560). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "object.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player.h"
#include "store.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include "borg/borg-store.h"
#include "borg/borg-store-buy.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-cave.h"
#include "borg/borg-io.h"
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
    object_prep(obj, kind, 30, MINIMISE);
    obj->number = number;
    return obj;
}

static void carry(int tval, const char *name, int number)
{
    struct object *obj = new_item(tval, name, number);
    make_known(obj);
    inven_carry(player, obj, false, false);
}

/* Modes: "rod" (a Warrior with no Teleport Other of its own),
 * "wand" (a Warrior already carrying Wands of Teleport Other with 16
 * charges), "mage" (a Mage carrying Magical Defences, which holds the
 * Teleport Other spell).  In every mode the Black Market offers one Rod of
 * Teleport Other. */
int setup_tests(void **state)
{
    const char *profile = getenv("TP560_PROFILE");
    const char *mode    = getenv("TP560_MODE");
    if (!profile || !mode) return 1;
    const char *cls = streq(mode, "mage") ? "Mage" : "Warrior";
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(560);
    player->opts.opt[OPT_birth_start_kit] = false;
    if (!player_make_simple("Human", cls, "RodPower")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    character_dungeon = true;
    /* Character level 30 through real experience gain. */
    player_exp_gain(player, player_exp[28] * player->expfact / 100);
    player->au = 20000;
    {
        struct object *torch = new_item(TV_LIGHT, "Wooden Torch", 1);
        make_known(torch);
        torch->timeout = 5000;
        inven_carry(player, torch, false, false);
        inven_wield(torch, wield_slot(torch));
    }
    carry(TV_FOOD, "Ration of Food", 5);
    if (streq(mode, "wand")) {
        /* Two wands (6+d5 charges each in object.txt); 16 charges in all. */
        struct object *wand = new_item(TV_WAND, "Teleport Other", 2);
        wand->pval = 16;
        make_known(wand);
        inven_carry(player, wand, false, false);
    }
    if (streq(mode, "mage"))
        carry(TV_MAGIC_BOOK, "[Magical Defences]", 1);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_TORCH | PU_INVEN;
    update_stuff(player);
    if (streq(mode, "mage")) {
        /* The Mage studies every spell of the carried book through the
         * game's own spell_learn() (all are level 30 or lower). */
        struct object *book = NULL;
        for (struct object *o = player->gear; o; o = o->next)
            if (o->tval == TV_MAGIC_BOOK) book = o;
        int *spells = NULL;
        int num = spell_collect_from_book(player, book, &spells);
        for (int i = 0; i < num; i++)
            if (spell_okay_to_study(player, spells[i])) spell_learn(spells[i]);
        mem_free(spells);
        player->upkeep->update |= PU_MANA | PU_SPELLS;
        update_stuff(player);
    }
    player->chp = player->mhp;
    player->csp = player->msp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;

    /* Every store is emptied, then the Black Market is given one rod. */
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
    }
    {
        struct store *bm = &stores[6];
        struct object *obj = new_item(TV_ROD, "Teleport Other", 1);
        make_known(obj);
        if (!store_carry(bm, obj, false)) { printf("SETUPFAIL store\n"); return 1; }
        printf("STORE6 [%s] stocks a Rod of Teleport Other\n", f_info[bm->feat].name);
    }
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=560 mode=%s class=%s clevel=%d gold=%d msp=%d\n",
           mode, player->class->name, borg.trait[BI_CLEVEL], borg.trait[BI_GOLD], player->msp);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_shop(void *state)
{
    const char *mode = getenv("TP560_MODE");
    int bm_ware = -1;
    for (int n = 0; n < z_info->store_inven_max; ++n)
        if (borg_shops[6].ware[n].iqty) {
            printf("SHOP 6 ware %d qty=%d cost=%d desc=[%s]\n", n,
                   borg_shops[6].ware[n].iqty, (int)borg_shops[6].ware[n].cost,
                   borg_shops[6].ware[n].desc);
            bm_ware = n;
        }
    int power_now = borg.power;
    /* Diagnostic only: the power the Borg sees with the rod added to an
     * empty pack slot, the same way the shop step evaluates a ware. */
    int hole = borg_first_empty_inventory_slot();
    memcpy(&borg_items[hole], &borg_safe_shops[6].ware[bm_ware], sizeof(borg_item));
    borg_items[hole].iqty = 1;
    borg_notice(true);
    int power_rod = borg_power();
    borg_items[hole].iqty = 0;
    borg_notice(true);
    printf("TRACK power_now=%d power_with_rod=%d\n", power_now, power_rod);
    printf("STATE atportother=%d power_gain=%d\n", borg.trait[BI_ATPORTOTHER], power_rod - power_now);
    /* The production shop step that scans every store with borg_good_buy()
     * and buys the ware that raises borg_power() the most. */
    bool chose = borg_think_shop_buy_useful();
    bool buys = chose && borg.goal.shop == 6 && borg.goal.ware == bm_ware;
    printf("DECISION buy_useful=%d goal_shop=%d goal_ware=%d\n",
           chose ? 1 : 0, borg.goal.shop, borg.goal.ware);
    printf("OUTCOME buys_rod=%d\n", buys ? 1 : 0);
    /* Correct contract: a Warrior without any Teleport Other buys the rod;
     * a Borg that already has Teleport Other (enough wand charges, or the
     * spell) gains nothing from it and does not. */
    if (streq(mode, "rod"))
        require(buys);
    else
        require(!buys);
    ok;
}

const char *suite_name = "native-c/torodpower-560";
struct test tests[] = {
    {"black_market_teleport_other_rod_value", scenario_shop},
    {NULL, NULL}};
