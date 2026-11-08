/* Synthetic native C home-score fixture, excluded from the source change.
 * Issue: diegoami/neo-angband-tracker#47 (neckdead-47). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
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
#include "borg/borg-store-sell.h"
#include "borg/borg-home-notice.h"
#include "borg/borg-home-power.h"
#include "borg/borg-cave.h"
#include "borg/borg-power.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static int home_amulets = 0;

/* Distinct amulet kinds, so the duplicate check does not enter the score. */
static const char *home_kinds[] = {
    "Resist Lightning", "Resist Acid", "Regeneration", "Infravision", "Resistance"};

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

int setup_tests(void **state)
{
    const char *profile = getenv("ND47_PROFILE");
    const char *mode    = getenv("ND47_MODE");
    if (!profile || !mode) return 1;
    if (streq(mode, "neck2")) home_amulets = 2;
    else if (streq(mode, "neck3")) home_amulets = 3;
    else if (streq(mode, "neck4")) home_amulets = 4;
    else if (streq(mode, "neck5")) home_amulets = 5;
    else { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(47);
    if (!player_make_simple("Human", "Warrior", "NeckDead47")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    player_exp_gain(player, player_exp[19] * player->expfact / 100);
    player->au = 1000;
    character_dungeon = true;
    struct object *torch = NULL;
    for (struct object *o = player->gear; o; o = o->next)
        if (tval_is_light(o)) { torch = o; break; }
    if (torch) inven_wield(torch, wield_slot(torch));

    /* The pack carries one Amulet of Slow Digestion, a candidate for the
     * home. */
    {
        struct object *obj = new_item(TV_AMULET, "Slow Digestion", 1);
        make_known(obj);
        inven_carry(player, obj, false, false);
        object_touch(player, obj);
        player_know_object(player, obj);
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

    /* Every store is emptied; the home then holds home_amulets distinct
     * amulets. */
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
        for (int n = 0; n < home_amulets; n++) {
            struct object *obj = new_item(TV_AMULET, home_kinds[n], 1);
            make_known(obj);
            if (!store_carry(home, obj, false)) { printf("SETUPFAIL store\n"); return 1; }
        }
    }
    borg_find_all_items();
    borg_notice(true);
    borg.power = borg_power();
    borg.goal.ware = -1; borg.goal.shop = -1; borg.goal.item = -1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=47 mode=%s class=%s clevel=%d home_amulets=%d\n",
           mode, player->class->name, player->lev, home_amulets);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_home(void *state)
{
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty)
            printf("PACK %d qty=%d desc=[%s]\n", i, borg_items[i].iqty, borg_items[i].desc);
    for (int n = 0; n < z_info->store_inven_max; ++n)
        if (borg_shops[BORG_HOME].ware[n].iqty)
            printf("HOME %d qty=%d desc=[%s]\n", n, borg_shops[BORG_HOME].ware[n].iqty,
                   borg_shops[BORG_HOME].ware[n].desc);
    /* The production home score of the home as it stands. */
    borg_notice_home(NULL, false);
    int16_t necks = num_neck;
    int32_t home_power = borg_power_home();
    printf("STATE num_neck=%d home_power=%d\n", necks, (int)home_power);
    /* The production step 1 of the store logic: sell useful things to the
     * home (here: whether to store the spare amulet). */
    int32_t best_home_power = 0;
    bool acts = borg_think_home_sell_useful(&best_home_power);
    bool stores_item = acts && borg.goal.item != -1;
    bool takes_ware = acts && borg.goal.item == -1;
    printf("DECISION acts=%d best_home_power=%d goal_item=%d goal_ware=%d what=[%s]\n",
           acts ? 1 : 0, (int)best_home_power, acts ? borg.goal.item : -1,
           takes_ware ? borg.goal.ware : -1,
           stores_item ? borg_items[borg.goal.item].desc
           : takes_ware ? borg_shops[BORG_HOME].ware[borg.goal.ware].desc : "(nothing)");
    printf("OUTCOME num_neck=%d home_power=%d stores_amulet=%d\n", necks,
           (int)home_power, stores_item ? 1 : 0);
    require(necks == home_amulets);
    ok;
}

const char *suite_name = "native-c/neckdead-47";
struct test tests[] = {
    {"home_amulet_score", scenario_home},
    {NULL, NULL}};
