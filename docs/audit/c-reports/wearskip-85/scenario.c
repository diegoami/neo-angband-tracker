/* Synthetic native C home best-equipment fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#85 (wearskip-85). */
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
#include "obj-desc.h"
#include "obj-util.h"
#include "player.h"
#include "store.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "ui-menu.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-item-wear.h"
#include "borg/borg-trait.h"
#include "borg/borg-store.h"
#include "borg/borg-think.h"
#include "borg/borg-io.h"
#include "borg/borg-power.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

/* Make the object fully known to the player and put it in the pack. */
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

static void wear(struct object *obj)
{
    inven_wield(obj, wield_slot(obj));
}

/* Distinct pack fillers that the Borg never wears. */
static const char *fillers[] = { "Cure Light Wounds", "Cure Serious Wounds",
    "Neutralize Poison", "Boldness", "Speed", "Heroism", "Berserk Strength",
    "Resist Heat", "Resist Cold", "Resist Poison", "Infravision",
    "True Seeing", "Restore Mana", "Surprise", "Sleep", "Blindness",
    "Confusion", "Salt Water", "Dragon Breath", "Light", NULL };

int setup_tests(void **state)
{
    const char *profile = getenv("WS85_PROFILE");
    const char *mode    = getenv("WS85_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(85);
    if (!player_make_simple("Human", "Warrior", "WearSkip85")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    player_exp_gain(player, player_exp[19] * player->expfact / 100);
    player->au = 0;
    character_dungeon = true;
    /* Discard the starting kit so the pack layout is exact. */
    for (int i = 0; i < player->body.count; ++i) player->body.slots[i].obj = NULL;
    object_pile_free(NULL, NULL, player->gear_k);
    object_pile_free(NULL, NULL, player->gear);
    player->gear = player->gear_k = NULL;
    player->upkeep->total_weight = 0;
    player->upkeep->equip_cnt = 0;
    calc_inventory(player);

    /* Worn: a Wooden Torch, a Dagger and Soft Leather Armour; no cloak. */
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    wear(torch);
    wear(known_item(TV_SWORD, "Dagger", 1));
    wear(known_item(TV_SOFT_ARMOR, "Soft Leather Armour", 1));

    /* Pack upgrades: Tulwars (two identical ones stacked in one slot,
     * except in 'roomy'), Metal Scale Mail and, except in 'nocloak', a
     * Cloak. */
    known_item(TV_SWORD, "Tulwar", streq(mode, "roomy") ? 1 : 2);
    known_item(TV_HARD_ARMOR, "Metal Scale Mail", 1);
    if (!streq(mode, "nocloak")) known_item(TV_CLOAK, "Cloak", 1);
    known_item(TV_FOOD, "Ration of Food", 5);

    /* Fill the pack with distinct potions until exactly one slot is free
     * ('roomy': two slots free). */
    int want_free = streq(mode, "roomy") ? 2 : 1;
    for (int i = 0; fillers[i] && pack_slots_used(player) < z_info->pack_size - want_free; ++i)
        known_item(TV_POTION, fillers[i], 1);
    if (pack_slots_used(player) != z_info->pack_size - want_free) {
        printf("SETUPFAIL filler\n"); return 1;
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

    /* Every store is emptied, including the home. */
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
    }
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    /* As on arriving in town (borg_update), start a fresh best-gear plan. */
    borg_clear_best();
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=85 mode=%s class=%s clevel=%d pack_used=%d/%d\n",
           mode, player->class->name, player->lev, pack_slots_used(player),
           z_info->pack_size);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int letter_index(char c)
{
    for (int i = 0; all_letters_nohjkl[i]; ++i)
        if (all_letters_nohjkl[i] == c) return i;
    return -1;
}

static void print_plan(void)
{
    if (!borg.goal.best_item) { printf("PLAN none do_best=%d\n", borg.goal.do_best ? 1 : 0); return; }
    printf("PLAN do_best=%d", borg.goal.do_best ? 1 : 0);
    for (int k = 0; k < z_info->equip_slots_max; ++k)
        if (borg.goal.best_item[k].tval)
            printf(" [%d:%s%d/%d]", k, borg.goal.best_item[k].home ? "home:" : "",
                   borg.goal.best_item[k].tval, borg.goal.best_item[k].sval);
    printf("\n");
}

static int body_obj(const char *slot_name, char *buf, size_t n)
{
    int s = slot_by_name(player, slot_name);
    struct object *o = s >= 0 ? player->body.slots[s].obj : NULL;
    if (!o) { my_strcpy(buf, "(nothing)", n); return 0; }
    object_desc(buf, n, o, ODESC_PREFIX | ODESC_BASE, player);
    return 1;
}

static int scenario_best(void *state)
{
    const char *mode = getenv("WS85_MODE");
    int steps = 0, acted_steps = 0;
    shop_num = BORG_HOME;
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty)
            printf("PACK %c qty=%d desc=[%s]\n", all_letters_nohjkl[i],
                   borg_items[i].iqty, borg_items[i].desc);
    for (int i = INVEN_WIELD; i < INVEN_TOTAL; ++i)
        if (borg_items[i].iqty)
            printf("WORN %c desc=[%s]\n", all_letters_nohjkl[i - INVEN_WIELD],
                   borg_items[i].desc);
    printf("TRACK power_start=%d\n", (int)borg.power);

    /* Repeat the production first step of the store logic (wear the
     * optimal equipment) until it declines to act, carrying out each
     * queued command with the game's own wield and take-off. */
    for (steps = 1; steps <= 12; ++steps) {
        char keys[64];
        int nkeys = 0;
        borg_flush();
        bool acts = borg_best_stuff();
        for (keycode_t k; nkeys < 63 && (k = borg_inkey(true)) != 0; )
            keys[nkeys++] = (k == KC_ENTER) ? '$' : (char)k;
        keys[nkeys] = '\0';
        printf("STEP %d acts=%d keys=[%s] full=%d\n", steps, acts ? 1 : 0, keys,
               borg_inventory_full() ? 1 : 0);
        print_plan();
        if (!acts) break;
        if (nkeys != 2 || (keys[0] != 'w' && keys[0] != 't')) {
            printf("UNEXPECTED keys\n");
            break;
        }
        int idx = letter_index(keys[1]);
        char name[120] = "?";
        uint16_t pre = messages_num();
        if (keys[0] == 't') {
            struct object *o = (idx >= 0 && idx < player->body.count) ? player->body.slots[idx].obj : NULL;
            if (!o || o->kind->tval != borg_items[INVEN_WIELD + idx].tval
                || o->kind->sval != borg_items[INVEN_WIELD + idx].sval) {
                printf("UNEXPECTED takeoff target\n");
                break;
            }
            object_desc(name, sizeof(name), o, ODESC_PREFIX | ODESC_BASE, player);
            inven_takeoff(o);
        } else {
            struct object *o = (idx >= 0 && idx < z_info->pack_size) ? player->upkeep->inven[idx] : NULL;
            if (!o || o->kind->tval != borg_items[idx].tval
                || o->kind->sval != borg_items[idx].sval) {
                printf("UNEXPECTED wield target\n");
                break;
            }
            object_desc(name, sizeof(name), o, ODESC_PREFIX | ODESC_BASE, player);
            inven_wield(o, wield_slot(o));
        }
        calc_inventory(player);
        player->upkeep->update |= PU_BONUS | PU_INVEN;
        update_stuff(player);
        uint16_t post = messages_num();
        printf("DECISION step=%d cmd=%c item=[%s]\n", steps, keys[0], name);
        for (int i = (int)(post - pre) - 1; i >= 0; --i)
            printf("ENGINE %s\n", message_str((uint16_t)i));
        ++acted_steps;
        /* Re-read gear as the Borg does after each command. */
        borg_find_all_items();
        borg_notice(true);
        borg.power = borg_power();
    }
    char weapon[80], body[80], cloak[80];
    body_obj("weapon", weapon, sizeof weapon);
    int has_body = body_obj("body", body, sizeof body);
    int has_cloak = body_obj("back", cloak, sizeof cloak);
    int scale = has_body && strstr(body, "Metal Scale Mail") != NULL;
    printf("TRACK power_end=%d\n", (int)borg.power);
    printf("STATE weapon=[%s] body=[%s] cloak=[%s] pack_used=%d\n", weapon, body,
           cloak, pack_slots_used(player));
    printf("OUTCOME actions=%d wears_scale_mail=%d wears_cloak=%d\n", acted_steps,
           scale, has_cloak);
    /* Correct contract: every planned change that can be made is made. */
    if (streq(mode, "nocloak"))
        require(!scale && !has_cloak); /* no room anywhere: nothing more to do */
    else
        require(scale && has_cloak);   /* fails before for 'tight' */
    ok;
}

const char *suite_name = "native-c/wearskip-85";
struct test tests[] = {
    {"home_wears_every_reachable_upgrade", scenario_best},
    {NULL, NULL}};
