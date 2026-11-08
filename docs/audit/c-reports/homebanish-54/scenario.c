/* Synthetic native C home-sell fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#54 (homebanish-54). */
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

int setup_tests(void **state)
{
    const char *profile = getenv("HB54_PROFILE");
    const char *mode    = getenv("HB54_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(54);
    if (!player_make_simple("Human", "Mage", "HomeBanish54")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 0;
    player_exp_gain(player, player_exp[24] * player->expfact / 100);
    player->au = 5000;
    character_dungeon = true;
    struct object *torch = NULL;
    for (struct object *o = player->gear; o; o = o->next)
        if (tval_is_light(o)) { torch = o; break; }
    if (torch) inven_wield(torch, wield_slot(torch));

    /* The pack holds one scroll: Banishment (changed case) or Mass
     * Banishment (control, which the home score already counts). */
    carry(new_item(TV_SCROLL, streq(mode, "banish") ? "Banishment" : "Mass Banishment", 1));
    player_learn_all_runes(player);
    for (struct object *o = player->gear; o; o = o->next) player_know_object(player, o);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;

    /* Every store and the home are emptied. */
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
    printf("SETUP seed=54 mode=%s class=%s clevel=%d gold=%d\n",
           mode, player->class->name, player->lev, (int)player->au);
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
    int32_t best_home_power = 0;
    bool acts = borg_think_home_sell_useful(&best_home_power);
    bool stores_item = acts && borg.goal.item != -1;
    const char *what = stores_item ? borg_items[borg.goal.item].desc : "(nothing)";
    printf("DECISION acts=%d goal_item=%d what=[%s]\n", acts ? 1 : 0, acts ? borg.goal.item : -1, what);
    int stores_scroll = stores_item && strstr(what, "Banishment") ? 1 : 0;
    printf("OUTCOME stores_banishment_scroll=%d\n", stores_scroll);
    require(stores_scroll); /* fails before for the plain Banishment scroll */
    ok;
}

const char *suite_name = "native-c/homebanish-54";
struct test tests[] = {
    {"home_stores_a_banishment_scroll", scenario_home},
    {NULL, NULL}};
