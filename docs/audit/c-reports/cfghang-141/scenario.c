/* Synthetic native C borg.txt parser-hang fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#141 (cfghang-141). */
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static term screen;
static char user_dir[1024];

/* The line appended to the stock borg.txt in each case. */
static const char *extra_line(const char *mode)
{
    if (streq(mode, "word"))   return "borg_stop_dlevel = off";
    if (streq(mode, "suffix")) return "borg_stop_dlevelx 5";
    return "borg_stop_dlevel = 7"; /* "valid" control */
}

static void watchdog(int sig)
{
    (void)sig;
    /* Only async-signal-safe calls here. */
    static const char msg[] = "OUTCOME init_completed=0 hung=1\nnative-c/cfghang-141 finished: 0/1 passed\n";
    if (write(STDOUT_FILENO, msg, sizeof(msg) - 1) < 0) {}
    _exit(2);
}

int setup_tests(void **state)
{
    const char *profile = getenv("CH141_PROFILE");
    const char *mode    = getenv("CH141_MODE");
    if (!profile || !mode) return 1;
    /* A per-case user directory holding the stock borg.txt plus one line. */
    strnfmt(user_dir, sizeof(user_dir), "%s/%s", profile, mode);
    mkdir(user_dir, 0755);
    {
        char src[1024], dst[1024], line[1024];
        strnfmt(src, sizeof(src), "%s/borg.txt", profile);
        strnfmt(dst, sizeof(dst), "%s/borg.txt", user_dir);
        FILE *in = fopen(src, "r"), *out = fopen(dst, "w");
        if (!in || !out) { printf("SETUPFAIL borg.txt\n"); return 1; }
        while (fgets(line, sizeof(line), in)) fputs(line, out);
        fprintf(out, "%s\n", extra_line(mode));
        fclose(in); fclose(out);
    }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(user_dir);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(141);
    if (!player_make_simple("Human", "Warrior", "CfgHang141")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    printf("SETUP seed=141 mode=%s line=\"%s\"\n", mode, extra_line(mode));
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_init(void *state)
{
    /* Start the Borg as the game does; a watchdog stops a hung parse. */
    signal(SIGALRM, watchdog);
    alarm(20);
    borg_init();
    alarm(0);
    printf("STATE init_failure=%d stop_dlevel=%d\n", borg_init_failure ? 1 : 0,
           borg_cfg ? borg_cfg[BORG_STOP_DLEVEL] : -999);
    printf("OUTCOME init_completed=1 hung=0\n");
    require(!borg_init_failure);
    ok;
}

const char *suite_name = "native-c/cfghang-141";
struct test tests[] = {
    {"borg_init_reads_borg_txt", scenario_init},
    {NULL, NULL}};
