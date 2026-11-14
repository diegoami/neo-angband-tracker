/* Synthetic native C dive-gate fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#629 (divegate-629).
 *
 * borg_flow_stair_more() wraps all four dive gates (preparedness, hunger,
 * sell-crap, no-light) inside "if (track_less.num)", so a borg that has
 * seen no up staircase yet — exactly the state after a monster
 * Teleport-Level or a Deep Descent — dives with no checks at all. The
 * fixture stands a lightless Warrior one grid from a down staircase with
 * no up staircase anywhere ("dark"), and the same borg with the up
 * staircase also seen ("control"), and calls the production
 * borg_flow_stair_more(). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
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
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
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
    const char *profile = getenv("D629_PROFILE");
    const char *mode    = getenv("D629_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(629);
    if (!player_make_simple("Human", "Warrior", "Divegate629")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    /* loc() takes x first: the Borg stands at (x=12, y=10). */
    player_place(cave, player, loc(12, 10));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 8;
    player_exp_gain(player, player_exp[15] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    known_item(TV_FOOD, "Ration of Food", 5);
    /* No light at all: the birth torch comes off, so the no-light dive
     * gate is the gate under test. */
    {
        int light_slot = slot_by_name(player, "light");
        struct object *torch = player->body.slots[light_slot].obj;
        if (torch) inven_takeoff(torch);
    }
    /* The down staircase, one grid north, tracked as seen. The control
     * has also seen an up staircase two grids east; the dark mode has
     * seen no up staircase at all. */
    square_set_feat(cave, loc(12, 9), FEAT_MORE);
    if (streq(mode, "control"))
        square_set_feat(cave, loc(14, 10), FEAT_LESS);
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
    track_more.x[track_more.num] = 12; track_more.y[track_more.num] = 9;
    track_more.num++;
    if (streq(mode, "control")) {
        track_less.x[track_less.num] = 14; track_less.y[track_less.num] = 10;
        track_less.num++;
    }
    borg.c = player->grid;
    borg.avoidance = player->chp;
    borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town  = borg.time.now;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=629 mode=%s class=%s clevel=%d light=%d depth=%d "
           "up=%d down=%d food=%d\n",
           mode, player->class->name, player->lev,
           borg.trait[BI_LIGHT] ? 1 : 0, borg.trait[BI_CDEPTH],
           track_less.num, track_more.num, borg.trait[BI_FOOD]);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_dive(void *state)
{
    const char *mode = getenv("D629_MODE");
    keycode_t keys[16];
    int len = 0;
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    uint16_t before = messages_num();
    /* The production dive decision, with the production arguments used by
     * the ordinary dungeon brain (not fleeing, not brave). */
    bool rc = borg_flow_stair_more(GOAL_FLEE, false, false);
    keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < 16) keys[len++] = key;
    char keybuf[17];
    for (int i = 0; i < len; ++i) keybuf[i] = (char)keys[i];
    keybuf[len] = 0;
    printf("DECISION rc=%d keys=\"%s\"\n", rc ? 1 : 0, keybuf);
    uint16_t after = messages_num();
    int flow_note = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (prefix(m, "# Flowing toward down-stairs")) flow_note = 1;
    }
    printf("OUTCOME flowed=%d keys=%d up=%d light=%d\n", rc ? 1 : 0, len,
           track_less.num, borg.trait[BI_LIGHT] ? 1 : 0);
    /* The contract: a lightless borg may not dive, whether or not an up
     * staircase has been seen. The control (up stair seen) already honours
     * that on both variants; the dark mode must honour it too. */
    require(!rc);
    require(len == 0);
    require(!flow_note);
    (void)mode;
    ok;
}

const char *suite_name = "native-c/divegate-629";
struct test tests[] = {
    {"a_lightless_borg_may_not_dive_without_seen_gates", scenario_dive},
    {NULL, NULL}};
