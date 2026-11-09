/* Synthetic native C stair-escape fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#34 (stairphase-34).
 *
 * A hurt Human Warrior stands at the dead end of a lit corridor with an
 * awake hill giant next to it.  Depending on the mode the dead end holds an
 * up staircase (upstair; antisummon: the Borg stood in an anti-summon spot
 * ten game turns ago; climb: hurt more),
 * a down staircase (downstair) or plain floor (floor).  The production
 * borg_caution() runs once (it reaches borg_escape() with the danger of the
 * Borg's grid).  Every key the Borg queues is recorded, and the queue is
 * then played through the game's own text UI: textui_process_command()
 * reads each key through the Borg's keypress hook (inkey_hack) and the game
 * loop (run_game_loop) executes the commands, including the level change
 * when the staircase is taken.  The fixture records the depth, the Phase
 * Door scrolls carried and the game messages after the queue is empty. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "game-event.h"
#include "game-world.h"
#include "generate.h"
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
#include "ui-game.h"
#include "ui-input.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40
#define PX 5
#define PY 12

static const char *mode;
static char captured[64][160];
static int n_captured;

static void capture(game_event_type type, game_event_data *data, void *user)
{
    if (n_captured < 64 && data->message.msg)
        my_strcpy(captured[n_captured++], data->message.msg, 160);
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

static int count_scroll(const char *name)
{
    int n = 0;
    for (struct object *o = player->gear; o; o = o->next)
        if (o->kind == lookup_kind(TV_SCROLL, lookup_sval(TV_SCROLL, name)))
            n += o->number;
    return n;
}
#define count_phase() count_scroll("Phase Door")

int setup_tests(void **state)
{
    const char *profile = getenv("SP34_PROFILE");
    mode = getenv("SP34_MODE");
    if (!profile || !mode) return 1;
    if (!streq(mode, "upstair") && !streq(mode, "downstair") && !streq(mode, "floor")
        && !streq(mode, "climb") && !streq(mode, "antisummon")) { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(34);
    if (!player_make_simple("Human", "Warrior", "StairPhase34")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    textui_input_init();
    cmd_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    /* Solid granite with one lit corridor running east from a dead end. */
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        square_set_feat(cave, loc(x, y), FEAT_GRANITE);
    for (int x = PX; x <= PX + 25; ++x) {
        square_set_feat(cave, loc(x, PY), FEAT_FLOOR);
        sqinfo_on(square(cave, loc(x, PY))->info, SQUARE_GLOW);
    }
    if (streq(mode, "upstair") || streq(mode, "climb") || streq(mode, "antisummon"))
        square_set_feat(cave, loc(PX, PY), FEAT_LESS);
    else if (streq(mode, "downstair"))
        square_set_feat(cave, loc(PX, PY), FEAT_MORE);
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 10;
    player_exp_gain(player, player_exp[14] * player->expfact / 100);
    character_dungeon = true;
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    known_item(TV_SCROLL, "Phase Door", 5);
    known_item(TV_POTION, "Cure Light Wounds", 5);
    known_item(TV_SCROLL, "Teleportation", 2);
    /* Let the game sort and combine the pack as it does during play, so
     * the inventory letters the Borg reads are the game's own. */
    player->upkeep->notice |= PN_COMBINE;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    notice_stuff(player);
    update_stuff(player);
    player->chp = player->mhp;
    wiz_light(cave, player, true);
    update_view(cave, player);
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.panels.x = borg.panels.y = 1;
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    track_less.num = track_more.num = 0;
    if (streq(mode, "upstair") || streq(mode, "climb") || streq(mode, "antisummon")) {
        track_less.x[0] = PX; track_less.y[0] = PY; track_less.num = 1;
    } else if (streq(mode, "downstair")) {
        track_more.x[0] = PX; track_more.y[0] = PY; track_more.num = 1;
    }
    borg.c = player->grid; borg.ready_morgoth = 0;
    /* Some game time has passed on this level.  The Borg has never been in
     * an anti-summon spot, except in the "antisummon" case where it marked
     * one ten turns ago (as borg_flow does when it stands in one). */
    borg.time.now = 5000;
    borg.time.level = borg.time.now - 1000;
    borg.time.town = borg.time.now - 1000;
    borg.time.antisummon = streq(mode, "antisummon") ? borg.time.now - 10 : 0;
    /* An awake monster next to the Borg, east along the corridor. */
    struct monster *foe = t_add_monster(cave, loc(PX + 1, PY), "Hill giant");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    if (observe(foe, "foe")) return 1;
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    /* Hurt so that the monster's danger is a little below the hit points
     * left (about 85%), or, in the "climb" control, above them. */
    player->chp = streq(mode, "climb") ? danger * 8 / 10 : danger * 100 / 85;
    if (player->chp > player->mhp || player->chp < 1) { printf("SETUPFAIL hp %d/%d\n", player->chp, player->mhp); return 1; }
    borg_notice(true);
    borg.avoidance = borg.trait[BI_CURHP];
    borg.status.active = true;
    borg_update_entrypoint(true);
    event_add_handler(EVENT_MESSAGE, capture, NULL);
    printf("SETUP seed=34 mode=%s class=%s clevel=%d hp=%d/%d depth=%d feat=%s phase=%d danger=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp, player->depth,
           f_info[square(cave, player->grid)->feat].name, count_phase(), danger);
    return 0;
}

int teardown_tests(void *state)
{
    event_remove_handler(EVENT_MESSAGE, capture, NULL);
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static void print_key(keycode_t k)
{
    if (k == ESCAPE) printf("<Esc>");
    else if (k >= 32 && k < 127) printf("%c", (char)k);
    else printf("<%d>", (int)k);
}

static int scenario(void *state)
{
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    int depth0 = player->depth, phase0 = count_phase(), tele0 = count_scroll("Teleportation");
    struct loc grid0 = player->grid;
    printf("STATE danger=%d avoidance=%d hp=%d escapes=%d antisummon_timer=%d\n", danger,
           borg.avoidance, borg.trait[BI_CURHP], borg.escapes,
           (int)borg_timer(borg.time.antisummon));

    /* The production caution routine, once. */
    bool rc = borg_caution();
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK borg-note <%s>\n", message_str((uint16_t)i));

    /* Record the queue without consuming it. */
    keycode_t keys[64];
    int len = 0;
    for (;;) {
        keycode_t k = borg_inkey(false);
        if (!k || len >= 64) break;
        keys[len++] = borg_inkey(true);
    }
    printf("DECISION caution=%d escapes=%d fleeing=%d leaving=%d keys=%d queue=", rc ? 1 : 0,
           borg.escapes, borg.goal.fleeing ? 1 : 0, borg.goal.leaving ? 1 : 0, len);
    for (int i = 0; i < len; ++i) print_key(keys[i]);
    putchar('\n');
    for (int i = 0; i < len; ++i) borg_keypress(keys[i]);

    /* Play the queue through the game's text UI and game loop until the
     * Borg's queue is empty (the Borg is not asked to think again). */
    int climbed = 0, commands = 0;
    n_captured = 0;
    while (borg_inkey(false) && commands < 16 && !player->is_dead) {
        int d = player->depth;
        textui_process_command();
        run_game_loop();
        ++commands;
        if (player->depth != d) ++climbed;
        for (int m = 0; m < n_captured; ++m)
            printf("TRACK game-message <%s>\n", captured[m]);
        n_captured = 0;
        printf("ENGINE command=%d depth=%d pos=(%d,%d) feat=%s phase=%d teleport=%d queue_left=%d\n", commands,
               player->depth, player->grid.x, player->grid.y,
               f_info[square(cave, player->grid)->feat].name, count_phase(),
               count_scroll("Teleportation"), borg_inkey(false) ? 1 : 0);
    }
    int read_phase = phase0 - count_phase();
    int read_tele = tele0 - count_scroll("Teleportation");
    int read_on_arrival = climbed && (read_phase > 0 || read_tele > 0);
    printf("OUTCOME depth_before=%d depth_after=%d climbed=%d phase_read=%d teleport_read=%d "
           "scroll_read_on_new_level=%d moved=%d commands=%d\n", depth0, player->depth, climbed,
           read_phase, read_tele, read_on_arrival,
           (climbed || !loc_eq(grid0, player->grid)) ? 1 : 0, commands);
    /* Corrected contract: when the Borg takes the staircase it does not
     * also read a scroll on the next level from keys queued on this one. */
    require(!read_on_arrival);
    ok;
}

const char *suite_name = "native-c/stairphase-34";
struct test tests[] = {
    {"stair_escape_queues_only_the_climb", scenario},
    {NULL, NULL}};
