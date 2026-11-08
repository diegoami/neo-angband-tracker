/* Synthetic native C sale-order fixture, excluded from the source change.
 * Issue: diegoami/neo-angband-tracker#70 (modcredit-70). */
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
#include "obj-power.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "store.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include "borg/borg-store-sell.h"
#include "borg/borg-store.h"
#include "borg/borg-power.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
/* The two rings carried for sale; 'first' is the one the corrected
 * valuation should sell first. */
static struct object *ring_a, *ring_b;
static const char *name_a, *name_b;

static struct object *carry_item(const char *spec)
{
    struct object *obj = object_new();
    if (!strncmp(spec, "art:", 4)) {
        /* A standard artifact, made as the game makes one. */
        const struct artifact *art = lookup_artifact_name(spec + 4);
        if (!art) quit("fixture artifact missing");
        struct object_kind *kind = lookup_kind(art->tval, art->sval);
        object_prep(obj, kind, art->alloc_min, RANDOMISE);
        obj->artifact = art;
        copy_artifact_data(obj, art);
        mark_artifact_created(art, true);
    } else {
        int tval = !strncmp(spec, "amulet:", 7) ? TV_AMULET : TV_RING;
        const char *name = strchr(spec, ':') + 1;
        struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
        if (!kind) { fprintf(stderr, "MISSING KIND %s\n", spec); quit("fixture item kind missing"); }
        /* Level 40, average rolls: a Ring of Strength gets +2 (1 + 5*40/128). */
        object_prep(obj, kind, 40, AVERAGE);
    }
    obj->number = 1;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    return obj;
}

int setup_tests(void **state)
{
    const char *profile = getenv("MC70_PROFILE");
    const char *mode    = getenv("MC70_MODE");
    if (!profile || !mode) return 1;
    if (streq(mode, "statring")) { name_a = "ring:Strength"; name_b = "ring:Soulkeeping"; }
    else if (streq(mode, "speed")) { name_a = "art:Colannon"; name_b = "amulet:ESP"; }
    else if (streq(mode, "nomods")) { name_a = "ring:Free Action"; name_b = "ring:Bodykeeping"; }
    else { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(70);
    if (!player_make_simple("Human", "Warrior", "ModCredit70")) { printf("SETUPFAIL birth\n"); return 1; }
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

    /* Empty the pack (starting food, potions, flasks) so the only things
     * the Borg could sell are the two rings; the starting equipment stays. */
    for (struct object *o = player->gear, *next; o; o = next) {
        next = o->next;
        if (object_is_equipped(player->body, o)) continue;
        bool none_left = false;
        struct object *used = gear_object_for_use(player, o, o->number, false, &none_left);
        if (used->known) object_delete(NULL, NULL, &used->known);
        object_delete(NULL, NULL, &used);
    }
    ring_a = carry_item(name_a);
    ring_b = carry_item(name_b);
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
    printf("SETUP seed=70 mode=%s class=%s clevel=%d maxdepth=%d no_selling=%d\n",
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
    char name[160];
    int value_a = -1, value_b = -1;
    for (int i = 0; i < z_info->pack_size; ++i) {
        borg_item *item = &borg_items[i];
        if (!item->iqty) continue;
        printf("PACK %d qty=%d borg_value=%d pval=%d str=%d speed=%d desc=[%s]\n", i, item->iqty,
               (int)item->value, item->pval, item->modifiers[OBJ_MOD_STR],
               item->modifiers[OBJ_MOD_SPEED], item->desc);
        if (player->upkeep->inven[i] == ring_a) value_a = (int)item->value;
        if (player->upkeep->inven[i] == ring_b) value_b = (int)item->value;
    }
    object_desc(name, sizeof name, ring_a, ODESC_PREFIX | ODESC_FULL, player);
    printf("TRACK game_value=%d kind_cost=%d %s\n", object_value_real(ring_a, 1),
           ring_a->kind->cost, name);
    object_desc(name, sizeof name, ring_b, ODESC_PREFIX | ODESC_FULL, player);
    printf("TRACK game_value=%d kind_cost=%d %s\n", object_value_real(ring_b, 1),
           ring_b->kind->cost, name);
    printf("STATE value_a=%d value_b=%d power_now=%d\n", value_a, value_b, (int)borg.power);

    /* The production step 2 of the store logic: sell useless items, the
     * cheapest (by the Borg's guessed value, capped at 30000) first. */
    bool acts = borg_think_shop_sell_useless();
    printf("DECISION sell=%d shop=%d item=%d what=[%s]\n", acts ? 1 : 0,
           acts ? borg.goal.shop : -1, acts ? borg.goal.item : -1,
           acts ? borg_items[borg.goal.item].desc : "(nothing)");
    if (!acts) { printf("OUTCOME sold_a=0 sold_b=0\n"); return 1; }

    /* Carry out the chosen sale with the real store command, standing in
     * the chosen shop. */
    struct object *obj = player->upkeep->inven[borg.goal.item];
    if (!obj) { printf("OUTCOME sold_a=0 sold_b=0\n"); return 1; }
    square_set_feat(cave, player->grid, stores[borg.goal.shop].feat);
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
    int kept_a = 0, kept_b = 0;
    for (struct object *o = player->gear; o; o = o->next) {
        if (o == ring_a) kept_a = 1;
        if (o == ring_b) kept_b = 1;
    }
    printf("OUTCOME sold_a=%d sold_b=%d\n", kept_a ? 0 : 1, kept_b ? 0 : 1);
    /* Correct contract: the ring with the modifier bonus (strength, speed)
     * is valued with that bonus, so the other ring is the cheaper one and
     * is sold first.  With no modifiers at all the cheaper kind (Ring of
     * Bodykeeping) is sold first in both versions. */
    require(kept_a && !kept_b);
    ok;
}

const char *suite_name = "native-c/modcredit-70";
struct test tests[] = {
    {"sells_cheaper_ring_first", scenario_sell},
    {NULL, NULL}};
