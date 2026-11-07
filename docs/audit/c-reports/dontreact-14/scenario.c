/* Synthetic native C knockout recovery diagnostic, excluded from the source fix. */
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
#include "borg/borg-init.h"
#include "borg/borg-io.h"
#include "borg/borg-messages-react.h"
#include "borg/borg-messages.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;

int setup_tests(void **state)
{
    const char *profile = getenv("KO14_PROFILE");
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
    if (!player_make_simple("Human", "Warrior", "KnockoutReact")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s\n", player->class->name);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    /* Baseline: a normal message queues into the borg's buffer. */
    borg_react("You feel great.", "TEST:baseline");
    int after_baseline = borg_msg_num;
    printf("STATE after_baseline=%d\n", after_baseline);
    require(after_baseline == 1);

    /* The knockout message, parsed the way the game delivers it. */
    borg_parse("You have been knocked out.");
    borg_parse(NULL);

    /* While the flag is up, reactions are gated. */
    borg_react("You feel less great.", "TEST:during-ko");
    int after_ko = borg_msg_num;
    printf("STATE after_ko=%d\n", after_ko);
    require(after_ko == 1);

    /* The production pump (borg.c: borg_parse(NULL); borg.dont_react = false;). */
    borg_parse(NULL);
    borg.dont_react = false;

    /* After the pump, reactions must resume: the recovered borg cannot
     * stay deaf to combat messages. */
    borg_react("You feel great again.", "TEST:after-recovery");
    int after_recovery = borg_msg_num;
    printf("STATE after_recovery=%d\n", after_recovery);
    printf("OUTCOME recovered=%d\n", after_recovery > after_ko ? 1 : 0);
    require(after_recovery > after_ko);
    ok;
}
const char *suite_name = "native-c/dontreact-14";
struct test tests[] = {{"knockout_flag_clears_on_pump", scenario}, {NULL, NULL}};
