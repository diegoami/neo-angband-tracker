/* Synthetic native C monster-status message fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#197 (painmsg-197).
 *
 * A kobold stands next to the Borg.  The game's own monster timed-effect
 * code (mon_inc_timed / mon_clear_timed) and monster-message code
 * (add_monster_message / show_monster_messages) produce a real message
 * line; an EVENT_MESSAGE handler records the exact text the game printed,
 * and the scenario hands that text to the Borg's message parser
 * (borg_parse), the function its screen reader calls with each message
 * line.  The Borg's message queue is then read back, and one Borg map
 * update (borg_update) shows whether the queued reaction is matched to the
 * kobold on the Borg's monster list. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
#include "mon-msg.h"
#include "mon-timed.h"
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
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
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
static struct monster *kobold;
static const char *mode;

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

int setup_tests(void **state)
{
    const char *profile = getenv("PM197_PROFILE");
    mode = getenv("PM197_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(197);
    if (!player_make_simple("Human", "Warrior", "PainMsg197")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[9] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
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
    borg.c = player->grid; borg.goal.g = player->grid;
    borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    /* A kobold two grids east of the Borg, seen and on its monster list. */
    kobold = t_add_monster(cave, loc(12, 12), "kobold");
    if (!kobold) { printf("SETUPFAIL kobold\n"); return 1; }
    mflag_on(kobold->mflag, MFLAG_AWARE); update_mon(kobold, cave, true);
    if (!monster_is_visible(kobold)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(kobold->grid.y, kobold->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[kobold->grid.y][kobold->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg.status.active = true;
    event_add_handler(EVENT_MESSAGE, capture, NULL);
    printf("SETUP seed=197 mode=%s class=%s clevel=%d depth=%d kobold=(%d,%d) borg_kill=%d\n",
           mode, player->class->name, player->lev, player->depth,
           kobold->grid.x, kobold->grid.y, k);
    return 0;
}

int teardown_tests(void *state)
{
    event_remove_handler(EVENT_MESSAGE, capture, NULL);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

/* Let the game print its pending monster messages. */
static void game_shows(void)
{
    show_monster_messages();
}

static int scenario(void *state)
{
    int mon_tmd = -1;
    bool shape = false;
    /* Start the effect first (its own message is not under test) ... */
    if (streq(mode, "held")) mon_tmd = MON_TMD_HOLD;
    else if (streq(mode, "uncommand")) mon_tmd = MON_TMD_COMMAND;
    else if (streq(mode, "stun")) mon_tmd = MON_TMD_STUN;
    else if (streq(mode, "confused")) mon_tmd = MON_TMD_CONF;
    if (mon_tmd >= 0) {
        bool took = mon_inc_timed(kobold, mon_tmd, 5, MON_TMD_FLG_NOTIFY);
        game_shows();
        printf("STATE effect_started=%d\n", took ? 1 : 0);
        for (int i = 0; i < n_captured; ++i) printf("TRACK start-message <%s>\n", captured[i]);
        n_captured = 0;
        /* ... then end it: the game prints the message under test. */
        mon_clear_timed(kobold, mon_tmd, MON_TMD_FLG_NOTIFY);
    } else if (streq(mode, "command")) {
        bool took = mon_inc_timed(kobold, MON_TMD_COMMAND, 5, MON_TMD_FLG_NOTIFY);
        printf("STATE effect_started=%d\n", took ? 1 : 0);
    } else if (streq(mode, "hurt")) {
        /* The game's pain message for 3 damage to the kobold, as printed
         * after a hit (message_pain, mon-msg.c). */
        kobold->hp -= 3;
        message_pain(kobold, 3);
        printf("STATE effect_started=1\n");
    } else if (streq(mode, "puff")) {
        /* The message a hit-and-run monster's blow queues (mon-attack.c
         * calls add_monster_message the same way). */
        shape = add_monster_message(kobold, MON_MSG_HIT_AND_RUN, true);
        printf("STATE effect_started=%d\n", shape ? 1 : 0);
    } else if (streq(mode, "shapefail")) {
        /* Fixture intervention: the game's monster-message code is asked
         * for its failed-shapechange message directly; the shapechange
         * itself is not run. */
        shape = add_monster_message(kobold, MON_MSG_SHAPE_FAIL, true);
        printf("STATE effect_started=%d\n", shape ? 1 : 0);
    }
    game_shows();

    int before = borg_msg_num;
    for (int i = 0; i < n_captured; ++i) {
        printf("TRACK game-message <%s>\n", captured[i]);
        borg_parse(captured[i]);
    }
    borg_parse(NULL);
    int queued = borg_msg_num - before;
    int pain = 0;
    for (int i = before; i < borg_msg_num; ++i) {
        const char *r = borg_msg_buf + borg_msg_pos[i];
        printf("TRACK borg-reaction <%s>\n", r);
        if (prefix(r, "PAIN:")) pain = 1;
    }
    if (!queued) printf("TRACK borg-reaction <none>\n");

    /* One Borg map update: its message passes try to match each queued
     * reaction to a monster on its list. */
    int end = borg_msg_num;
    borg_update();
    int matched = 0;
    /* borg_update empties the queue count at its end; the entries and
     * their use marks stay in the buffers. */
    for (int i = before; i < end; ++i) {
        printf("TRACK reaction-use <%s> use=%d\n", borg_msg_buf + borg_msg_pos[i], borg_msg_use[i]);
        if (borg_msg_use[i] >= 2 && borg_msg_use[i] <= 4) matched = 1;
    }
    printf("STATE game_messages=%d reactions=%d\n", n_captured, queued);
    printf("OUTCOME recognised=%d matched_to_kobold=%d\n", pain, matched);
    require(n_captured >= 1);
    require(pain);
    ok;
}

const char *suite_name = "native-c/painmsg-197";
struct test tests[] = {
    {"monster_status_message_recognised", scenario},
    {NULL, NULL}};
