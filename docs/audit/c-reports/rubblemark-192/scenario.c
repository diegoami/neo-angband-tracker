/* Synthetic native C removed-rubble map fixture, excluded from the source
 * fix. Issue: diegoami/neo-angband-tracker#192 (rubblemark-192).
 *
 * The Borg's "You have removed the ..." handler is meant to convert the
 * rubble grids around the Borg to broken doors so a lightless Borg does
 * not treat freshly dug ground as rubble. The original loops a 2x2 block
 * while its body only ever tests the goal grid; the fixture digs real
 * rubble through the production dig command, hands the game's captured
 * message to the Borg's production parser (borg_parse), and records the
 * Borg's map conversions. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "cmds.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "object.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-prefs.h"
#include "ui-term.h"
#include "z-virt.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-item.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-messages.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40

static char captured[16][256];
static int  n_captured;

static void capture(game_event_type type, game_event_data *data, void *user)
{
    if (n_captured < 16 && data->message.msg)
        my_strcpy(captured[n_captured++], data->message.msg, 256);
}

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

/* The rubble neighbourhood the handler is meant to convert, as (x, y). */
static const struct loc rubble_spots[] = {
    {12, 9},  /* north: the dig target */
    {13, 9},  /* north-east */
    {13, 10}, /* east */
    {13, 11}, /* south-east */
    {11, 9},  /* north-west */
};
#define N_SPOTS ((int)(sizeof rubble_spots / sizeof rubble_spots[0]))

int setup_tests(void **state)
{
    const char *profile = getenv("M192_PROFILE");
    const char *mode    = getenv("M192_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(192);
    if (!player_make_simple("Human", "Warrior", "Rubblemark192")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    /* loc() takes x first: the Borg stands at (x=12, y=10). */
    player_place(cave, player, loc(12, 10));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 8;
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    known_item(TV_FOOD, "Ration of Food", 5);
    /* A digger so the rubble clears on the fixed roll. The dark mode takes
     * off the birth torch so the Borg carries no light at all; the control
     * keeps it (the handler skips everything while the Borg has light). */
    struct object *shovel = known_item(TV_DIGGING, "Shovel", 1);
    inven_wield(shovel, wield_slot(shovel));
    if (streq(mode, "dark")) {
        int light_slot = slot_by_name(player, "light");
        struct object *torch = player->body.slots[light_slot].obj;
        if (torch) inven_takeoff(torch);
    } else {
        struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
        torch->timeout = 2000;
        inven_wield(torch, wield_slot(torch));
    }
    player_exp_gain(player, player_exp[14] * player->expfact / 100);
    /* Lay the rubble in the game cave. */
    for (int i = 0; i < N_SPOTS; ++i)
        square_set_feat(cave, rubble_spots[i], FEAT_RUBBLE);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true); borg_notice_player();
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid;
    borg.time.level = borg.time.now;
    borg.time.town  = borg.time.now;
    borg.status.active = true;
    borg_update_entrypoint(true);
    /* The Borg's goal sits on clear floor outside the neighbourhood, as it
     * may in ordinary play; the handler's slip marks only that goal grid. */
    borg.goal.g = loc(12, 6);
    printf("SETUP seed=192 mode=%s class=%s clevel=%d light=%d goal=(%d,%d)\n",
           mode, player->class->name, player->lev, borg.trait[BI_LIGHT] ? 1 : 0,
           borg.goal.g.x, borg.goal.g.y);
    return 0;
}

int teardown_tests(void *state)
{
    event_remove_handler(EVENT_MESSAGE, capture, NULL);
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_dig(void *state)
{
    const char *mode = getenv("M192_MODE");
    n_captured = 0;
    event_add_handler(EVENT_MESSAGE, capture, NULL);
    /* Fixture intervention: a high digging skill (the Shovel plus strength
     * in ordinary play) so the fixed roll clears the rubble. */
    player->state.skills[SKILL_DIGGING] = 400;
    /* The production dig command, one grid north: the real message the
     * player sees comes from the game's own dig code. */
    struct command cmd;
    memset(&cmd, 0, sizeof cmd);
    cmd.code = CMD_TUNNEL;
    cmd_set_arg_direction(&cmd, "direction", 8);
    do_cmd_tunnel(&cmd);
    event_remove_handler(EVENT_MESSAGE, capture, NULL);
    const char *removed = NULL;
    for (int i = 0; i < n_captured; ++i) {
        printf("TRACK game-message <%s>\n", captured[i]);
        if (prefix(captured[i], "You have removed the rubble"))
            removed = captured[i];
    }
    printf("STATE removed=%d light=%d\n", removed ? 1 : 0,
           borg.trait[BI_LIGHT] ? 1 : 0);
    if (!removed) {
        /* The dig roll missed; the fixed seed must make this impossible. */
        require(0);
        ok;
    }
    /* The Borg's own screen reader hands each message line to the
     * production parser; the captured text is the exact line the game
     * printed. */
    borg_parse(removed);
    borg_parse(NULL);
    int broken = 0, rubble = 0;
    for (int i = 0; i < N_SPOTS; ++i) {
        int f = borg_grids[rubble_spots[i].y][rubble_spots[i].x].feat;
        printf("MAP grid=(%d,%d) feat=%d\n", rubble_spots[i].x,
               rubble_spots[i].y, f);
        if (f == FEAT_BROKEN) broken++;
        else if (f == FEAT_RUBBLE) rubble++;
    }
    printf("OUTCOME broken=%d rubble=%d light=%d\n", broken, rubble,
           borg.trait[BI_LIGHT] ? 1 : 0);
    if (streq(mode, "control")) {
        /* Lit: the handler skips everything; the map must be untouched. */
        require(broken == 0);
        require(rubble == N_SPOTS);
    } else {
        /* Dark: the contract is every rubble grid around the Borg becomes
         * a broken door on the Borg's own map. */
        require(broken == N_SPOTS);
        require(rubble == 0);
    }
    ok;
}

const char *suite_name = "native-c/rubblemark-192";
struct test tests[] = {
    {"removed_rubble_is_marked_around_the_borg", scenario_dig},
    {NULL, NULL}};
