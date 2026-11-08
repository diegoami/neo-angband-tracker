/* Synthetic native C swap-weapon fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#201 (trollswap-201). */
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
#include "borg/borg-store-sell.h"
#include "borg/borg-cave.h"
#include "borg/borg-power.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

/* Make the object fully known, as after normal play with every rune learned. */
static struct object *carry_known(struct object *obj)
{
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 50, MAXIMISE);
    obj->number = number;
    return carry_known(obj);
}

/* A Mace with the given ego ("of Slay Orc", "of Slay Troll") or none. */
static struct object *mace(const char *ego, int to_d)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(TV_HAFTED, lookup_sval(TV_HAFTED, "Mace"));
    if (!kind) quit("fixture mace kind missing");
    object_prep(obj, kind, 50, MAXIMISE);
    if (ego) {
        obj->ego = lookup_ego_item(ego, kind->tval, kind->sval);
        if (!obj->ego) quit("fixture ego missing");
        ego_apply_magic(obj, 50);
    }
    obj->to_d += to_d;
    return carry_known(obj);
}

static void wear(struct object *obj)
{
    inven_wield(obj, wield_slot(obj));
}

int setup_tests(void **state)
{
    const char *profile = getenv("TS201_PROFILE");
    const char *mode    = getenv("TS201_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(201);
    if (!player_make_simple("Human", "Warrior", "TrollSwap201")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    /* The Borg only carries a swap weapon once its deepest level is 50. */
    player->depth = cave->depth = player->cave->depth = 1;
    player->max_depth = 50;
    player_exp_gain(player, player_exp[28] * player->expfact / 100);
    player->au = 0;
    character_dungeon = true;
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    wear(torch);
    /* The Warrior wields the dagger from the starting kit, so the only spare
     * weapons carried are the two maces below. */
    for (struct object *o = player->gear; o; o = o->next)
        if (tval_is_melee_weapon(o) && o->tval == TV_SWORD) { wear(o); break; }
    known_item(TV_FOOD, "Ration of Food", 6);
    /* Remove the starting Potion of Berserk Strength: it is a spare the
     * Borg would offer for sale first (cheapest first), which would hide the
     * choice between the two maces. */
    for (struct object *o = player->gear; o; o = o->next)
        if (tval_is_potion(o)) {
            bool none = false;
            struct object *used = gear_object_for_use(player, o, o->number, false, &none);
            object_delete(player->cave, NULL, &used);
            break;
        }
    /* Two spare Maces, identical but for their slay. The troll one carries
     * one extra point of damage so that, once valued correctly, it is
     * strictly the better swap rather than a tie. */
    if (streq(mode, "troll-orc")) {
        mace("of Slay Orc", 0);
        mace("of Slay Troll", 1);
    } else if (streq(mode, "troll-plain")) {
        mace(NULL, 0);
        mace("of Slay Troll", 0);
    } else {
        printf("SETUPFAIL mode\n");
        return 1;
    }
    /* The player has learned every rune, as after normal play. */
    player_learn_all_runes(player);
    for (struct object *o = player->gear; o; o = o->next) player_know_object(player, o);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    borg_equipment(); borg_inventory(); borg_notice(true);
    /* As on entering the town: record the shops and the safe copy of the pack. */
    borg_find_all_items();
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=201 mode=%s class=%s clevel=%d maxdepth=%d gold=%d\n",
           mode, player->class->name, player->lev, player->max_depth, (int)player->au);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

/* Pack index of the first item whose description contains text, or -1. */
static int find_item(const char *text)
{
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty && strstr(borg_items[i].desc, text)) return i;
    return -1;
}

static int scenario_sell(void *state)
{
    const char *mode = getenv("TS201_MODE");
    bool changed = streq(mode, "troll-orc");
    borg_notice(true);
    borg.power = borg_power();
    int troll = find_item("Slay Troll");
    int other = changed ? find_item("Slay Orc") : find_item("Mace");
    if (!changed) {
        /* the plain mace is the one that does not mention a slay */
        for (int i = 0; i < z_info->pack_size; ++i)
            if (borg_items[i].iqty && borg_items[i].tval == TV_HAFTED
                && !strstr(borg_items[i].desc, "Slay")) other = i;
    }
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty) printf("PACK %d tval=%d desc=[%s] ego=%d value=%d\n", i, borg_items[i].tval, borg_items[i].desc, (int)borg_items[i].ego_idx, (int)borg_items[i].value);
    require(troll >= 0 && other >= 0);
    printf("STATE troll_slot=%d other_slot=%d swap_slot=%d\n", troll, other, weapon_swap - 1);
    /* Only the changed case prints the swap value: it is the number the
     * correction moves; the control must print identical logs. */
    if (changed)
        printf("TRACK swap_value=%d\n", (int)weapon_swap_value);
    printf("TRACK swap_weapon=%s\n", weapon_swap ? borg_items[weapon_swap - 1].desc : "(none)");
    /* The production "sell useless items" decision. */
    borg.goal.shop = borg.goal.item = -1;
    bool sells = borg_think_shop_sell_useless();
    int item = sells ? borg.goal.item : -1;
    printf("TRACK sells=%d item_slot=%d item=%s\n", sells ? 1 : 0, item,
           item >= 0 ? borg_items[item].desc : "(nothing)");
    printf("DECISION sell_slot=%d keeps_troll_weapon=%d\n", item, (weapon_swap - 1) == troll ? 1 : 0);
    printf("OUTCOME sells_other=%d sells_troll=%d\n", item == other ? 1 : 0, item == troll ? 1 : 0);
    /* Correct contract: the Slay Troll weapon is the swap weapon being kept,
     * so the spare that is offered for sale is the other mace. */
    require(sells);
    require((weapon_swap - 1) == troll);
    require(item == other);
    ok;
}

const char *suite_name = "native-c/trollswap-201";
struct test tests[] = {
    {"troll_slay_weapon_is_kept_as_swap", scenario_sell},
    {NULL, NULL}};
