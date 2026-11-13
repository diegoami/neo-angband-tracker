/* Synthetic native C wand attack-choice fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#38 (wandnames-38, CBUG-0022). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-util.h"
#include "monster.h"
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
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static struct monster *ogre;

/* A fully known wand with the given name and charges, carried in the pack. */
static struct object *known_wand(const char *name, int charges)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(TV_WAND, lookup_sval(TV_WAND, name));
    if (!kind) { fprintf(stderr, "MISSING WAND %s\n", name); quit("fixture wand kind missing"); }
    object_prep(obj, kind, 30, MAXIMISE);
    obj->pval = charges;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

/* Remove every pack item that is not a wand, so the only ranged attacks the
 * Borg can consider are the wands under test. Food is kept. */
static void strip_pack(void)
{
    bool again = true;
    while (again) {
        again = false;
        for (struct object *o = player->gear; o; o = o->next) {
            if (object_is_equipped(player->body, o)) continue;
            if (tval_is_wand(o) || tval_is_food(o)) continue;
            bool none = false;
            struct object *used = gear_object_for_use(player, o, o->number, false, &none);
            object_delete(player->cave, NULL, &used);
            again = true;
            break;
        }
    }
}

int setup_tests(void **state)
{
    const char *profile = getenv("WN38_PROFILE");
    const char *mode    = getenv("WN38_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(38);
    if (!player_make_simple("Human", "Warrior", "WandNames38")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 20;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[19] * player->expfact / 100);
    strip_pack();
    if (streq(mode, "bolts-vs-frost")) {
        known_wand("Lightning Bolts", 10);
        known_wand("Frost Bolts", 10);
    } else if (streq(mode, "balls-only")) {
        known_wand("Lightning Balls", 10);
    } else if (streq(mode, "confuse-only")) {
        known_wand("Confuse Monster", 10);
    } else if (streq(mode, "scare-only")) {
        known_wand("Scare Monster", 10);
    } else if (streq(mode, "frost-only")) {
        known_wand("Frost Bolts", 10);
    } else {
        printf("SETUPFAIL mode\n");
        return 1;
    }
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW | BORG_GLOW;
    }
    update_view(cave, player);
    /* An awake black ogre four grids east, in a straight line: out of melee
     * reach, in range and line of fire of every wand. */
    ogre = t_add_monster(cave, loc(15, 11), "black ogre");
    if (!ogre) { printf("SETUPFAIL ogre\n"); return 1; }
    mflag_on(ogre->mflag, MFLAG_AWARE); update_mon(ogre, cave, true);
    if (!mflag_has(ogre->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL ogre visible\n"); return 1; }
    if (!observe_kill_diff(ogre->grid.y, ogre->grid.x, 0, 0)) { printf("SETUPFAIL ogre observe\n"); return 1; }
    int k = borg_grids[ogre->grid.y][ogre->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL ogre kill\n"); return 1; }
    borg_kills[k].awake = 1;
    borg.status.active = true;
    printf("SETUP seed=38 mode=%s class=%s clevel=%d depth=%d hp=%d/%d ogre_kill=%d ogre_hp=%d\n",
        mode, player->class->name, player->lev, player->depth, player->chp, player->mhp,
        k, ogre->hp);
    return 0;
}

int teardown_tests(void *state)
{
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

/* The wand attack types whose wand lookups the issue concerns, plus the
 * frost bolt alternative. */
static const struct { int type; const char *name; } WAND_TYPES[] = {
    { BF_WAND_ELEC_BOLT, "elec_bolt" },
    { BF_WAND_ELEC_BALL, "elec_ball" },
    { BF_WAND_COLD_BOLT, "cold_bolt" },
    { BF_WAND_CONFUSE_MONSTER, "confuse" },
    { BF_WAND_FEAR_MONSTER, "fear" },
};

static const char *type_name(int type)
{
    for (size_t i = 0; i < N_ELEMENTS(WAND_TYPES); ++i)
        if (WAND_TYPES[i].type == type) return WAND_TYPES[i].name;
    return "other";
}

static int scenario(void *state)
{
    const char *mode = getenv("WN38_MODE");
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty)
            printf("PACK %d tval=%d sval=%d desc=[%s]\n", i, borg_items[i].tval,
                   borg_items[i].sval, borg_items[i].desc);

    /* Simulated value of each wand attack type against the ogre, exactly as
     * the production chooser computes it (target list as borg_attack fills it). */
    borg_temp_x[0] = ogre->grid.x; borg_temp_y[0] = ogre->grid.y; borg_temp_n = 1;
    int value[N_ELEMENTS(WAND_TYPES)];
    borg_simulate = true;
    for (size_t i = 0; i < N_ELEMENTS(WAND_TYPES); ++i) {
        value[i] = borg_calculate_attack_effectiveness(WAND_TYPES[i].type);
        printf("VALUE %s=%d\n", WAND_TYPES[i].name, value[i]);
    }

    /* The production attack decision. It notes the chosen type and queues
     * the keys for that wand. */
    int before_msgs = messages_num();
    bool attacked = borg_attack(false);
    int chosen = -1, chosen_value = 0;
    for (int m = messages_num() - before_msgs - 1; m >= 0; --m) {
        int t, v;
        if (sscanf(message_str(m), "# Performing attack type %d with value %d.", &t, &v) == 2) {
            chosen = t; chosen_value = v; break;
        }
    }
    printf("DECISION attacked=%d type=%d (%s) value=%d\n", attacked ? 1 : 0, chosen,
           chosen >= 0 ? type_name(chosen) : "none", chosen_value);
    printf("OUTCOME attacked=%d elec_bolt=%d elec_ball=%d cold_bolt=%d confuse=%d fear=%d chose_cold=%d chose_elec_ball=%d\n",
           attacked ? 1 : 0, value[0] > 0, value[1] > 0, value[2] > 0, value[3] > 0, value[4] > 0,
           chosen == BF_WAND_COLD_BOLT, chosen == BF_WAND_ELEC_BALL);

    /* Correct contract per mode: each wand is offered only as the attack it
     * really performs. */
    if (streq(mode, "bolts-vs-frost")) {
        /* A Lightning Bolts wand is a 6d6 beam, not a 64-damage ball, so the
         * 12d8 Frost Bolts wand is the better attack. */
        require(value[1] == 0);
        require(chosen == BF_WAND_COLD_BOLT);
    } else if (streq(mode, "balls-only")) {
        /* A real Lightning Balls wand is usable as one. */
        require(value[1] > 0);
        require(chosen == BF_WAND_ELEC_BALL);
    } else if (streq(mode, "confuse-only")) {
        /* A real Confuse Monster wand is evaluated as the confusion attack
         * (any non-zero score; 0 means no matching wand was found). */
        require(value[3] != 0);
    } else if (streq(mode, "scare-only")) {
        /* A Scare Monster wand does not confuse. */
        require(value[3] == 0);
    } else {
        /* Control: Frost Bolts only, chosen in both builds. */
        require(chosen == BF_WAND_COLD_BOLT);
    }
    ok;
}

const char *suite_name = "native-c/wandnames-38";
struct test tests[] = {
    {"each_wand_is_offered_as_what_it_does", scenario},
    {NULL, NULL}};
