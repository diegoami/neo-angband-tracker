/* Synthetic native C breeder-level door-closing fixture, excluded from the
 * source change.  Issue: diegoami/neo-angband-tracker#126 (doorcap-126). */
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
    const char *profile = getenv("DC126_PROFILE");
    const char *mode    = getenv("DC126_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(126);
    if (!player_make_simple("Dwarf", "Rogue", "DoorCap126")) { printf("SETUPFAIL birth\n"); return 1; }
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
    /* Open doors next to the player, off the route east. */
    if (!streq(mode, "nodoor")) square_set_feat(cave, loc(PX, PY - 1), FEAT_OPEN);
    if (streq(mode, "twodoors")) square_set_feat(cave, loc(PX, PY + 1), FEAT_OPEN);
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
    /* The Borg's map: the real cave, except that the columns from x=30
     * eastward have not been seen yet, so the dark flow heads east. */
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_MARK;
        if (x >= 30 && x < W - 1 && y > 0 && y < H - 1) {
            borg_grids[y][x].feat = FEAT_NONE;
            borg_grids[y][x].info &= ~(BORG_OKAY | BORG_MARK);
        }
    }
    update_view(cave, player);
    borg.c = player->grid;
    borg.time.level = borg.time.now;
    borg_notice(true);
    borg_update_view();
    borg_update_light();
    borg.avoidance = borg.trait[BI_CURHP];
    /* Breeder level: the flag borg_check_breeders() sets when at least
     * three breeders are known (set directly here). */
    borg.near.breeder = !streq(mode, "nobreeder");
    /* "full": the door list is at its capacity of 100 entries (doors closed
     * earlier on the level, recorded at grids away from the player). */
    track_door.num = 0;
    if (streq(mode, "full")) {
        for (int i = 0; i < track_door.size; ++i) {
            track_door.x[i] = 1 + i % 20;
            track_door.y[i] = 1 + i / 20;
        }
        track_door.num = track_door.size;
    }
    printf("SETUP seed=126 mode=%s class=%s clevel=%d depth=%d breeder=%d doors=%d/%d\n",
           mode, player->class->name, player->lev, player->depth, borg.near.breeder ? 1 : 0,
           track_door.num, track_door.size);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_step(void *state)
{
    keycode_t keys[16];
    int len = 0;
    keycode_t key;
    int before = track_door.num;
    borg_flush();
    borg.goal.type = 0;
    /* The far dark flow commits a route east and takes its first step
     * through borg_flow_old() and borg_play_step(). */
    bool rc = borg_flow_dark(false);
    while ((key = borg_inkey(true)) != 0 && len < 16) keys[len++] = key;
    printf("DECISION rc=%d keys=%d sequence=", rc ? 1 : 0, len);
    for (int i = 0; i < len; ++i) printf("%c", (char)keys[i]);
    putchar('\n');
    /* The close command is queued as 'c'; its direction is passed
     * separately (borg_queue_direction), so the closed grid is read from
     * the goal the step set. */
    int closed = (rc && len >= 1 && keys[0] == 'c') ? 1 : 0;
    printf("TRACK goal=(%d,%d) doors_before=%d doors_after=%d last=(%d,%d)\n", borg.goal.g.x,
           borg.goal.g.y, before, track_door.num,
           track_door.num ? track_door.x[track_door.num - 1] : -1,
           track_door.num ? track_door.y[track_door.num - 1] : -1);
    printf("OUTCOME close=%d door_x=%d door_y=%d noted=%d\n", closed, closed ? borg.goal.g.x : -1,
           closed ? borg.goal.g.y : -1, track_door.num - before);
    ok;
}

const char *suite_name = "native-c/doorcap-126";
struct test tests[] = {
    {"play_step", scenario_step},
    {NULL, NULL}};
