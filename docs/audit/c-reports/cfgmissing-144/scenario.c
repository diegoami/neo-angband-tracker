/* Synthetic native C missing borg.txt fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#144 (cfgmissing-144). */
#include "unit-test.h"
#include "test-utils.h"
#include "init.h"
#include "player.h"
#include "player-birth.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-init.h"
#include "borg/borg.h"
#include <signal.h>
#include "z-util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static term screen;
static char user_dir[1024];

static void crashed(int sig)
{
    (void)sig;
    /* Only async-signal-safe calls here. */
    static const char msg[] = "OUTCOME init_completed=0 crashed=1\nnative-c/cfgmissing-144 finished: 0/1 passed\n";
    if (write(STDOUT_FILENO, msg, sizeof(msg) - 1) < 0) {}
    _exit(3);
}

int setup_tests(void **state)
{
    const char *profile = getenv("CM144_PROFILE");
    const char *mode    = getenv("CM144_MODE");
    if (!profile || !mode) return 1;
    /* "missing": the user directory path runs through a regular file
     * (the profile's borg.txt), so borg.txt there can be neither read nor
     * written, even as root.  "present" (control): the
     * user directory holds the stock borg.txt. */
    if (streq(mode, "missing"))
        strnfmt(user_dir, sizeof(user_dir), "%s/borg.txt/user", profile);
    else
        my_strcpy(user_dir, profile, sizeof(user_dir));
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(144);
    if (!player_make_simple("Human", "Warrior", "CfgMissing144")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    printf("SETUP seed=144 mode=%s borg_txt_dir=%s\n", mode,
           streq(mode, "missing") ? "(missing)" : "(profile)");
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_init(void *state)
{
    /* Start the Borg as the game does; a crash is reported, not lost. */
    /* Point the user directory at the case's directory only now: the
     * game's own start-up (create_needed_dirs) would create a missing one. */
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(user_dir);
    signal(SIGSEGV, crashed);
    borg_init();
    printf("STATE init_failure=%d stop_dlevel=%d\n", borg_init_failure ? 1 : 0,
           borg_cfg ? borg_cfg[BORG_STOP_DLEVEL] : -999);
    printf("OUTCOME init_completed=1 crashed=0\n");
    require(!borg_init_failure);
    ok;
}

const char *suite_name = "native-c/cfgmissing-144";
struct test tests[] = {
    {"borg_init_without_borg_txt", scenario_init},
    {NULL, NULL}};
