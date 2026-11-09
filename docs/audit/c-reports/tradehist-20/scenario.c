/* Synthetic native C home-trading fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#20 (tradehist-20). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "game-event.h"
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
#include "ui-menu.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include "borg/borg-store.h"
#include "borg/borg-store-buy.h"
#include "borg/borg-store-sell.h"
#include "borg/borg-think-store.h"
#include "borg/borg-think.h"
#include "borg/borg-power.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static const char *mode;

static void on_message(game_event_type type, game_event_data *data, void *user)
{
    if (data && data->message.msg)
        printf("MSG %s\n", data->message.msg);
}

int setup_tests(void **state)
{
    const char *profile = getenv("TH20_PROFILE");
    mode = getenv("TH20_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(20);
    if (!player_make_simple("Human", "Warrior", "TradeHist20")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = cave->depth = player->cave->depth = 0;
    player->max_depth = 10;
    player_exp_gain(player, player_exp[4] * player->expfact / 100);
    player->au = 3000;
    character_dungeon = true;
    square_set_feat(cave, loc(12, 12), FEAT_HOME);

    struct object *torch = NULL;
    for (struct object *o = player->gear; o; o = o->next)
        if (tval_is_light(o)) { torch = o; break; }
    if (torch) inven_wield(torch, wield_slot(torch));
    /* Fixture kit: the starting Warrior kit with its ration stack raised to
     * eleven.  'rations-only': the scroll of Word of Recall and the potion
     * are gone, as after recalling to town and drinking the potion.
     * 'with-recall': the starting scroll and potion are kept. */
    for (struct object *o = player->gear; o; o = o->next)
        if (tval_is_food(o)) o->number = 11;
    if (streq(mode, "rations-only")) {
        for (struct object *o = player->gear, *next; o; o = next) {
            next = o->next;
            if (tval_is_scroll(o) || tval_is_potion(o)) {
                bool none_left = false;
                struct object *used = gear_object_for_use(player, o, o->number, false, &none_left);
                if (used->known) object_delete(NULL, NULL, &used->known);
                object_delete(NULL, NULL, &used);
            }
        }
    }
    player_learn_all_runes(player);
    for (struct object *o = player->gear; o; o = o->next) {
        object_touch(player, o);
        player_know_object(player, o);
    }
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    borg_cfg[BORG_MONEY_SCUM_AMOUNT] = 0;
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
    }
    event_add_handler(EVENT_MESSAGE, on_message, NULL);
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=20 mode=%s class=%s clevel=%d gold=%d\n",
           mode, player->class->name, player->lev, (int)player->au);
    return 0;
}

int teardown_tests(void *state)
{
    event_remove_handler(EVENT_MESSAGE, on_message, NULL);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static void refresh(void)
{
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
}

static void dump(void)
{
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty)
            printf("PACK %d qty=%d desc=[%s]\n", i, borg_items[i].iqty, borg_items[i].desc);
    for (int k = 0; k < z_info->store_max; ++k)
        for (int n = 0; n < z_info->store_inven_max; ++n)
            if (borg_shops[k].ware[n].iqty)
                printf("SHOP %d ware %d qty=%d desc=[%s]\n", k, n,
                       borg_shops[k].ware[n].iqty, borg_shops[k].ware[n].desc);
    printf("TRACK power=%d gold=%d\n", (int)borg.power, (int)player->au);
}

/* One production store turn at the store the player stands in, then the
 * queued keys are carried out with the real store commands. */
static int store_turn(int round)
{
    keycode_t keys[64];
    int nk = 0;
    refresh();
    borg_flush();
    borg_think_store();
    while (nk < 64) {
        keycode_t k = borg_inkey(true);
        if (!k) break;
        keys[nk++] = k;
    }
    printf("KEYS round=%d store=%d n=%d", round, shop_num, nk);
    for (int i = 0; i < nk; i++) printf(" %d", (int)keys[i]);
    printf("\n");
    struct store *s = &stores[shop_num];
    if (nk >= 2 && keys[0] == 's') {
        const char *p = strchr(all_letters_nohjkl, (char)keys[1]);
        int idx = p ? (int)(p - all_letters_nohjkl) : -1;
        int qty = 1;
        if (nk >= 3 && keys[2] >= '2' && keys[2] <= '5') qty = keys[2] - '0';
        struct object *obj = idx >= 0 ? player->upkeep->inven[idx] : NULL;
        if (!obj) { printf("ACTION bad-letter\n"); return -1; }
        char name[120];
        object_desc(name, sizeof name, obj, ODESC_PREFIX | ODESC_FULL, player);
        printf("ACTION %s letter=%c qty=%d from=[%s]\n",
               s->feat == FEAT_HOME ? "stash" : "sell", (char)keys[1], qty, name);
        struct command cmd;
        memset(&cmd, 0, sizeof cmd);
        cmd.code = s->feat == FEAT_HOME ? CMD_STASH : CMD_SELL;
        cmd_set_arg_item(&cmd, "item", obj);
        cmd_set_arg_number(&cmd, "quantity", qty);
        if (s->feat == FEAT_HOME) do_cmd_stash(&cmd); else do_cmd_sell(&cmd);
        return 1;
    }
    if (nk >= 2 && keys[1] == 'p') {
        const char *p = strchr(SHOP_MENU_ITEMS, (char)keys[0]);
        int ware = p ? (int)(p - SHOP_MENU_ITEMS) : -1;
        struct object **list = mem_zalloc(sizeof(struct object *) * z_info->store_inven_max);
        store_stock_list(s, list, z_info->store_inven_max);
        struct object *obj = ware >= 0 ? list[ware] : NULL;
        mem_free(list);
        if (!obj) { printf("ACTION bad-ware\n"); return -1; }
        char name[120];
        object_desc(name, sizeof name, obj, ODESC_PREFIX | ODESC_FULL, player);
        /* The keys carry no quantity: the store prompt's default of one
         * is accepted with Enter. */
        printf("ACTION %s ware=%c qty=1 from=[%s]\n",
               s->feat == FEAT_HOME ? "retrieve" : "buy", (char)keys[0], name);
        struct command cmd;
        memset(&cmd, 0, sizeof cmd);
        cmd.code = s->feat == FEAT_HOME ? CMD_RETRIEVE : CMD_BUY;
        cmd_set_arg_item(&cmd, "item", obj);
        cmd_set_arg_number(&cmd, "quantity", 1);
        if (s->feat == FEAT_HOME) do_cmd_retrieve(&cmd); else do_cmd_buy(&cmd);
        return 2;
    }
    printf("ACTION leave\n");
    return 0;
}

static int scenario_trade(void *state)
{
    int stashed_food = 0, retrieved_food = 0;
    refresh();
    dump();
    for (int r = 1; r <= 4; r++) {
        int food_before = 0, food_after = 0;
        for (struct object *o = player->gear; o; o = o->next)
            if (tval_is_food(o)) food_before += o->number;
        int a = store_turn(r);
        for (struct object *o = player->gear; o; o = o->next)
            if (tval_is_food(o)) food_after += o->number;
        if (a == 1 && food_after < food_before) stashed_food += food_before - food_after;
        if (a == 2 && food_after > food_before) retrieved_food += food_after - food_before;
        refresh();
        dump();
        if (a <= 0) {
            /* Outside the store the Borg picks its next destination. */
            bool chose = borg_choose_shop();
            printf("NEXT chose=%d goal_shop=%d goal_item=%d goal_ware=%d\n", chose ? 1 : 0,
                   borg.goal.shop, borg.goal.item, borg.goal.ware);
            break;
        }
    }
    printf("OUTCOME stashed_rations=%d retrieved_rations=%d\n", stashed_food, retrieved_food);
    /* Correct contract: rations the Borg has just stored at home are not
     * taken straight back out on the next store turn. */
    require(stashed_food > 0);
    require(retrieved_food == 0);
    ok;
}

const char *suite_name = "native-c/tradehist-20";
struct test tests[] = {
    {"home_trade_history", scenario_trade},
    {NULL, NULL}};
