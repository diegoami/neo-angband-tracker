/* Synthetic native C home-grab fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#29 (homeskip-29). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "game-event.h"
#include "obj-desc.h"
#include "ui-menu.h"
#include "borg/borg-think.h"
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
#include "borg/borg-store-sell.h"
#include "borg/borg-home-notice.h"
#include "borg/borg-home-power.h"
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


static const char *mode;

/* Home contents per mode, carried to the home in this order (the home
 * sorts them; potions before scrolls before diggers).  'recent' names the
 * kind the Borg's sale history records as just stored at home, or NULL. */
struct ware_def { int tval; const char *name; };
struct mode_def {
    const char *name;
    struct ware_def wares[2];
    struct ware_def recent;
};
static const struct mode_def modes[] = {
    {"stored-first", {{TV_POTION, "Cure Critical Wounds"}, {TV_SCROLL, "Light"}},
     {TV_POTION, "Cure Critical Wounds"}},
    {"stored-last", {{TV_SCROLL, "Light"}, {TV_DIGGING, "Shovel"}},
     {TV_DIGGING, "Shovel"}},
    {"no-history", {{TV_POTION, "Infravision"}, {TV_SCROLL, "Light"}},
     {0, NULL}},
};
static const struct mode_def *md;

static void on_message(game_event_type type, game_event_data *data, void *user)
{
    if (data && data->message.msg)
        printf("MSG %s\n", data->message.msg);
    else if (messages_num())
        printf("NOTE %s\n", message_str(0));
}

int setup_tests(void **state)
{
    const char *profile = getenv("HS29_PROFILE");
    mode = getenv("HS29_MODE");
    if (!profile || !mode) return 1;
    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
        if (streq(modes[i].name, mode)) md = &modes[i];
    if (!md) { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(29);
    if (!player_make_simple("Human", "Warrior", "HomeSkip29")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = cave->depth = player->cave->depth = 0;
    player->max_depth = 10;
    player_exp_gain(player, player_exp[9] * player->expfact / 100);
    player->au = 3000;
    character_dungeon = true;
    square_set_feat(cave, loc(12, 12), FEAT_HOME);
    {
        struct object *torch = NULL;
        for (struct object *o = player->gear; o; o = o->next)
            if (tval_is_light(o) && !object_is_equipped(player->body, o)) { torch = o; break; }
        if (torch) inven_wield(torch, wield_slot(torch));
    }
    /* The starting Word of Recall scroll and Berserk Strength potion are
     * removed, as after recalling to town and drinking the potion. */
    for (struct object *o = player->gear, *next; o; o = next) {
        next = o->next;
        if (tval_is_scroll(o) || tval_is_potion(o)) {
            bool none_left = false;
            struct object *used = gear_object_for_use(player, o, o->number, false, &none_left);
            if (used->known) object_delete(NULL, NULL, &used->known);
            object_delete(NULL, NULL, &used);
        }
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
        for (int i = 0; i < 2; i++) {
            struct object *obj = new_item(md->wares[i].tval, md->wares[i].name, 1);
            make_known(obj);
            if (!store_carry(home, obj, false)) { printf("SETUPFAIL store\n"); return 1; }
        }
    }
    /* Fixture intervention: one entry in the Borg's sale history saying the
     * 'recent' kind was just stored at home, the fields borg_think_shop_sell()
     * writes for a home sale.  The count of one is what the history holds
     * after one recorded sale with the counter counting entries (and after
     * two sales with the original counter, which leaves out the newest). */
    if (md->recent.name) {
        struct object_kind *k = lookup_kind(md->recent.tval,
            lookup_sval(md->recent.tval, md->recent.name));
        sold_item_tval[0] = k->tval; sold_item_sval[0] = k->sval;
        sold_item_pval[0] = 0; sold_item_store[0] = BORG_HOME;
        sold_item_num = 1; sold_item_nxt = 1;
    }
    event_add_handler(EVENT_MESSAGE, on_message, NULL);
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=29 mode=%s class=%s clevel=%d gold=%d free_from=%d\n",
           mode, player->class->name, player->lev, (int)player->au,
           borg_first_empty_inventory_slot());
    return 0;
}

int teardown_tests(void *state)
{
    event_remove_handler(EVENT_MESSAGE, on_message, NULL);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static void dump(const char *when)
{
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty)
            printf("PACK %s %d qty=%d desc=[%s]\n", when, i, borg_items[i].iqty, borg_items[i].desc);
    for (int n = 0; n < z_info->store_inven_max; ++n)
        if (borg_shops[BORG_HOME].ware[n].iqty)
            printf("HOME %s %d qty=%d desc=[%s]\n", when, n, borg_shops[BORG_HOME].ware[n].iqty,
                   borg_shops[BORG_HOME].ware[n].desc);
}

static int scenario_grab(void *state)
{
    printf("\n");
    for (int p = 0; p < sold_item_num; p++)
        printf("HISTORY %d tval=%d sval=%d store=%d\n", p, sold_item_tval[p],
               sold_item_sval[p], sold_item_store[p]);
    dump("start");
    /* Home power with each ware removed, as step 5 computes it. */
    borg_notice_home(NULL, false);
    printf("TRACK home_power=%d\n", (int)borg_power_home());
    for (int n = 0; n < z_info->store_inven_max; ++n) {
        borg_item *w = &borg_shops[BORG_HOME].ware[n];
        if (!w->iqty) continue;
        int q = w->iqty; w->iqty = 0;
        borg_notice_home(NULL, false);
        printf("TRACK without_ware%d home_power=%d\n", n, (int)borg_power_home());
        w->iqty = q;
    }
    borg_notice_home(NULL, false);

    /* Production store choice, as borg_think_store() makes it in town. */
    bool chose = borg_choose_shop();
    printf("DECISION chose=%d goal_shop=%d goal_ware=%d goal_item=%d\n", chose ? 1 : 0,
           borg.goal.shop, borg.goal.ware, borg.goal.item);
    int want = -1;
    for (int n = 0; n < 2; n++)
        if (borg_shops[BORG_HOME].ware[n].tval == TV_SCROLL) want = n;
    bool grabs = chose && borg.goal.shop == BORG_HOME && borg.goal.ware == want
                 && borg.goal.item == -1;

    /* Carry the choice out: the next production store turn in the home
     * queues the keys, and the matching real retrieve command runs. */
    int scrolls_taken = 0;
    if (grabs) {
        keycode_t keys[64];
        int nk = 0;
        borg_flush();
        borg_think_store();
        while (nk < 64) {
            keycode_t k = borg_inkey(true);
            if (!k) break;
            keys[nk++] = k;
        }
        printf("KEYS store=%d n=%d", shop_num, nk);
        for (int i = 0; i < nk; i++) printf(" %d", (int)keys[i]);
        printf("\n");
        struct store *s = &stores[shop_num];
        if (nk >= 2 && keys[1] == 'p' && s->feat == FEAT_HOME) {
            const char *p = strchr(SHOP_MENU_ITEMS, (char)keys[0]);
            int ware = p ? (int)(p - SHOP_MENU_ITEMS) : -1;
            struct object **list = mem_zalloc(sizeof(struct object *) * z_info->store_inven_max);
            store_stock_list(s, list, z_info->store_inven_max);
            struct object *obj = ware >= 0 ? list[ware] : NULL;
            mem_free(list);
            if (obj) {
                char name[120];
                object_desc(name, sizeof name, obj, ODESC_PREFIX | ODESC_FULL, player);
                printf("ACTION retrieve ware=%c qty=1 from=[%s]\n", (char)keys[0], name);
                struct command cmd;
                memset(&cmd, 0, sizeof cmd);
                cmd.code = CMD_RETRIEVE;
                cmd_set_arg_item(&cmd, "item", obj);
                cmd_set_arg_number(&cmd, "quantity", 1);
                do_cmd_retrieve(&cmd);
            }
        }
        for (struct object *o = player->gear; o; o = o->next)
            if (tval_is_scroll(o)) scrolls_taken += o->number;
        borg_find_all_items();
        dump("end");
    }
    printf("OUTCOME grabs_scroll_of_light=%d scrolls_in_pack=%d\n", grabs ? 1 : 0, scrolls_taken);
    /* Correct contract: a Scroll of Light, which adds nothing to the home,
     * is taken from the home to be sold, whatever sits before it. */
    require(grabs);
    require(scrolls_taken == 1);
    ok;
}

const char *suite_name = "native-c/homeskip-29";
struct test tests[] = {
    {"home_grab_useless", scenario_grab},
    {NULL, NULL}};
