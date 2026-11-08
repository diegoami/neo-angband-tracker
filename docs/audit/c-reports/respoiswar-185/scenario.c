/* Synthetic native C warrior Resist Poison potion reward fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#185 (respoiswar-185). */
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
#include "borg/borg-prepared.h"
#include "borg/borg-trait.h"
#include "borg/borg-junk.h"
#include "borg/borg-io.h"
#include "borg/borg-power.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

/* Make the object fully known to the player, as after normal play with
 * every rune learned. */
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

static struct object *ego_item(int tval, const char *name, const char *ego)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture ego kind missing");
    object_prep(obj, kind, 50, MAXIMISE);
    obj->ego = lookup_ego_item(ego, kind->tval, kind->sval);
    if (!obj->ego) quit("fixture ego missing");
    ego_apply_magic(obj, 50);
    return carry_known(obj);
}

static struct object *artifact_item(const char *name)
{
    const struct artifact *art = lookup_artifact_name(name);
    if (!art) quit("fixture artifact missing");
    struct object *obj = object_new();
    object_prep(obj, lookup_kind(art->tval, art->sval), 50, MAXIMISE);
    obj->artifact = art;
    copy_artifact_data(obj, art);
    return carry_known(obj);
}

static void wear(struct object *obj)
{
    inven_wield(obj, wield_slot(obj));
}


int setup_tests(void **state)
{
    const char *profile = getenv("RW185_PROFILE");
    const char *mode    = getenv("RW185_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(185);
    if (!player_make_simple("Human", "Warrior", "ResPoisWar185")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = cave->depth = player->cave->depth = 20;
    player->max_depth = 25;
    player_exp_gain(player, player_exp[23] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *lantern = known_item(TV_LIGHT, "Lantern", 1);
    lantern->timeout = 2000;
    wear(lantern);
    known_item(TV_FOOD, "Ration of Food", 5);
    /* Main case: four Resist Cold potions as well. */
    if (streq(mode, "cold"))
        known_item(TV_POTION, "Resist Cold", 4);
    known_item(TV_POTION, "Resist Poison", 3);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=185 mode=%s class=%s clevel=%d depth=%d\n",
           mode, player->class->name, player->lev, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int find_slot(int tval, const char *name)
{
    int sval = lookup_sval(tval, name);
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty && borg_items[i].tval == tval && borg_items[i].sval == sval)
            return i;
    return -1;
}

static int scenario_reward(void *state)
{
    int slot = find_slot(TV_POTION, "Resist Poison");
    require(slot >= 0);
    borg_notice(true);
    int32_t with = borg_power();
    printf("STATE arescold=%d arespois=%d\n", borg.trait[BI_ARESCOLD], borg.trait[BI_ARESPOIS]);
    /* The value the Borg puts on its three Resist Poison potions: its power
     * with them minus its power with the stack removed. */
    int qty = borg_items[slot].iqty;
    borg_items[slot].iqty = 0;
    borg_notice(true);
    int32_t without = borg_power();
    borg_items[slot].iqty = qty;
    borg_notice(true);
    memcpy(safe_items, borg_items, QUIVER_END * sizeof(borg_item));
    printf("STATE potion_reward=%ld\n", (long)(with - without));

    /* The production junk pass, as after picking something up. */
    borg.power = borg_power();
    borg_do_crush_junk = true;
    borg_flush();
    uint16_t before = messages_num();
    bool rc = borg_drop_junk();
    uint16_t after = messages_num();
    int dropped_potion = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Dropping junk") && strstr(m, "Resist Poison")) dropped_potion = 1;
    }
    printf("DECISION junk=%d\n", rc ? 1 : 0);
    printf("OUTCOME potions_kept=%d\n", dropped_potion ? 0 : 1);
    /* 3 potions x 300 (all classes) + 3 x 500 (warriors past depth 20). */
    require(with - without == 2400);
    require(!dropped_potion);
    ok;
}

const char *suite_name = "native-c/respoiswar-185";
struct test tests[] = {
    {"resist_poison_potions_are_valued", scenario_reward},
    {NULL, NULL}};
