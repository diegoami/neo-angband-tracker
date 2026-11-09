/* Synthetic native C respawn fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#52 (rebirthkey-52).
 *
 * A previous character is made natively.  The Borg runs with the game's
 * cheat_live option on and borg_cheat_death off, with its respawn race and
 * class set as borg.txt's borg_respawn_race/borg_respawn_class would set
 * them.  A fatal hit is dealt through take_hit(); the game asks "Die?" and
 * every key from then on is produced by the Borg's own keypress hook
 * (borg_entry_point via inkey_hack): it answers the prompt, queues the
 * respawn keys in borg_reincarnate_start(), and those keys are consumed by
 * the real death menu (death_screen) and the real birth screens
 * (textui_do_birth).  The character that comes out of the birth screens
 * is printed.  The save/re-initialise step that close_game()/play_game()
 * perform between the death menu and the birth screens is skipped. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "ui-birth.h"
#include "ui-death.h"
#include "ui-input.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-init.h"
#include "borg/borg-io.h"
#include "borg/borg-trait.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40

struct rk_case {
    const char *mode;
    const char *prev_race, *prev_class;
    int race, class;
};

static const struct rk_case cases[] = {
    { "race8",        "Human", "Warrior", 8, 0 },
    { "class8",       "Human", "Warrior", 0, 8 },
    { "race7",        "Human", "Warrior", 7, 0 },
    { "dwarfpriest",  "Human", "Warrior", 5, 3 },
    { "humanwarrior", "Elf",   "Mage",    0, 0 },
};

static const struct rk_case *cur;
static struct keypress (*borg_hook)(int flush_first);
static int keys_seen;
static const char *stage = "die-prompt";
static char typed[256];
static size_t typed_len;

/* Every key the game asks for comes from the Borg's own hook; the fixture
 * only records it and stops the run if the Borg's queue runs dry while the
 * respawn is still in progress (it would otherwise answer ESCAPE forever). */
static struct keypress rk_hook(int flush_first)
{
    struct keypress k;
    if (keys_seen > 0 && borg_inkey(false) == 0) {
        printf("KEYS starved stage=%s after=%d\n", stage, keys_seen);
        printf("OUTCOME starved=1\n");
        fflush(stdout);
        exit(3);
    }
    k = borg_hook(flush_first);
    keys_seen++;
    if (typed_len + 8 < sizeof(typed)) {
        if (k.code == KC_ENTER) typed_len += (size_t) snprintf(typed + typed_len, 8, "<CR>");
        else if (k.code == ESCAPE) typed_len += (size_t) snprintf(typed + typed_len, 8, "<ESC>");
        else if (k.code >= 32 && k.code < 127) typed[typed_len++] = (char) k.code;
        else typed_len += (size_t) snprintf(typed + typed_len, 8, "<%u>", (unsigned) k.code);
        typed[typed_len] = 0;
    }
    printf("KEY stage=%s n=%d code=%u%s%c\n", stage, keys_seen, (unsigned) k.code,
        (k.code >= 32 && k.code < 127) ? " char=" : "",
        (k.code >= 32 && k.code < 127) ? (char) k.code : ' ');
    return k;
}

int setup_tests(void **state)
{
    const char *profile = getenv("RK52_PROFILE");
    const char *mode    = getenv("RK52_MODE");
    if (!profile || !mode) return 1;
    for (size_t i = 0; i < N_ELEMENTS(cases); i++)
        if (streq(cases[i].mode, mode)) cur = &cases[i];
    if (!cur) { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(52);
    if (!player_make_simple(cur->prev_race, cur->prev_class, "Rebirth")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    textui_input_init();
    ui_init_birthstate_handlers();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 1;
    character_dungeon = true;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    /* The game option that offers "Die?" instead of ending the game. */
    option_set("cheat_live", true);
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || !borg.status.cheat_death) {
        printf("SETUPFAIL borg\n"); return 1;
    }
    /* As borg_respawn_race / borg_respawn_class in borg.txt would. */
    borg_cfg[BORG_RESPAWN_RACE]  = cur->race;
    borg_cfg[BORG_RESPAWN_CLASS] = cur->class;
    borg.status.active = true;
    printf("STATE prev_race=%d prev_class=%d want_race=%d want_class=%d\n",
        player->race->ridx, player->class->cidx, cur->race, cur->class);
    printf("PREPARED previous %s %s; respawn wanted race %d (%s) class %d (%s)\n",
        player->race->name, player->class->name,
        cur->race, player_id2race(cur->race)->name,
        cur->class, player_id2class(cur->class)->name);
    return 0;
}

int teardown_tests(void *state)
{
    return 0;
}

static int test_respawn(void *state)
{
    /* The Borg's own keypress hook, as when it is playing. */
    borg_update_entrypoint(true);
    borg_hook = inkey_hack;
    inkey_hack = rk_hook;
    /* A fatal hit: the game asks "Die?" because cheat_live is on. */
    take_hit(player, player->chp + 1000, "a fixture blow");
    printf("TRACK after_hit is_dead=%d respawning=%d\n", player->is_dead ? 1 : 0,
        borg.status.respawning ? 1 : 0);
    if (!player->is_dead || !borg.status.respawning) {
        printf("OUTCOME starved=0 respawn_started=0\n");
        inkey_hack = NULL;
        eq(player->is_dead, true);
        ok;
    }
    /* What close_game() does next for a dead character. */
    stage = "death-menu";
    death_knowledge(player);
    death_screen();
    printf("TRACK after_death_menu play_again=%d\n", play_again ? 1 : 0);
    /* start_game(): no living character, so the birth screens run. */
    stage = "birth";
    character_generated = false;
    textui_do_birth();
    inkey_hack = NULL;
    printf("TRACK typed %s\n", typed);
    printf("DECISION created %s %s named %s; wanted %s %s\n",
        player->race->name, player->class->name, player->full_name,
        player_id2race(cur->race)->name, player_id2class(cur->class)->name);
    {
        int good = player->race->ridx == cur->race && player->class->cidx == cur->class;
        printf("OUTCOME starved=0 respawn_started=1 race=%d class=%d want_race=%d want_class=%d"
            " queue_left=%d keys=%d right_character=%d\n",
            player->race->ridx, player->class->cidx, cur->race, cur->class,
            borg_inkey(false) != 0 ? 1 : 0, keys_seen, good);
        eq(play_again, true);
        eq(player->race->ridx, cur->race);
        eq(player->class->cidx, cur->class);
    }
    ok;
}

const char *suite_name = "native-c/rebirthkey-52";
struct test tests[] = {
    { "respawn", test_respawn },
    { NULL, NULL }
};
