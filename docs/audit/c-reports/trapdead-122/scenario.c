/* Synthetic native C dark-flow exploration fixture, excluded from the
 * source change.  Issue: diegoami/neo-angband-tracker#122 (trapdead-122). */
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
#include "borg/borg-cave.h"
#include "borg/borg-cave-light.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-dark.h"
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
#define PX 10
#define PY 12

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

int setup_tests(void **state)
{
    const char *profile = getenv("TD122_PROFILE");
    const char *mode    = getenv("TD122_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(122);
    if (!player_make_simple("Dwarf", "Rogue", "TrapDead122")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[23] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* A closed door four grids west of the player (door mode). */
    if (streq(mode, "door")) square_set_feat(cave, loc(PX - 4, PY), FEAT_CLOSED);
    struct object *lantern = known_item(TV_LIGHT, "Lantern", 1);
    lantern->timeout = 2000;
    inven_wield(lantern, wield_slot(lantern));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    /* The Borg's map: the real cave, except that in the "unknown" mode the
     * columns from x=30 eastward have not been seen yet. */
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_MARK;
        if (streq(mode, "unknown") && x >= 30 && x < W - 1 && y > 0 && y < H - 1) {
            borg_grids[y][x].feat = FEAT_NONE;
            borg_grids[y][x].info &= ~(BORG_OKAY | BORG_MARK);
        }
    }
    /* A known, visible trap on the open floor next to the player, as the
     * Borg records traps: the grid keeps FEAT_FLOOR and ag->trap is set. */
    borg_grids[PY][PX + 1].trap = true;
    update_view(cave, player);
    borg.c = player->grid;
    borg.time.level = borg.time.now;
    borg_notice(true);
    borg_update_view();
    borg_update_light();
    borg.avoidance = borg.trait[BI_CURHP];
    printf("SETUP seed=122 mode=%s class=%s clevel=%d depth=%d hp=%d light=%d disp=%d dism=%d trap=(%d,%d) lit=%d\n",
           mode, player->class->name, player->lev, player->depth, borg.trait[BI_CURHP],
           borg.trait[BI_LIGHT], borg.trait[BI_DISP], borg.trait[BI_DISM], PX + 1, PY,
           borg_light_n);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_dark(void *state)
{
    int flowed = 0, pass = 0;
    keycode_t key = 0;
    for (pass = 0; pass < 2 && !flowed; ++pass) {
        borg_flush();
        borg.goal.type = 0;
        bool rc = borg_flow_dark(pass == 0);
        key = borg_inkey(false);
        printf("DECISION pass=%s rc=%d key=%d goal_type=%d step=(%d,%d)\n",
               pass == 0 ? "near" : "far", rc ? 1 : 0, (int)key, borg.goal.type,
               borg.goal.g.x, borg.goal.g.y);
        flowed = rc ? 1 : 0;
    }
    /* borg.goal.g is the grid of the step the flow took */
    int step_trap = (flowed && borg.goal.g.x == PX + 1 && borg.goal.g.y == PY) ? 1 : 0;
    printf("OUTCOME flowed=%d key=%d step_x=%d step_y=%d step_is_trap=%d\n", flowed, (int)key,
           flowed ? borg.goal.g.x : -1, flowed ? borg.goal.g.y : -1, step_trap);
    ok;
}

const char *suite_name = "native-c/trapdead-122";
struct test tests[] = {
    {"flow_dark", scenario_dark},
    {NULL, NULL}};
