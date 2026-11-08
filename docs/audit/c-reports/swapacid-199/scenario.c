/* Synthetic native C swap-weapon acid-resistance fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#199 (swapacid-199). */
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
#include "borg/borg-trait-swap.h"
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
    const char *profile = getenv("SA199_PROFILE");
    const char *mode    = getenv("SA199_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(199);
    if (!player_make_simple("Human", "Warrior", "SwapAcid199")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    /* A level-50 Warrior that has been to depth 50 (the Borg only keeps a
     * swap weapon from max depth 50), now standing on depth 30. */
    player->depth = cave->depth = player->cave->depth = 30;
    player->max_depth = 50;
    player_exp_gain(player, player_exp[48] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);

    /* Worn gear: fire and cold (ring), lightning (body armour), free
     * action (ring), telepathy (amulet), a lantern.  Nothing resists acid. */
    struct object *lantern = known_item(TV_LIGHT, "Lantern", 1);
    lantern->timeout = 2000;
    wear(lantern);
    wear(known_item(TV_RING, "Resist Fire and Cold", 1));
    wear(known_item(TV_RING, "Free Action", 1));
    wear(known_item(TV_AMULET, "ESP", 1));
    wear(ego_item(TV_HARD_ARMOR, "Metal Scale Mail", "of Resist Lightning"));
    wear(known_item(TV_SWORD, "Long Sword", 1));
    /* Supplies for the shallower preparedness bands. */
    known_item(TV_FOOD, "Ration of Food", 6);
    known_item(TV_SCROLL, "Word of Recall", 2);
    known_item(TV_SCROLL, "Teleportation", 6);
    known_item(TV_SCROLL, "Phase Door", 5);
    known_item(TV_POTION, "Cure Light Wounds", 5);

    /* The carried swap weapon under test. */
    if (streq(mode, "elec"))
        artifact_item("'Dethanc'");      /* resists lightning only */
    else if (streq(mode, "acid"))
        artifact_item("'Angrist'");      /* resists acid (and dark) */
    else if (streq(mode, "fire"))
        artifact_item("'Narthanc'");     /* resists fire only */
    /* mode "none": no swap weapon */

    player_learn_all_runes(player);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;

    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    borg_cfg[BORG_PLAYS_RISKY] = false;
    borg_equipment(); borg_inventory(); borg_notice(true);
    printf("SETUP seed=199 mode=%s class=%s clevel=%d max_depth=%d depth=%d\n",
           mode, player->class->name, player->lev, player->max_depth, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_swap(void *state)
{
    const char *mode = getenv("SA199_MODE");
    const char *why;
    char swap_name[120] = "(none)";

    /* Production notice pass, including the swap scan. */
    borg_notice(true);
    if (weapon_swap > 0)
        my_strcpy(swap_name, borg_items[weapon_swap - 1].desc, sizeof(swap_name));
    why = borg_prepared(30);

    printf("STATE base_racid=%d base_relec=%d base_rfire=%d base_rcold=%d\n",
           borg.trait[BI_RACID] ? 1 : 0, borg.trait[BI_RELEC] ? 1 : 0,
           borg.trait[BI_RFIRE] ? 1 : 0, borg.trait[BI_RCOLD] ? 1 : 0);
    printf("SWAP item=%s\n", swap_name);
    printf("STATE swap=%d swap_acid=%d swap_elec=%d swap_fire=%d\n",
           weapon_swap > 0 ? 1 : 0, weapon_swap_resist_acid ? 1 : 0,
           weapon_swap_resist_elec ? 1 : 0, weapon_swap_resist_fire ? 1 : 0);
    printf("STATE sracid=%d srelec=%d srfire=%d srcold=%d\n",
           borg.trait[BI_SRACID] ? 1 : 0, borg.trait[BI_SRELEC] ? 1 : 0,
           borg.trait[BI_SRFIRE] ? 1 : 0, borg.trait[BI_SRCOLD] ? 1 : 0);
    printf("PREPARED depth=30 reason=%s\n", why ? why : "ready");
    printf("OUTCOME prepared30=%d acid_counted=%d\n", why ? 0 : 1,
           borg.trait[BI_SRACID] ? 1 : 0);

    /* The Borg counts acid resistance only when something it wears or can
     * swap to actually resists acid. */
    if (streq(mode, "elec") || streq(mode, "fire") || streq(mode, "none")) {
        require(!borg.trait[BI_SRACID]);
    } else {
        require(borg.trait[BI_SRACID]);
    }
    ok;
}

const char *suite_name = "native-c/swapacid-199";
struct test tests[] = {
    {"swap_weapon_acid_resistance", scenario_swap},
    {NULL, NULL}};
