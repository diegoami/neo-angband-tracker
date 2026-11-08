/* Synthetic native C rest-check fixture, excluded from the source change.
 * Issue: diegoami/neo-angband-tracker#486 (restcomment-486). */
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
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-misc.h"
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

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 10, MAXIMISE);
    obj->number = number;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static int kidx = 0;

int setup_tests(void **state)
{
    const char *profile = getenv("RC486_PROFILE");
    const char *mode    = getenv("RC486_MODE");
    const char *race    = "cave bear";
    int dist = 3, lev = 10, awake = 1;
    if (!profile || !mode) return 1;
    if (streq(mode, "bear")) { /* defaults */ }
    else if (streq(mode, "bear-l20")) lev = 20;
    else if (streq(mode, "asleep")) awake = 0;
    else if (streq(mode, "wolf")) { race = "wolf"; dist = 2; }
    else if (streq(mode, "orc")) race = "cave orc";
    else { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(486);
    if (!player_make_simple("Human", "Warrior", "RestCheck486")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 10;
    if (lev > 1) player_exp_gain(player, player_exp[lev - 2] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp * 6 / 10;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    borg.c = player->grid;
    borg.time.level = borg.time.now;
    borg_notice(true);
    borg.avoidance = borg.trait[BI_CURHP];
    struct monster *mon = t_add_monster(cave, loc(10 + dist, 12), race);
    if (!mon) { printf("SETUPFAIL mon\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    kidx = borg_grids[mon->grid.y][mon->grid.x].kill;
    if (kidx <= 0) { printf("SETUPFAIL kill\n"); return 1; }
    borg_kills[kidx].awake = awake ? true : false;
    printf("SETUP mode=%s race=%s dist=%d clevel=%d hp=%d/%d avoidance=%d\n", mode, race, dist,
           player->lev, borg.trait[BI_CURHP], borg.trait[BI_MAXHP], borg.avoidance);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_rest(void *state)
{
    borg_kill *k = &borg_kills[kidx];
    int y = borg.c.y, x = borg.c.x;
    int p_mon  = borg_danger_one_kill(k->pos.y, k->pos.x, 1, kidx, true, true);
    int p_rest = borg_danger_one_kill(y, x, 1, kidx, true, true);
    printf("DANGER at_monster_grid=%d at_rest_grid=%d threshold=%d awake=%d ranged=%d speed=%d\n",
           p_mon, p_rest, borg.avoidance / 3, k->awake ? 1 : 0, k->ranged_attack, k->speed);
    bool rest = borg_check_rest(y, x);
    printf("DECISION check_rest=%d\n", rest ? 1 : 0);
    printf("OUTCOME rest=%d\n", rest ? 1 : 0);
    ok;
}

const char *suite_name = "native-c/restcomment-486";
struct test tests[] = {
    {"check_rest", scenario_rest},
    {NULL, NULL}};
