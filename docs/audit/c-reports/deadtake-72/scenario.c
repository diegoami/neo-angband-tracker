/* Synthetic native C dead object entry fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#72 (deadtake-72). */
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
#include "obj-tval.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-think.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-take.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40
#define EAST_KEY 54

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
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
    const char *profile = getenv("DT72_PROFILE");
    const char *mode    = getenv("DT72_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(72);
    if (!player_make_simple("Human", "Warrior", "DeadTake72")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(3, 3));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[10] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp;
    player->csp = player->msp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory();
    borg_do_spell = true;
    borg_notice(true);
    /* The map update has not run: size the detection arrays for a two by
     * two panel map, as it would. */
    borg.panels.x = 2; borg.panels.y = 2;
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=72 mode=%s class=%s clevel=%d hp=%d/%d sp=%d/%d food=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp,
           player->csp, player->msp, player->timed[TMD_FOOD], player->depth);
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
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *mode = getenv("DT72_MODE");
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);

    /* A pile of gold lies four grids east, in view: an item the Lunal take
     * routine wants. Placed by the fixture in the Borg's object list. */
    struct object_kind *kind = lookup_kind(TV_GOLD, lookup_sval(TV_GOLD, "copper"));
    if (!kind) { printf("SETUPFAIL kind\n"); return 1; }
    int slot = 1;
    if (streq(mode, "dead-entry")) {
        /* An earlier object (a pile of gold at the far side of the room) was
         * recorded and then forgotten with the production delete: its entry
         * stays below borg_takes_nxt, zeroed. */
        borg_takes[1].kidx  = kind->kidx;
        borg_takes[1].known = true;
        borg_takes[1].x     = 30;
        borg_takes[1].y     = 18;
        borg_takes[1].value = 100;
        borg_takes[1].tval  = kind->tval;
        borg_grids[18][30].take = 1;
        borg_takes_nxt      = 2;
        borg_takes_cnt      = 1;
        borg_delete_take(1);
        slot = 2;
    }
    borg_takes[slot].kidx  = kind->kidx;
    borg_takes[slot].known = true;
    borg_takes[slot].x     = 7;
    borg_takes[slot].y     = 3;
    borg_takes[slot].value = 100;
    borg_takes[slot].tval  = kind->tval;
    borg_takes_nxt         = slot + 1;
    borg_takes_cnt         = 1;
    printf("STATE borg=(%d,%d) clevel=%d gold_at=(%d,%d) gold_slot=%d takes_nxt=%d takes_cnt=%d\n",
           borg.c.x, borg.c.y, player->lev, borg_takes[slot].x, borg_takes[slot].y, slot,
           borg_takes_nxt, borg_takes_cnt);
    printf("STATE entry1_kidx=%d entry1_tval=%d entry1_at=(%d,%d) entry1_value=%d corner_in_view=%d\n",
           (int)borg_takes[1].kidx, (int)borg_takes[1].tval, borg_takes[1].x, borg_takes[1].y,
           borg_takes[1].value, (borg_grids[0][0].info & BORG_VIEW) ? 1 : 0);

    bool rc = borg_flow_take_lunal(true, 4);
    keycode_t key = borg_inkey(false);
    printf("DECISION rc=%d key=%d goal_type=%d goal_at=(%d,%d)\n", rc ? 1 : 0, (int)key,
           (int)borg.goal.type, borg.goal.g.x, borg.goal.g.y);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    printf("OUTCOME walks_toward_gold=%d\n", (rc && key == EAST_KEY) ? 1 : 0);
    require(rc);
    require(key == EAST_KEY); /* fails before in dead-entry mode: the step goes towards the dead entry's grid (0,0) */
    ok;
}

const char *suite_name = "native-c/deadtake-72";
struct test tests[] = {
    {"lunal_take_skips_dead_entries", scenario_take},
    {NULL, NULL}};
