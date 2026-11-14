/* Synthetic native C elemental-vulnerability fixture, excluded from the
 * source fix. Issue: diegoami/neo-angband-tracker#610 (vulnmod-610). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "object.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40

static struct object *armour, *shield;

/* Wear a fully known item of the given kind. */
static struct object *wear(int tval, const char *name)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 35, MAXIMISE);
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    inven_wield(obj, wield_slot(obj));
    return obj;
}

static int observe(struct monster *mon, const char *tag)
{
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!mflag_has(mon->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL %s visible\n", tag); return 1; }
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL %s observe\n", tag); return 1; }
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL %s kill_index\n", tag); return 1; }
    borg_kills[k].awake = 1;
    return 0;
}

/* Set the fire resistance levels of the two worn items and the temporary
 * fire resistance, let the game and the Borg recompute, and return the
 * production borg_danger() of the Borg's grid. */
static int reading(int armour_fire, int shield_fire, bool temp, int *game_level)
{
    armour->el_info[ELEM_FIRE].res_level = armour_fire;
    armour->known->el_info[ELEM_FIRE].res_level = armour_fire;
    shield->el_info[ELEM_FIRE].res_level = shield_fire;
    shield->known->el_info[ELEM_FIRE].res_level = shield_fire;
    player->timed[TMD_OPP_FIRE] = temp ? 50 : 0;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_equipment(); borg_inventory(); borg_notice(true); borg_notice_player();
    borg.avoidance = player->chp;
    *game_level = player->state.el_info[ELEM_FIRE].res_level;
    return borg_danger(borg.c.y, borg.c.x, 1, true, false);
}

int setup_tests(void **state)
{
    const char *profile = getenv("VM610_PROFILE");
    const char *mode    = getenv("VM610_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(610);
    if (!player_make_simple("Human", "Warrior", "VulnMod610")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 35;
    player_exp_gain(player, player_exp[33] * player->expfact / 100);
    character_dungeon = true;
    armour = wear(TV_SOFT_ARMOR, "Soft Leather Armour");
    shield = wear(TV_SHIELD, "Leather Shield");
    player_learn_all_runes(player);
    for (struct object *o = player->gear; o; o = o->next) player_know_object(player, o);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    borg_equipment(); borg_inventory(); borg_notice(true); borg_notice_player();
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW | BORG_GLOW;
    }
    update_view(cave, player);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    /* An awake young red dragon three grids east (fire breath range). */
    struct monster *dragon = t_add_monster(cave, loc(15, 12), "young red dragon");
    if (!dragon) { printf("SETUPFAIL dragon\n"); return 1; }
    if (observe(dragon, "dragon")) return 1;
    borg.status.active = true;
    printf("SETUP seed=610 mode=%s class=%s clevel=%d hp=%d\n", mode,
           player->class->name, player->lev, player->mhp);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_danger(void *state)
{
    const char *mode = getenv("VM610_MODE");
    int ref_level, level, ref_danger, danger;
    /* Reference: no fire resistance, no vulnerability, no temporary
     * resistance (the game's level 0: fire damage taken as it is). */
    ref_danger = reading(0, 0, false, &ref_level);
    if (streq(mode, "vuln"))
        danger = reading(0, -1, false, &level);  /* game level -1: x4/3 */
    else if (streq(mode, "temp"))
        danger = reading(0, -1, true, &level);   /* -1 + 1 = 0: no reduction */
    else
        danger = reading(1, -1, false, &level);  /* 1 - 1 = 0 (as #594) */
    printf("STATE game_fire_res_level=%d reference_game_fire_res_level=%d borg_rfire=%d borg_temp_rfire=%d borg_fire_vuln=%d\n",
           level, ref_level, borg.trait[BI_RFIRE] ? 1 : 0, borg.temp.res_fire ? 1 : 0,
           borg.trait[BI_CRSFVULN] ? 1 : 0);
    printf("OUTCOME danger=%d reference=%d\n", danger, ref_danger);
    if (ref_level != 0) { printf("SETUPFAIL reference level\n"); return 1; }
    /* The Borg's danger must move with the game's resistance level: more
     * than the reference when the game takes x4/3 damage (level -1), equal
     * to it when the game takes damage as it is (level 0). */
    if (level < 0)
        require(danger > ref_danger);
    else
        require(danger == ref_danger);
    ok;
}

const char *suite_name = "native-c/vulnmod-610";
struct test tests[] = {
    {"fire_danger_follows_the_game_resistance_level", scenario_danger},
    {NULL, NULL}};
