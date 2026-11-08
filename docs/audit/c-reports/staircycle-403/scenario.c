/* Synthetic native C stair-cycle decision fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#403 (staircycle-403). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-power.h"
#include "borg/borg-think-dungeon.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

static void reset_notes(void) { }

static void print_notes(void)
{
    uint16_t total = messages_num();
    uint16_t show = total < 14 ? total : 14;
    for (uint16_t i = 0; i < show; ++i)
        printf("NOTE %02d %s\n", (int)i, message_str(i));
}

int setup_tests(void **state)
{
    const char *profile = getenv("STAIR403_PROFILE");
    if (!profile) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(1045);
    if (!player_make_simple("Human", "Warrior", "StairCycle403")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 24, 24, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 9;
    character_dungeon = true;
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* Underfoot up stair; a known down stair three tiles east. */
    square_set_feat(cave, loc(12, 12), FEAT_LESS);
    square_set_feat(cave, loc(15, 12), FEAT_MORE);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH; update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA;
    update_stuff(player);
    borg_equipment();
    borg_inventory();
    borg_notice(true);
    memcpy(safe_items, borg_items, QUIVER_END * sizeof(borg_item));
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    /* Leave the borg's knowledge of the two stair squares at floor: the
     * munchkin underfoot cheat must discover them from the native cave,
     * as it does in production. */
    borg_grids[12][12].feat = FEAT_FLOOR;
    borg_grids[15][12].feat = FEAT_FLOOR;
    update_view(cave, player);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.status.active = true;
    borg_update_entrypoint(true);
    borg_cfg[BORG_MUNCHKIN_START] = true;
    printf("SETUP seed=1045 class=%s depth=%d hp=%d\n",
           player->class->name, player->depth, player->chp);
    /* STAIR403_MODE=clean: run the healthy control on its own, in a fresh
     * process, so its complete log can be compared byte for byte. */
    const char *mode = getenv("STAIR403_MODE");
    if (mode && streq(mode, "clean")) {
        tests[0] = tests[1];
        tests[1].name = NULL;
        tests[1].func = NULL;
    }
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

/* Reset the decision state that a previous scenario may have mutated. */
static void reset_decision_state(void)
{
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    track_less.num = 0; track_more.num = 0;
    borg.munchkin_mode = false;
    borg.times_twitch = 0;
    borg.no_retreat = 0;
    borg.c = player->grid;
    /* Restore the two stair grids to undiscovered: a previous scenario's
     * munchkin pass may have stamped ag->feat and tracked them, and the
     * underfoot discovery cheat must run from the same premise each time. */
    borg_grids[12][12].feat = FEAT_FLOOR;
    borg_grids[15][12].feat = FEAT_FLOOR;
    /* The borg_think() prologue runs borg_notice(true), then borg_update()
     * (screen/state scan), then evaluates borg_power before descending into
     * borg_think_dungeon(). The screen scan needs a live game screen, which
     * this fixture does not drive; replay only the state those branches
     * consume: panel-sized detection arrays (the exploration flows
     * dereference borg_detect_door) and the world power evaluation. */
    borg.panels.x = 1;
    borg.panels.y = 2; /* a 66-row cave behind one full-height panel */
    borg_alloc_detection();
    borg.power = borg_power();
    reset_notes();
}

static void drain_keys(char *out, size_t size)
{
    size_t n = 0;
    out[0] = '\0';
    for (int i = 0; i < 16; ++i) {
        keycode_t k = borg_inkey(true);
        if (!k) break;
        if (n + 2 < size) { out[n++] = (k >= 32 && k < 127) ? (char)k : '?'; if (k == 27) { out[n++] = '!'; } }
    }
    out[n] = '\0';
}

/* A Warrior at depth 9 (past the munchkin depth target), standing on a known
 * up stair, poisoned and hallucinating. Stair travel does not age the world
 * clock (level entry grants minimum move energy and the world tick is skipped
 * during level generation), so shuttling between stairs never lets these
 * timed ailments wear off. The munchkin branch must hand the turn back to
 * the ordinary recovery path instead of directing the borg at stairs. */
static int scenario_ailing(void *state)
{
    char keys[32];
    bool stair_directed;

    player->timed[TMD_POISONED] = 20;
    player->timed[TMD_IMAGE] = 14;
    reset_decision_state();

    bool rc = borg_think_dungeon();
    drain_keys(keys, sizeof(keys));
    stair_directed = borg.munchkin_mode
        || (strchr(keys, '<') != NULL) || (strchr(keys, '>') != NULL);

    printf("STATE poisoned=%d hallucinating=%d depth=%d munchkin_mode=%d rc=%d keys=%s\n",
           borg.trait[BI_ISPOISONED] ? 1 : 0, borg.trait[BI_ISIMAGE] ? 1 : 0,
           borg.trait[BI_CDEPTH], borg.munchkin_mode ? 1 : 0, rc ? 1 : 0,
           keys[0] ? keys : "(none)");
    print_notes();
    printf("OUTCOME stair_directed=%d\n", stair_directed ? 1 : 0);
    require(borg.trait[BI_ISPOISONED]);
    require(borg.trait[BI_ISIMAGE]);
    require(!stair_directed);
    ok;
}

/* Control: the same position and depth without the ailments. Munchkin mode
 * must keep handling the turn (climb/dive logic is unchanged for healthy
 * borgs), so the correction does not disable munchkin play. */
static int scenario_clean(void *state)
{
    char keys[32];

    player->timed[TMD_POISONED] = 0;
    player->timed[TMD_IMAGE] = 0;
    reset_decision_state();

    bool rc = borg_think_dungeon();
    drain_keys(keys, sizeof(keys));

    printf("STATE poisoned=%d hallucinating=%d depth=%d munchkin_mode=%d rc=%d keys=%s\n",
           borg.trait[BI_ISPOISONED] ? 1 : 0, borg.trait[BI_ISIMAGE] ? 1 : 0,
           borg.trait[BI_CDEPTH], borg.munchkin_mode ? 1 : 0, rc ? 1 : 0,
           keys[0] ? keys : "(none)");
    print_notes();
    printf("OUTCOME munchkin_handled=%d\n", borg.munchkin_mode ? 1 : 0);
    require(!borg.trait[BI_ISPOISONED]);
    require(!borg.trait[BI_ISIMAGE]);
    require(borg.munchkin_mode);
    ok;
}

const char *suite_name = "native-c/staircycle-403";
struct test tests[] = {
    {"stair_travel_gated_when_ailing", scenario_ailing},
    {"munchkin_open_when_clean", scenario_clean},
    {NULL, NULL}};
