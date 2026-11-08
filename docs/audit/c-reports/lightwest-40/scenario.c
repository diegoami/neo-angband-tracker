/* Synthetic native C lantern-light fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#40 (lightwest-40). */
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
#include "trap.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave-light.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-stairs.h"
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
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 50, MAXIMISE);
    obj->number = number;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
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

int setup_tests(void **state)
{
    const char *profile = getenv("LW40_PROFILE");
    const char *mode    = getenv("LW40_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(40);
    if (!player_make_simple("Human", "Warrior", "LightWest40")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    /* The room is dark: only the Borg's own light shows its surroundings. */
    /* A staircase two steps west and, except in the control, a second one
     * of the same kind 25 steps east.  Up staircases, or down staircases in
     * the "twodown" case. */
    struct object *torch = known_item(TV_LIGHT, "Lantern", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* The Borg has seen both staircases. */
    track_less.num = track_more.num = 0;
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    /* Some time into the level; no anti-summon corridor in use. */
    borg.time.now = 1000;
    borg.time.level = borg.time.now;
    borg.time.antisummon = 0;
    borg.time.town = borg.time.now;
    borg_notice(true);
    borg.avoidance = player->chp;
    /* An awake cave bear was last seen two grids west ('west') or east
     * ('east', control) of the Borg and one grid south, in the dark.  It has
     * since walked away: the game monster is removed, the Borg still has it
     * in its monster list. */
    int fx = streq(mode, "west") ? 8 : 12;
    struct monster *foe = t_add_monster(cave, loc(fx, 13), "Cave bear");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    mflag_on(foe->mflag, MFLAG_AWARE); update_mon(foe, cave, true);
    if (!observe_kill_diff(foe->grid.y, foe->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int kidx = borg_grids[foe->grid.y][foe->grid.x].kill;
    if (kidx <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[kidx].awake = 1;
    delete_monster(cave, loc(fx, 13));
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=40 mode=%s class=%s clevel=%d hp=%d depth=%d \n",
           mode, player->class->name, player->lev, player->chp, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_follow(void *state)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *mode = getenv("LW40_MODE");
    uint16_t before = messages_num();
    borg_flush();
    borg_notice(true);
    borg_update_light();
    int k = 1;
    int gy = borg_kills[k].pos.y, gx = borg_kills[k].pos.x;
    printf("STATE borg=(%d,%d) light=%d lit_grids=%d kill_at=(%d,%d) grid_lit=%d\n", borg.c.x, borg.c.y,
           borg.trait[BI_LIGHT], (int)borg_light_n, gx, gy,
           (borg_grids[gy][gx].info & BORG_LIGHT) ? 1 : 0);
    int r_before = borg_kills[k].r_idx;
    borg_follow_kill(k);
    int r_after = borg_kills[k].r_idx;
    printf("DECISION kill_before=%d kill_after=%d\n", r_before ? 1 : 0, r_after ? 1 : 0);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    printf("OUTCOME ghost_kept=%d\n", r_after ? 1 : 0);
    (void)mode;
    require(!r_after); /* the bear is not at a lit grid any more: it must not stay on the list */
    ok;
}

const char *suite_name = "native-c/lightwest-40";
struct test tests[] = {
    {"unseen_monster_in_the_lantern_light", scenario_follow},
    {NULL, NULL}};
