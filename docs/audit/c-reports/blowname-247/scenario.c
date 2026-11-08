/* Synthetic native C blow-effect table lookup diagnostic, excluded from the source fix. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-init.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("BW247_PROFILE");
    if (!profile) return 1;
    printf("SETUPSTEP profile\n");
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Warrior", "BlowNameLookup")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    printf("SETUP seed=1000 class=%s\n", player->class->name);
    return 0;
}

int teardown_tests(void *state)
{
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("BW247_MODE");
    if (mode && !strcmp(mode, "known")) {
        /* Control: only names that are in the table. BLACK_BREATH is the
         * last real entry, so the loop runs through every entry before
         * the NULL end marker (the bound the correction changes) and
         * returns before reaching it in both versions. */
        int first = borg_mon_blow_effect("HURT");
        int last = borg_mon_blow_effect("BLACK_BREATH");
        printf("KNOWN value=%d expected=%d\n", first, MONBLOW_HURT);
        printf("LAST value=%d expected=%d\n", last, MONBLOW_BLACK_BREATH);
        eq(first, MONBLOW_HURT);
        eq(last, MONBLOW_BLACK_BREATH);
        ok;
    }
    /* In-table lookup still resolves. */
    int hurt = borg_mon_blow_effect("HURT");
    printf("KNOWN value=%d expected=%d\n", hurt, MONBLOW_HURT);
    eq(hurt, MONBLOW_HURT);

    /* A blow name absent from the table must return NONE, not walk into
     * the table's NULL end entry (strcmp against NULL). This is the name
     * a modded blow_effects.txt could supply. */
    int unknown = borg_mon_blow_effect("MADE_UP_BLOW_NAME");
    printf("UNKNOWN value=%d expected=%d\n", unknown, MONBLOW_NONE);
    eq(unknown, MONBLOW_NONE);
    ok;
}
const char *suite_name = "native-c/blowname-247";
struct test tests[] = {{"unknown_blow_name_returns_none", scenario}, {NULL, NULL}};
