/* Synthetic native C home swap-trial fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#31 (swaptrial-31). */
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
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "store.h"
#include "ui-menu.h"
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
#include "borg/borg-think.h"
#include "borg/borg-store-sell.h"
#include "borg/borg-power.h"
#include "borg/borg-io.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static const char *mode;

/* Make the object fully known, as after normal play with every rune known. */
static struct object *make_known(struct object *obj)
{
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    player_know_object(player, obj);
    return obj;
}

static struct object *plain_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 0, MINIMISE);
    obj->number = number;
    return obj;
}

static struct object *artifact_item(const char *name)
{
    const struct artifact *art = lookup_artifact_name(name);
    if (!art) quit("fixture artifact missing");
    struct object *obj = object_new();
    object_prep(obj, lookup_kind(art->tval, art->sval), 50, MAXIMISE);
    obj->artifact = art;
    copy_artifact_data(obj, art);
    return obj;
}

static struct object *carry(struct object *obj)
{
    make_known(obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static void wear(struct object *obj)
{
    carry(obj);
    inven_wield(obj, wield_slot(obj));
}

static void on_message(game_event_type type, game_event_data *data, void *user)
{
    if (data && data->message.msg)
        printf("MSG %s\n", data->message.msg);
}

/* Pack slots of the two carried swaps and of the first free slot, as the
 * Borg saw them before the store turn. */
static int wslot = -1, aslot = -1, free_from = -1;

int setup_tests(void **state)
{
    const char *profile = getenv("ST31_PROFILE");
    mode = getenv("ST31_MODE");
    if (!profile || !mode) return 1;
    if (!streq(mode, "swap-held") && !streq(mode, "no-weapon-swap")
        && !streq(mode, "no-candidate")) { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(31);
    if (!player_make_simple("Human", "Warrior", "SwapTrial31")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    /* A level-50 Warrior that has been to depth 60, back in town. */
    player->depth = cave->depth = player->cave->depth = 0;
    player->max_depth = 60;
    player_exp_gain(player, player_exp[48] * player->expfact / 100);
    player->au = 5000;
    character_dungeon = true;
    square_set_feat(cave, loc(12, 12), FEAT_HOME);

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
    /* Worn: a lantern, a long sword, the helm of Dor-Lomin (see invisible,
     * telepathy, the four basic resists), a ring of Free Action, an amulet
     * of Trickery (speed, poison resistance) and metal scale mail.  Nothing worn resists nether, confusion or blindness. */
    {
        struct object *lantern = plain_item(TV_LIGHT, "Lantern", 1);
        lantern->timeout = 2000;
        wear(lantern);
    }
    wear(artifact_item("'Ringil'"));
    wear(artifact_item("of Dor-Lómin"));
    wear(plain_item(TV_RING, "Free Action", 1));
    {
        struct object *amulet = object_new();
        object_prep(amulet, lookup_kind(TV_AMULET, lookup_sval(TV_AMULET, "Trickery")), 50, MAXIMISE);
        wear(amulet);
    }
    wear(plain_item(TV_HARD_ARMOR, "Metal Scale Mail", 1));
    /* The starting kit's torch, soft leather armour and dagger, replaced by
     * the worn gear above, and its food, are dropped. */
    for (struct object *o = player->gear, *next; o; o = next) {
        next = o->next;
        if (object_is_equipped(player->body, o)) continue;
        bool none_left = false;
        struct object *used = gear_object_for_use(player, o, o->number, false, &none_left);
        if (used->known) object_delete(NULL, NULL, &used->known);
        object_delete(NULL, NULL, &used);
    }
    carry(plain_item(TV_FOOD, "Ration of Food", 5));
    carry(plain_item(TV_SCROLL, "Word of Recall", 2));
    carry(plain_item(TV_SCROLL, "Teleportation", 5));
    carry(plain_item(TV_POTION, "Cure Critical Wounds", 5));

    /* Carried swaps: except in 'no-weapon-swap', the quarterstaff of Olorin
     * (resists nether) as the backup weapon; the helm 'Holhenneth' (protects
     * from confusion and blindness) as the backup armour. */
    if (!streq(mode, "no-weapon-swap"))
        carry(artifact_item("of Olórin"));
    carry(artifact_item("'Holhenneth'"));

    player_learn_all_runes(player);
    for (struct object *o = player->gear; o; o = o->next) player_know_object(player, o);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;

    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;

    /* Every store emptied.  'swap-held': the home holds the amulet
     * 'Evenstar' (resists nether, like the quarterstaff) and the long sword
     * 'Elvagil' (a backup-weapon candidate worse than the quarterstaff).
     * 'no-weapon-swap': the long sword only.  'no-candidate': nothing. */
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
    }
    if (!streq(mode, "no-candidate")) {
        struct store *home = NULL;
        for (int i = 0; i < z_info->store_max; i++)
            if (stores[i].feat == FEAT_HOME) home = &stores[i];
        if (!home) { printf("SETUPFAIL home\n"); return 1; }
        struct object *wares[2] = { NULL, NULL };
        if (streq(mode, "swap-held")) {
            wares[0] = artifact_item("'Evenstar'");
            wares[1] = artifact_item("'Elvagil'");
        } else {
            wares[0] = artifact_item("'Elvagil'");
        }
        for (int i = 0; i < 2 && wares[i]; i++) {
            make_known(wares[i]);
            if (!store_carry(home, wares[i], false)) { printf("SETUPFAIL store\n"); return 1; }
        }
        for (struct object *o = home->stock; o; o = o->next) {
            object_touch(player, o);
            player_know_object(player, o);
        }
    }
    /* Fixture intervention ('no-weapon-swap' only): one entry in the Borg's
     * sale history saying the long sword was just stored at home, the
     * fields borg_think_shop_sell() writes for a home sale.  Steps 4 and 5
     * skip a just-stored item; steps 7A and 7B do not read the history. */
    if (streq(mode, "no-weapon-swap")) {
        const struct artifact *art = lookup_artifact_name("'Elvagil'");
        sold_item_tval[0] = art->tval; sold_item_sval[0] = art->sval;
        sold_item_pval[0] = 0; sold_item_store[0] = BORG_HOME;
        sold_item_num = 1; sold_item_nxt = 1;
    }
    event_add_handler(EVENT_MESSAGE, on_message, NULL);

    /* What borg_think() does before a store turn. */
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    wslot = weapon_swap - 1;
    aslot = armour_swap - 1;
    free_from = borg_first_empty_inventory_slot();
    printf("SETUP seed=31 mode=%s class=%s clevel=%d max_depth=%d gold=%d free_from=%d\n",
           mode, player->class->name, player->lev, player->max_depth,
           (int)player->au, free_from);
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

/* The Borg's record of one pack slot against the game's own pack.  A slot
 * whose quantity is zero is an empty slot to every reader, so its stale
 * description is not printed. */
static bool slot_state(const char *what, int i)
{
    if (i < 0) { printf("STATE %s_held=0\n", what); return true; }
    const borg_item *b = &borg_items[i];
    const borg_item *s = &safe_items[i];
    bool same = (b->iqty == s->iqty) && (!b->iqty || streq(b->desc, s->desc));
    printf("STATE %s_slot=%d %s_model_qty=%d model=[%s] %s_game_qty=%d game=[%s] %s_matches_game=%d\n",
           what, i, what, b->iqty, b->iqty ? b->desc : "", what, s->iqty, s->iqty ? s->desc : "",
           what, same ? 1 : 0);
    return same;
}

/* Artifact name of the item the last store command moved, if any. */
static const char *last_artifact;

/* One production store turn in the home, then the queued keys are carried
 * out with the real store command: 1 = stash, 2 = retrieve, 0 = leave. */
static int store_turn(int round)
{
    keycode_t keys[64];
    int nk = 0;
    last_artifact = NULL;
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
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
    if (s->feat != FEAT_HOME) { printf("ACTION not-home\n"); return -1; }
    if (nk >= 2 && (keys[0] == 's' || keys[0] == 'd')) {
        const char *p = strchr(all_letters_nohjkl, (char)keys[1]);
        int idx = p ? (int)(p - all_letters_nohjkl) : -1;
        struct object *obj = idx >= 0 ? player->upkeep->inven[idx] : NULL;
        if (!obj) { printf("ACTION bad-letter\n"); return -1; }
        char name[120];
        object_desc(name, sizeof name, obj, ODESC_PREFIX | ODESC_FULL, player);
        printf("ACTION round=%d stash letter=%c qty=1 from=[%s]\n", round, (char)keys[1], name);
        last_artifact = obj->artifact ? obj->artifact->name : NULL;
        struct command cmd;
        memset(&cmd, 0, sizeof cmd);
        cmd.code = CMD_STASH;
        cmd_set_arg_item(&cmd, "item", obj);
        cmd_set_arg_number(&cmd, "quantity", 1);
        do_cmd_stash(&cmd);
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
        /* The keys carry no quantity: the prompt's default of one is
         * accepted with Enter. */
        printf("ACTION round=%d retrieve ware=%c qty=1 from=[%s]\n", round, (char)keys[0], name);
        last_artifact = obj->artifact ? obj->artifact->name : NULL;
        struct command cmd;
        memset(&cmd, 0, sizeof cmd);
        cmd.code = CMD_RETRIEVE;
        cmd_set_arg_item(&cmd, "item", obj);
        cmd_set_arg_number(&cmd, "quantity", 1);
        do_cmd_retrieve(&cmd);
        return 2;
    }
    printf("ACTION round=%d leave\n", round);
    return 0;
}

static int scenario_swaptrial(void *state)
{
    printf("\n");
    dump("start");
    printf("TRACK weapon_swap=%d weapon_swap_value=%ld armour_swap=%d armour_swap_value=%ld\n",
           weapon_swap, (long)weapon_swap_value, armour_swap, (long)armour_swap_value);

    /* Production store choice, as borg_think_store() makes it in the home:
     * steps 1 to 6 find nothing here, then 7A (backup weapon) and 7B
     * (backup armour) try each home ware in the pack. */
    bool chose = borg_choose_shop();
    printf("DECISION chose=%d goal_shop=%d goal_ware=%d goal_item=%d\n", chose ? 1 : 0,
           borg.goal.shop, borg.goal.ware, borg.goal.item);
    if (chose && borg.goal.shop == BORG_HOME && borg.goal.ware >= 0)
        printf("DECISION ware=[%s]\n", borg_shops[BORG_HOME].ware[borg.goal.ware].desc);

    /* The swap flags the last production borg_notice() of the trials left
     * behind: whether the Borg saw a backup weapon that resists nether. */
    printf("TRACK after_trials weapon_swap_resist_neth=%d\n", weapon_swap_resist_neth ? 1 : 0);

    /* The Borg's pack record right after the decision, before anything
     * re-reads the game's pack. */
    bool ok_w = slot_state("weapon_swap", wslot);
    bool ok_a = slot_state("armour_swap", aslot);
    bool ok_f = slot_state("first_free", free_from);

    /* Carry the choice out and keep going: each round is one production
     * store turn (borg_think_store(), after the pack and stores are re-read
     * as borg_think() does), whose queued keys are carried out with the real
     * store commands, until the Borg leaves or six rounds have run. */
    int got_evenstar = 0, stored_evenstar = 0, got_elvagil = 0;
    for (int r = 1; r <= 6; r++) {
        int a = store_turn(r);
        if (a < 0) break;
        if (a == 2 && last_artifact && streq(last_artifact, "'Evenstar'")) got_evenstar++;
        if (a == 2 && last_artifact && streq(last_artifact, "'Elvagil'")) got_elvagil++;
        if (a == 1 && last_artifact && streq(last_artifact, "'Evenstar'")) stored_evenstar++;
        borg_find_all_items();
        dump("after");
        if (a == 0) break;
        /* A completed store command keeps the Borg in the home. */
        borg.goal.shop = borg.goal.ware = borg.goal.item = -1;
    }
    printf("OUTCOME model_matches_game=%d evenstar_retrieved=%d evenstar_stored=%d elvagil_retrieved=%d\n",
           (ok_w && ok_a && ok_f) ? 1 : 0, got_evenstar, stored_evenstar, got_elvagil);

    /* Correct contract: the store trials leave the Borg's pack record as the
     * game has it, and the Borg does not take a backup amulet whose only
     * high resist, nether, its backup weapon already gives. */
    require(ok_w && ok_a && ok_f);
    require(got_evenstar == 0);
    ok;
}

const char *suite_name = "native-c/swaptrial-31";
struct test tests[] = {
    {"home_swap_trials", scenario_swaptrial},
    {NULL, NULL}};
