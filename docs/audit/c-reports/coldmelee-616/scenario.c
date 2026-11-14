/* Synthetic native C cold-melee fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#616 (coldmelee-616). */
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

/* Wear a fully known item of the given kind. */
static void wear(int tval, const char *name)
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

/* Set the temporary resistances, let the game and the Borg recompute, and
 * return the production borg_danger() of the Borg's grid. */
static int reading(bool cold, bool acid, bool fire)
{
    player->timed[TMD_OPP_COLD] = cold ? 50 : 0;
    player->timed[TMD_OPP_ACID] = acid ? 50 : 0;
    player->timed[TMD_OPP_FIRE] = fire ? 50 : 0;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_equipment(); borg_inventory(); borg_notice(true); borg_notice_player();
    borg.avoidance = player->chp;
    return borg_danger(borg.c.y, borg.c.x, 1, true, false);
}

int setup_tests(void **state)
{
    const char *profile = getenv("CM616_PROFILE");
    const char *mode    = getenv("CM616_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(616);
    if (!player_make_simple("Human", "Warrior", "ColdMelee616")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 28;
    player_exp_gain(player, player_exp[28] * player->expfact / 100);
    character_dungeon = true;
    wear(TV_SOFT_ARMOR, "Soft Leather Armour");
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
    /* An awake frost giant next to the Borg (two 5d8 cold blows). */
    struct monster *giant = t_add_monster(cave, loc(13, 12), "frost giant");
    if (!giant) { printf("SETUPFAIL giant\n"); return 1; }
    if (observe(giant, "giant")) return 1;
    borg.status.active = true;
    printf("SETUP seed=616 mode=%s class=%s clevel=%d hp=%d\n", mode,
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
    const char *mode = getenv("CM616_MODE");
    /* Reference: no temporary resistance. */
    int ref_danger = reading(false, false, false);
    int danger;
    if (streq(mode, "cold"))
        danger = reading(true, false, false);
    else if (streq(mode, "acid"))
        danger = reading(false, true, false);
    else
        danger = reading(false, false, true);
    printf("STATE temp_cold=%d temp_acid=%d temp_fire=%d borg_rcold=%d\n",
           borg.temp.res_cold ? 1 : 0, borg.temp.res_acid ? 1 : 0,
           borg.temp.res_fire ? 1 : 0, borg.trait[BI_RCOLD] ? 1 : 0);
    printf("OUTCOME danger=%d reference=%d\n", danger, ref_danger);
    /* Only temporary cold resistance reduces the danger of cold blows. */
    if (streq(mode, "cold"))
        require(danger < ref_danger);
    else
        require(danger == ref_danger);
    ok;
}

const char *suite_name = "native-c/coldmelee-616";
struct test tests[] = {
    {"cold_blow_danger_follows_temporary_cold_resistance", scenario_danger},
    {NULL, NULL}};
