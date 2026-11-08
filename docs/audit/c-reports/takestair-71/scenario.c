/* Synthetic native C item-pickup leash fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#71 (takestair-71). */
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
#include "borg/borg-cave-view.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-misc.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-flow-take.h"
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
#define W 72
#define PX 10
#define PY 12

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 1, MAXIMISE);
    obj->number = number;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static const char *mode;

int setup_tests(void **state)
{
    const char *profile = getenv("TS71_PROFILE");
    mode = getenv("TS71_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(71);
    if (!player_make_simple("Human", "Warrior", "TakeStair71")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 1;
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* An up staircase two steps west and, except in the control, a second
     * up staircase 55 steps east. */
    square_set_feat(cave, loc(PX - 2, PY), FEAT_LESS);
    if (!streq(mode, "control"))
        square_set_feat(cave, loc(PX + 55, PY), FEAT_LESS);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    /* A Flask of Oil on the floor three steps east of the Borg. */
    {
        struct object *oil = object_new();
        struct object_kind *kind = lookup_kind(TV_FLASK, lookup_sval(TV_FLASK, "Flask of Oil"));
        bool dummy = true;
        object_prep(oil, kind, 1, MAXIMISE);
        oil->number = 1;
        oil->known = object_new();
        object_set_base_known(player, oil);
        object_flavor_aware(player, oil);
        if (!floor_carry(cave, loc(PX + 3, PY), oil, &dummy)) { printf("SETUPFAIL floor\n"); return 1; }
    }
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    /* The panel count borg_update() derives for this one-panel arena; the
     * detection arrays are sized from it. */
    borg.panels.x = borg.panels.y = 1;
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* The Borg has seen the up staircases. */
    track_less.num = track_more.num = 0;
    track_less.x[track_less.num] = PX - 2; track_less.y[track_less.num] = PY; track_less.num++;
    if (!streq(mode, "control")) {
        track_less.x[track_less.num] = PX + 55; track_less.y[track_less.num] = PY; track_less.num++;
    }
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    borg_notice(true);
    borg.status.active = true;
    borg_update_entrypoint(true);
    /* The Borg notices the flask (the production object observer). */
    if (!observe_take_diff(PY, PX + 3, 0, 0)) { printf("SETUPFAIL take\n"); return 1; }
    printf("SETUP seed=71 mode=%s class=%s clevel=%d depth=%d up=%d takes=%d leash_take=%d leash_explore=%d\n",
           mode, player->class->name, borg.trait[BI_CLEVEL], borg.trait[BI_CDEPTH], track_less.num,
           borg_takes_cnt, borg_get_leash(true), borg_get_leash(false));
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_take(void *state)
{
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    for (int k = 1; k < borg_takes_nxt; ++k)
        printf("TAKE k=%d kind=%s pos=(%d,%d) value=%d\n", k, k_info[borg_takes[k].kidx].name,
               borg_takes[k].x, borg_takes[k].y, borg_takes[k].value);
    bool rc;
    const char *fn;
    if (streq(mode, "scum")) { fn = "borg_flow_take_scum(true, 6)"; rc = borg_flow_take_scum(true, 6); }
    else if (streq(mode, "lunal")) { fn = "borg_flow_take_lunal(true, 4)"; rc = borg_flow_take_lunal(true, 4); }
    else { fn = "borg_flow_take(true, 250)"; rc = borg_flow_take(true, 250); }
    keycode_t key = borg_inkey(false);
    printf("DECISION fn=%s rc=%d key=%d goal=%d\n", fn, rc ? 1 : 0, (int)key, borg.goal.type);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    int go = (rc && key == '6') ? 1 : 0;
    printf("OUTCOME goes_for_item=%d\n", go);
    require(go); /* fails before: a far staircase puts the flask "off leash" */
    ok;
}

const char *suite_name = "native-c/takestair-71";
struct test tests[] = {
    {"steps_toward_the_flask_near_its_stair", scenario_take},
    {NULL, NULL}};
