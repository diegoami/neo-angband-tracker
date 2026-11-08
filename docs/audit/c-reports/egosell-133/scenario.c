/* Synthetic native C sell-decision fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#133 (egosell-133). */
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
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "project.h"
#include "store.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-item-id.h"
#include "borg/borg-trait.h"
#include "borg/borg-store-sell.h"
#include "borg/borg-store.h"
#include "borg/borg-power.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static struct object *spare_cloak;
static int hidden_elem = -1;

static struct object *new_cloak(const char *ego)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(TV_CLOAK, lookup_sval(TV_CLOAK, "Cloak"));
    if (!kind) quit("fixture cloak kind missing");
    object_prep(obj, kind, 40, RANDOMISE);
    if (ego) {
        obj->ego = lookup_ego_item(ego, kind->tval, kind->sval);
        if (!obj->ego) quit("fixture ego missing");
        ego_apply_magic(obj, 40);
    }
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    return obj;
}

static int random_resist(const struct object *obj)
{
    for (int e = 0; e < ELEM_MAX; e++)
        if (obj->el_info[e].res_level > 0) return e;
    return -1;
}

int setup_tests(void **state)
{
    const char *profile = getenv("ES133_PROFILE");
    const char *mode    = getenv("ES133_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(133);
    if (!player_make_simple("Human", "Warrior", "EgoSell133")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = cave->depth = player->cave->depth = 0;
    player->max_depth = 20;
    player_exp_gain(player, player_exp[19] * player->expfact / 100);
    player->au = 1000;
    character_dungeon = true;
    /* The player stands in the Black Market's entrance. */
    square_set_feat(cave, loc(12, 12), lookup_feat_code("STORE_BLACK"));

    /* Empty the pack (starting food, potions, flasks) so the only thing
     * the Borg could sell is the spare cloak; the starting equipment stays. */
    for (struct object *o = player->gear, *next; o; o = next) {
        next = o->next;
        if (object_is_equipped(player->body, o)) continue;
        bool none_left = false;
        struct object *used = gear_object_for_use(player, o, o->number, false, &none_left);
        if (used->known) object_delete(NULL, NULL, &used->known);
        object_delete(NULL, NULL, &used);
    }

    /* Worn cloak: a plain Cloak ('plain-worn') or a Cloak of Aman
     * ('aman-worn'). Carried spare: an unidentified Cloak of Aman. */
    spare_cloak = new_cloak("of Aman");
    struct object *worn = new_cloak(streq(mode, "aman-worn") ? "of Aman" : NULL);
    hidden_elem = random_resist(spare_cloak);
    if (hidden_elem < 0) { printf("SETUPFAIL no random resist\n"); return 1; }
    inven_carry(player, worn, false, false);
    inven_wield(worn, wield_slot(worn));
    inven_carry(player, spare_cloak, false, false);

    /* Every rune is known except the spare cloak's random high resist, as
     * for a character that has met everything but that element on gear. */
    player_learn_all_runes(player);
    player->obj_k->el_info[hidden_elem].res_level = 0;
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
    borg_cfg[BORG_WORSHIPS_GOLD] = false;
    borg_cfg[BORG_MONEY_SCUM_AMOUNT] = 0;
    /* Every store is empty, so no shop is "full". */
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
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=133 mode=%s class=%s clevel=%d maxdepth=%d no_selling=%d\n",
           mode, player->class->name, player->lev, player->max_depth,
           OPT(player, birth_no_selling) ? 1 : 0);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_sell(void *state)
{
    const char *mode = getenv("ES133_MODE");
    char name[160];
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty)
            printf("PACK %d qty=%d ident=%d needs_id=%d ego=%s desc=[%s]\n", i,
                   borg_items[i].iqty, borg_items[i].ident ? 1 : 0,
                   borg_item_note_needs_id(&borg_items[i]) ? 1 : 0,
                   borg_items[i].ego_idx ? e_info[borg_items[i].ego_idx].name : "-",
                   borg_items[i].desc);
    printf("WORN cloak ego=%s desc=[%s]\n",
           borg_items[INVEN_OUTER].ego_idx ? e_info[borg_items[INVEN_OUTER].ego_idx].name : "-",
           borg_items[INVEN_OUTER].desc);
    printf("TRACK hidden_rune=%s power_now=%d\n", projections[hidden_elem].name,
           (int)borg.power);

    /* The production step 2 of the store logic: sell useless items. */
    bool acts = borg_think_shop_sell_useless();
    bool sells_spare = acts && borg.goal.item >= 0
        && borg_items[borg.goal.item].tval == TV_CLOAK;
    printf("DECISION sell=%d shop=%d item=%d what=[%s]\n", acts ? 1 : 0,
           acts ? borg.goal.shop : -1, acts ? borg.goal.item : -1,
           acts ? borg_items[borg.goal.item].desc : "(nothing)");

    /* Carry out the chosen sale with the real store command. */
    int sold = 0;
    if (sells_spare) {
        struct object *obj = NULL;
        for (struct object *o = player->gear; o; o = o->next)
            if (o == spare_cloak) obj = o;
        uint16_t before = messages_num();
        struct command cmd;
        memset(&cmd, 0, sizeof cmd);
        cmd.code = CMD_SELL;
        cmd_set_arg_item(&cmd, "item", obj);
        cmd_set_arg_number(&cmd, "quantity", 1);
        do_cmd_sell(&cmd);
        uint16_t after = messages_num();
        for (int i = (int)(after - before) - 1; i >= 0; --i)
            printf("TRACK %s\n", message_str((uint16_t)i));
        sold = 1;
        for (struct object *o = player->gear; o; o = o->next)
            if (o == spare_cloak) sold = 0;
    } else {
        object_desc(name, sizeof name, spare_cloak, ODESC_PREFIX | ODESC_FULL, player);
        printf("TRACK still carried: %s\n", name);
    }
    printf("OUTCOME sold_unidentified_aman=%d\n", sold);
    /* Correct contract: an ego cloak whose random power is still unknown is
     * kept, whatever cloak is worn. */
    require(!sold);
    (void)mode;
    ok;
}

const char *suite_name = "native-c/egosell-133";
struct test tests[] = {
    {"keeps_unidentified_random_ego", scenario_sell},
    {NULL, NULL}};
