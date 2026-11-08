/* Native C triage fixture for diegoami/neo-angband-tracker#186 (encumb-186):
 * the encumbrance "just bought" exemption in borg_power_inventory(). */
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
#include "borg/borg-store.h"
#include "borg/borg-store-buy.h"
#include "borg/borg-cave.h"
#include "borg/borg-power.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

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
    object_prep(obj, kind, 0, MINIMISE);
    obj->number = number;
    return carry_known(obj);
}

/* Refresh the Borg's view of the pack and its traits. */
static void borg_refresh(void)
{
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    borg_equipment(); borg_inventory(); borg_notice(true);
}

int setup_tests(void **state)
{
    const char *profile = getenv("EN186_PROFILE");
    const char *mode    = getenv("EN186_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(186);
    if (!player_make_simple("Human", "Warrior", "Encumb186")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    /* Enough gold that the Borg will shop at the black market. */
    player->au = 30000;
    character_dungeon = true;
    /* The Warrior wields the Dagger from the starting kit. */
    for (struct object *o = player->gear; o; o = o->next)
        if (o->tval == TV_SWORD) { inven_wield(o, wield_slot(o)); break; }
    /* A Warrior in heavy armour: Full Plate Armour worn (the starting soft
     * leather stays in the pack). */
    {
        struct object *plate = known_item(TV_HARD_ARMOR, "Full Plate Armour", 1);
        inven_wield(plate, wield_slot(plate));
    }
    struct object *torch = NULL;
    for (struct object *o = player->gear; o; o = o->next)
        if (tval_is_light(o)) { torch = o; break; }
    if (torch) inven_wield(torch, wield_slot(torch));
    /* 'quiver': twenty iron shots in the quiver (one quiver slot). */
    if (streq(mode, "quiver")) known_item(TV_SHOT, "Iron Shot", 20);
    player_learn_all_runes(player);
    for (struct object *o = player->gear; o; o = o->next) player_know_object(player, o);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    borg_refresh();

    /* Load the pack with flasks of oil and rations so that the carried weight
     * sits at a chosen point of the encumbrance scale (each tenth of the
     * weight limit over half of it costs one point of speed). */
    int carry = borg.trait[BI_CARRY], step = carry / 10, half = carry / 2;
    int base = borg.trait[BI_WEIGHT], target = base;
    if (streq(mode, "edge") || streq(mode, "quiver"))
        target = half + step - 3;     /* the scroll's 0.5 lb crosses a step */
    else if (streq(mode, "mid") || streq(mode, "str"))
        target = half + step + step / 2;     /* inside the second step */
    if (target != base) {
        int d = target - base;
        d -= ((d % 4) + 4) % 4;                 /* flasks 2 lb, rations 0.8 lb */
        int rations = 0;
        while ((d - 8 * rations) % 20) ++rations;
        int flasks = (d - 8 * rations) / 20;
        if (flasks < 0 || flasks > 80 || rations > 40) { printf("SETUPFAIL weight %d\n", d); return 1; }
        /* at most 40 to a stack */
        if (flasks > 40) { known_item(TV_FLASK, "Flask of Oil", 40); flasks -= 40; }
        if (flasks) known_item(TV_FLASK, "Flask of Oil", flasks);
        if (rations) known_item(TV_FOOD, "Ration of Food", rations);
        player_know_object(player, player->gear);
        borg_refresh();
    }

    /* The stores hold one item: an Enchant Weapon To-Hit scroll in the
     * black market. */
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
    }
    {
        struct store *bm = NULL;
        for (int i = 0; i < z_info->store_max; i++)
            if (stores[i].feat == FEAT_STORE_BLACK) bm = &stores[i];
        struct object_kind *kind = lookup_kind(TV_SCROLL,
            lookup_sval(TV_SCROLL, "Enchant Weapon To-Hit"));
        if (streq(mode, "str") || streq(mode, "strlight"))
            kind = lookup_kind(TV_POTION, lookup_sval(TV_POTION, "Strength"));
        struct object *obj = object_new();
        object_prep(obj, kind, 0, MINIMISE);
        obj->known = object_new();
        object_set_base_known(player, obj);
        object_flavor_aware(player, obj);
        player_know_object(player, obj);
        if (!bm || !store_carry(bm, obj, false)) { printf("SETUPFAIL store\n"); return 1; }
    }
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=186 mode=%s class=%s clevel=%d gold=%d quiver_slots=%d\n",
           mode, player->class->name, player->lev, (int)player->au,
           (int)borg.trait[BI_QUIVER_SLOTS]);
    printf("STATE weight=%d limit=%d half=%d step=%d encumbered=%d speed_loss=%d\n",
           (int)borg.trait[BI_WEIGHT], carry, half, step,
           borg.trait[BI_WEIGHT] > half ? 1 : 0,
           borg.trait[BI_WEIGHT] > half ? (int)((borg.trait[BI_WEIGHT] - half) / step) : 0);
    printf("STATE wielded=[%s] slot_PACK_SLOTS=%d need_tohit=%d\n",
           borg_items[INVEN_WIELD].desc, PACK_SLOTS,
           (int)borg.trait[BI_NEED_ENCHANT_TO_H]);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_buy(void *state)
{
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty)
            printf("PACK %d qty=%d weight=%d desc=[%s]\n", i, borg_items[i].iqty,
                   borg_items[i].weight, borg_items[i].desc);
    for (int k = 0; k < z_info->store_max - 1; ++k)
        for (int n = 0; n < z_info->store_inven_max; ++n)
            if (borg_shops[k].ware[n].iqty)
                printf("WARE shop=%d ware=%d cost=%d desc=[%s]\n", k, n,
                       (int)borg_shops[k].ware[n].cost, borg_shops[k].ware[n].desc);
    printf("TRACK power_now=%d\n", (int)borg.power);
    /* The production "buy something useful" decision. */
    bool buys = borg_think_shop_buy_useful();
    int shop = buys ? borg.goal.shop : -1, ware = buys ? borg.goal.ware : -1;
    printf("DECISION buys=%d shop=%d ware=%d item=%s\n", buys ? 1 : 0, shop, ware,
           buys ? borg_shops[shop].ware[ware].desc : "(nothing)");
    printf("OUTCOME buys_scroll=%d\n", buys ? 1 : 0);
    /* Correct contract: an enchant scroll the Borg needs is read as soon as
     * it is bought, so its weight must not stop the purchase. */
    require(buys);
    ok;
}

const char *suite_name = "native-c/encumb-186";
struct test tests[] = {
    {"needed_enchant_scroll_is_bought_when_encumbered", scenario_buy},
    {NULL, NULL}};
