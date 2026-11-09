/* Synthetic native C home-buy fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#19 (homebuyqty-19). */
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
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include "borg/borg-trait-swap.h"
#include "borg/borg-store.h"
#include "borg/borg-store-buy.h"
#include "borg/borg-think-store.h"
#include "borg/borg-cave.h"
#include "borg/borg-power.h"
#include "borg/borg-io.h"
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

static void carry(struct object *obj)
{
    make_known(obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
}

/* Distinct single items used to fill the pack. */
static const struct { int tval; const char *name; } filler[] = {
    {TV_POTION, "Cure Light Wounds"}, {TV_POTION, "Cure Serious Wounds"},
    {TV_POTION, "Cure Critical Wounds"}, {TV_POTION, "Healing"},
    {TV_POTION, "Speed"}, {TV_POTION, "Heroism"},
    {TV_POTION, "Resist Heat"}, {TV_POTION, "Resist Cold"},
    {TV_POTION, "Resist Poison"}, {TV_POTION, "Neutralize Poison"},
    {TV_POTION, "True Seeing"}, {TV_SCROLL, "Phase Door"},
    {TV_SCROLL, "Teleportation"}, {TV_SCROLL, "Teleport Level"},
    {TV_SCROLL, "Magic Mapping"}, {TV_SCROLL, "Deep Descent"},
    {TV_SCROLL, "Blessing"}, {TV_SCROLL, "Holy Chant"},
    {TV_SCROLL, "Light"}, {TV_SCROLL, "Detect Invisible"},
    {TV_SCROLL, "Remove Hunger"}, {TV_SCROLL, "Banishment"},
};

int setup_tests(void **state)
{
    const char *profile = getenv("HB19_PROFILE");
    const char *mode    = getenv("HB19_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(19);
    if (!player_make_simple("Human", "Warrior", "HomeBuy19")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = cave->depth = player->cave->depth = 0;
    player->max_depth = 50;
    player_exp_gain(player, player_exp[19] * player->expfact / 100);
    player->au = 5000;
    character_dungeon = true;
    for (struct object *o = player->gear; o; o = o->next)
        if (!object_is_equipped(player->body, o) && wield_slot(o) >= 0
            && (tval_is_light(o) || tval_is_weapon(o) || tval_is_armor(o))) {
            inven_wield(o, wield_slot(o));
            o = player->gear;
        }

    /* Fill the pack with distinct single items until only the last pack
     * slot is free ('roomy': the last two slots are free). */
    {
        int want = z_info->pack_size - (streq(mode, "roomy") ? 2 : 1);
        size_t i = 0;
        calc_inventory(player);
        while (pack_slots_used(player) < want
               && i < sizeof(filler) / sizeof(filler[0])) {
            carry(new_item(filler[i].tval, filler[i].name, 1));
            calc_inventory(player);
            i++;
        }
        if (pack_slots_used(player) != want) { printf("SETUPFAIL fill %d/%d\n", pack_slots_used(player), want); return 1; }
    }
    player_learn_all_runes(player);
    for (struct object *o = player->gear; o; o = o->next) player_know_object(player, o);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;

    /* Every store is emptied.  The home holds one Tulwar ('pair': two
     * Tulwars; 'stack': one Potion of Cure Light Wounds, which stacks with
     * the one in the pack). */
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
    }
    {
        struct store *home = NULL;
        for (int i = 0; i < z_info->store_max; i++)
            if (stores[i].feat == FEAT_HOME) home = &stores[i];
        if (!home) { printf("SETUPFAIL home\n"); return 1; }
        struct object *obj = streq(mode, "stack")
            ? new_item(TV_POTION, "Cure Light Wounds", 1)
            : new_item(TV_SWORD, "Tulwar", streq(mode, "pair") ? 2 : 1);
        make_known(obj);
        if (!store_carry(home, obj, false)) { printf("SETUPFAIL store\n"); return 1; }
    }
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=19 mode=%s class=%s clevel=%d gold=%d pack_slots=%d free_from=%d\n",
           mode, player->class->name, player->lev, (int)player->au,
           (int)PACK_SLOTS, borg_first_empty_inventory_slot());
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_home(void *state)
{
    const char *mode = getenv("HB19_MODE");
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty)
            printf("PACK %d qty=%d desc=[%s]\n", i, borg_items[i].iqty, borg_items[i].desc);
    for (int n = 0; n < z_info->store_inven_max; ++n)
        if (borg_shops[BORG_HOME].ware[n].iqty)
            printf("HOME %d qty=%d desc=[%s]\n", n, borg_shops[BORG_HOME].ware[n].iqty,
                   borg_shops[BORG_HOME].ware[n].desc);
    printf("TRACK power_now=%d uses_swaps=%d weapon_swap=%d\n", (int)borg.power,
           borg_uses_swaps() ? 1 : 0, weapon_swap);
    /* The production store steps 4 to 7B, in the order borg_choose_shop()
     * runs them; the first one that sets a goal decides. */
    const char *step = "none";
    int qty_before = borg_shops[BORG_HOME].ware[0].iqty;
    int qty_after4 = qty_before;
    bool acts = false;
    if (borg_think_home_buy_useful()) { acts = true; step = "4 buy useful from home"; }
    else {
        qty_after4 = borg_shops[BORG_HOME].ware[0].iqty;
        printf("STATE home_ware0_qty_after_step4=%d\n", qty_after4);
        if (borg_think_home_grab_useless()) { acts = true; step = "5 grab useless from home"; }
        else if (borg_think_shop_grab_interesting()) { acts = true; step = "6 grab interesting for home"; }
        else if (borg_uses_swaps() && borg_think_home_buy_swap_weapon()) { acts = true; step = "7A buy backup weapon from home"; }
        else if (borg_uses_swaps() && borg_think_home_buy_swap_armour()) { acts = true; step = "7B buy backup armour from home"; }
    }
    printf("STEP decided_by=[%s]\n", step);
    printf("STATE home_ware0_qty_after_steps=%d\n",
           borg_shops[BORG_HOME].ware[0].iqty);
    bool buys_home_ware0 = acts && borg.goal.shop == BORG_HOME && borg.goal.ware == 0
                       && borg.goal.item == -1;
    printf("DECISION acts=%d goal_shop=%d goal_ware=%d goal_item=%d\n", acts ? 1 : 0,
           borg.goal.shop, borg.goal.ware, borg.goal.item);
    if (acts && borg.goal.item >= 0) printf("NOTE goal_item_desc=[%s]\n", borg_items[borg.goal.item].desc);
    printf("OUTCOME buys_home_ware0=%d\n", buys_home_ware0 ? 1 : 0);
    /* Correct contract: a step-4 trial that is skipped leaves the home's
     * record of that ware whole, and with a weapon at home and room for one
     * more item the Borg takes it (as a backup weapon, or for use). */
    require(qty_after4 == qty_before);
    if (!streq(mode, "stack"))
        require(buys_home_ware0);
    ok;
}

const char *suite_name = "native-c/homebuyqty-19";
struct test tests[] = {
    {"home_ware_is_considered", scenario_home},
    {NULL, NULL}};
