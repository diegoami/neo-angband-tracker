/* Synthetic native C vulnerability-resistance fixture, excluded from the
 * source fix. Issue: diegoami/neo-angband-tracker#594 (vulnres-594). */
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

/* Wear a fully known item of the given kind with the given fire
 * resistance level (1 = resist, -1 = vulnerable, as the burning up curse). */
static void wear_fire(int tval, const char *name, int fire)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 35, MAXIMISE);
    obj->el_info[ELEM_FIRE].res_level = fire;
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

int setup_tests(void **state)
{
    const char *profile = getenv("VR594_PROFILE");
    const char *mode    = getenv("VR594_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(594);
    if (!player_make_simple("Human", "Warrior", "VulnRes594")) { printf("SETUPFAIL birth\n"); return 1; }
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
    /* Fire resistance from the armour; in "vuln" mode a shield carries a fire
     * vulnerability (RES_FIRE[-1], as the burning up curse gives). */
    wear_fire(TV_SOFT_ARMOR, "Soft Leather Armour", 1);
    if (streq(mode, "vuln")) wear_fire(TV_SHIELD, "Leather Shield", -1);
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
    struct monster *hound = t_add_monster(cave, loc(15, 12), "young red dragon");
    if (!hound) { printf("SETUPFAIL dragon\n"); return 1; }
    if (observe(hound, "dragon")) return 1;
    borg.status.active = true;
    printf("SETUP seed=594 mode=%s class=%s clevel=%d hp=%d game_fire_res_level=%d\n", mode,
           player->class->name, player->lev, player->chp, player->state.el_info[ELEM_FIRE].res_level);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_danger(void *state)
{
    const char *mode = getenv("VR594_MODE");
    borg_notice(true); borg_notice_player();
    int game_level = player->state.el_info[ELEM_FIRE].res_level;
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("STATE game_fire_res_level=%d borg_rfire=%d borg_fire_vuln=%d\n", game_level,
           borg.trait[BI_RFIRE] ? 1 : 0, borg.trait[BI_CRSFVULN] ? 1 : 0);
    printf("OUTCOME danger=%d borg_rfire=%d\n", danger, borg.trait[BI_RFIRE] ? 1 : 0);
    /* The Borg must believe it resists fire exactly when the game says so. */
    require((borg.trait[BI_RFIRE] ? 1 : 0) == (game_level >= 1 ? 1 : 0));
    ok;
}

const char *suite_name = "native-c/vulnres-594";
struct test tests[] = {
    {"fire_resistance_matches_the_game", scenario_danger},
    {NULL, NULL}};
