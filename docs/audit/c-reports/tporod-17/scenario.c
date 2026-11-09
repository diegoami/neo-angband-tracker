/* Synthetic native C Black Market shopping fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#17 (tporod-17). */
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

/* Modes: "warrior" (level 30, 10000 gold: a class that cannot cast
 * Teleport Other, short of the price), "warrior-buy" (level 12, 16000
 * gold: enough for the rod, not enough for the Black Market's general
 * rich-Borg rule), "rogue" (level 30, 10000 gold, carries Arcane Control,
 * which holds Teleport Other) and "mage" (level 30, 10000 gold, carries
 * Magical Defences, which holds Teleport Other). */
int setup_tests(void **state)
{
    const char *profile = getenv("TP17_PROFILE");
    const char *mode    = getenv("TP17_MODE");
    if (!profile || !mode) return 1;
    const char *cls = streq(mode, "rogue") ? "Rogue" : streq(mode, "mage") ? "Mage" : "Warrior";
    int clevel = streq(mode, "warrior-buy") ? 12 : 30;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(17);
    player->opts.opt[OPT_birth_start_kit] = false;
    if (!player_make_simple("Human", cls, "TpRod17")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    character_dungeon = true;
    /* Character level through real experience gain. */
    player_exp_gain(player, player_exp[clevel - 2] * player->expfact / 100);
    /* Fixture stats: base Dexterity 18/120 so the Borg's anti-theft check
     * for scumming (adj_dex_safe + level > 90) is met in every mode. */
    player->stat_max[STAT_DEX] = player->stat_cur[STAT_DEX] = 18 + 120;
    player->au = streq(mode, "warrior-buy") ? 16000 : 10000;
    {
        struct object *torch = new_item(TV_LIGHT, "Wooden Torch", 1);
        make_known(torch);
        torch->timeout = 5000;
        inven_carry(player, torch, false, false);
        inven_wield(torch, wield_slot(torch));
    }
    /* 120 Rations of Food: the scum rule wants at least 100 food items. */
    for (int i = 0; i < 3; ++i) carry(TV_FOOD, "Ration of Food", 40);
    if (streq(mode, "mage")) carry(TV_MAGIC_BOOK, "[Magical Defences]", 1);
    if (streq(mode, "rogue")) carry(TV_MAGIC_BOOK, "[Arcane Control]", 1);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_TORCH | PU_INVEN;
    update_stuff(player);
    for (struct object *o = player->gear; o; o = o->next) {
        /* Casters study every spell of the carried book through the game's
         * own spell_learn() (all are level 30 or lower). */
        if (o->tval != TV_MAGIC_BOOK) continue;
        int *spells = NULL;
        int num = spell_collect_from_book(player, o, &spells);
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
    printf("SETUP seed=17 mode=%s class=%s clevel=%d gold=%d food=%d light=%d dex_safe=%d self_scum=%d\n",
           mode, player->class->name, borg.trait[BI_CLEVEL], borg.trait[BI_GOLD],
           borg.trait[BI_FOOD], borg.trait[BI_LIGHT],
           adj_dex_safe[borg.trait[BI_DEX_INDEX]], borg_cfg[BORG_SELF_SCUM] ? 1 : 0);
    printf("STATE atportother=%d\n", borg.trait[BI_ATPORTOTHER]);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_shop(void *state)
{
    const char *mode = getenv("TP17_MODE");
    for (int k = 0; k < z_info->store_max; ++k)
        for (int n = 0; n < z_info->store_inven_max; ++n)
            if (borg_shops[k].ware[n].iqty)
                printf("SHOP %d ware %d qty=%d cost=%d desc=[%s]\n", k, n,
                       borg_shops[k].ware[n].iqty, (int)borg_shops[k].ware[n].cost,
                       borg_shops[k].ware[n].desc);
    printf("TRACK money_scum_amount=%d (before shopping)\n", borg_cfg[BORG_MONEY_SCUM_AMOUNT]);
    /* The production shop step that scans every store with borg_good_buy(). */
    bool chose = borg_think_shop_buy_useful();
    int target = borg_cfg[BORG_MONEY_SCUM_AMOUNT];
    printf("DECISION buy_useful=%d goal_shop=%d goal_ware=%d money_scum_amount=%d\n",
           chose ? 1 : 0, borg.goal.shop, borg.goal.ware, target);
    /* The production town step the Borg takes after shopping when a scum
     * target is set: stay in town and wait for gold instead of leaving. */
    borg_flush();
    bool scum = borg_money_scum();
    keycode_t key = borg_inkey(false);
    printf("DECISION money_scum=%d first_key=%d\n", scum ? 1 : 0, (int)key);
    bool buys = chose && borg.goal.shop == 6 && borg.goal.ware == 0;
    printf("OUTCOME saves_for_rod=%d buys_rod=%d\n", target > 0 ? 1 : 0, buys ? 1 : 0);
    /* Correct contract: classes that cannot cast Teleport Other save up for
     * the Black Market rod while short of its price, and buy it once they
     * can pay for it; Mages and Rogues, who cast it, do neither. */
    if (streq(mode, "warrior"))
        require(target > 0 && scum && !buys);
    else if (streq(mode, "warrior-buy"))
        require(buys);
    else
        require(target == 0 && !scum && !buys);
    ok;
}

const char *suite_name = "native-c/tporod-17";
struct test tests[] = {
    {"black_market_teleport_other_rod_class_gate", scenario_shop},
    {NULL, NULL}};
