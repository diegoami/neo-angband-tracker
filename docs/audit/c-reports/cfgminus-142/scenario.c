/* Synthetic native C bare-minus borg.txt setting fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#142 (cfgminus-142). */
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
#include <sys/stat.h>
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
    const char *profile = getenv("CN142_PROFILE");
    const char *mode    = getenv("CN142_MODE");
    if (!profile || !mode) return 1;
    /* A per-case user directory: the stock borg.txt plus one line. */
    static char user_dir[1024];
    strnfmt(user_dir, sizeof(user_dir), "%s/%s", profile, mode);
    mkdir(user_dir, 0755);
    {
        char src[1024], dst[1024], line[1024];
        strnfmt(src, sizeof(src), "%s/borg.txt", profile);
        strnfmt(dst, sizeof(dst), "%s/borg.txt", user_dir);
        FILE *in = fopen(src, "r"), *out = fopen(dst, "w");
        if (!in || !out) { printf("SETUPFAIL borg.txt\n"); return 1; }
        while (fgets(line, sizeof(line), in)) fputs(line, out);
        fprintf(out, "%s\n", streq(mode, "minus") ? "borg_no_deeper = -" : streq(mode, "negative") ? "borg_no_deeper = -5" : "borg_no_deeper = 100");
        fclose(in); fclose(out);
    }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(user_dir);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(142);
    if (!player_make_simple("Human", "Warrior", "CfgMinus142")) { printf("SETUPFAIL birth\n"); return 1; }
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
    printf("SETUP seed=142 mode=%s line=\"%s\" class=%s clevel=%d max_depth=%d depth=%d\n",
           mode, streq(mode, "minus") ? "borg_no_deeper = -" : streq(mode, "negative") ? "borg_no_deeper = -5" : "borg_no_deeper = 100", player->class->name, player->lev, player->max_depth, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_nodeeper(void *state)
{
    const char *mode = getenv("CN142_MODE");
    borg_notice(true);
    const char *why = borg_prepared(20);
    printf("STATE no_deeper=%d\n", borg_cfg[BORG_NO_DEEPER]);
    printf("PREPARED depth=20 reason=%s\n", why ? why : "ready");
    printf("OUTCOME prepared20=%d\n", why ? 0 : 1);
    if (streq(mode, "negative")) {
        /* A real negative number is still read as negative. */
        require(borg_cfg[BORG_NO_DEEPER] == -5);
    } else {
        /* A bare '-' is not a number: the default (127) must stay. */
        require(borg_cfg[BORG_NO_DEEPER] > 0);
        require(!why);
    }
    ok;
}

const char *suite_name = "native-c/cfgminus-142";
struct test tests[] = {
    {"no_deeper_setting_is_read", scenario_nodeeper},
    {NULL, NULL}};
