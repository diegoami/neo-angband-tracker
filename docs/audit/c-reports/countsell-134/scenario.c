/* Synthetic native C sellable-count fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#134 (countsell-134). */
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
#include "borg/borg-store-sell.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-flow.h"
#include "borg/borg-io.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-update.h"
#include "borg/borg-power.h"
#include "borg/borg-item-id.h"
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


static const char *spares[] = { "Dagger", "Main Gauche", "Rapier", "Short Sword",
    "Cutlass", "Tulwar", "Broad Sword", "Long Sword", "Scimitar", "Katana",
    "Bastard Sword", "Zweihander", "Executioner's Sword", NULL };

int setup_tests(void **state)
{
    const char *profile = getenv("CS134_PROFILE");
    const char *mode    = getenv("CS134_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(134);
    if (!player_make_simple("Human", "Warrior", "CountSell134")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 1;
    player_exp_gain(player, player_exp[2] * player->expfact / 100);
    player->au = 0;
    character_dungeon = true;
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* A down staircase three steps east, an up staircase far west. */
    square_set_feat(cave, loc(15, 12), FEAT_MORE);
    square_set_feat(cave, loc(2, 12), FEAT_LESS);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    wear(torch);
    known_item(TV_FOOD, "Ration of Food", 6);
    /* "full" and "full-unid": thirteen different spare weapons to sell. */
    if (!streq(mode, "light"))
        for (int i = 0; spares[i]; ++i) known_item(TV_SWORD, spares[i], 1);
    /* "full-unid" and "light": one spare weapon with an unidentified rune. */
    if (!streq(mode, "full")) {
        struct object *odd = object_new();
        struct object_kind *kind = lookup_kind(TV_HAFTED, lookup_sval(TV_HAFTED, "Mace"));
        object_prep(odd, kind, 10, MAXIMISE);
        odd->ego = lookup_ego_item("(Holy Avenger)", kind->tval, kind->sval);
        if (!odd->ego) quit("fixture ego missing");
        ego_apply_magic(odd, 10);
        carry_known(odd); /* flavour and base known; its runes are not */
    }
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    track_less.num = track_more.num = 0;
    track_more.x[0] = 15; track_more.y[0] = 12; track_more.num = 1;
    track_less.x[0] = 2; track_less.y[0] = 12; track_less.num = 1;
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=134 mode=%s class=%s clevel=%d depth=%d gold=%d\n",
           mode, player->class->name, player->lev, player->depth, (int)player->au);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_sell(void *state)
{
    const char *mode = getenv("CS134_MODE");
    int unid = 0;
    borg_notice(true);
    borg.power = borg_power();
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty && borg_item_note_needs_id(&borg_items[i])) ++unid;
    int count = borg_count_sell();
    const char *why = borg_prepared(borg.trait[BI_CDEPTH] + 1);
    printf("STATE sellable=%d unidentified=%d prepared_next=%d\n", count, unid, why ? 0 : 1);
    /* The production down-stair flow, as when leaving the level. */
    borg_flush();
    uint16_t before = messages_num();
    bool down = borg_flow_stair_more(GOAL_FLEE, false, false);
    keycode_t key = borg_inkey(false);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    printf("DECISION flow_down=%d key=%d\n", down ? 1 : 0, (int)key);
    printf("OUTCOME went_down=%d\n", down ? 1 : 0);
    if (streq(mode, "light"))
        require(down);   /* nothing to sell: dive */
    else
        require(!down);  /* a full pack: go up to sell first (fails before for full-unid) */
    ok;
}

const char *suite_name = "native-c/countsell-134";
struct test tests[] = {
    {"full_pack_goes_up_to_sell", scenario_sell},
    {NULL, NULL}};
