/* Synthetic native C level-change message fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#198 (iswarded-198).
 *
 * The Borg reads game messages from the top screen line.  When a Word of
 * Recall activates in the same game turn as a monster's action, the game
 * shows the pending messages with -more- before it leaves the level; the
 * Borg stores that line and parses it at its next keypress, which is the
 * first command prompt on the new level, before its map update moves its
 * own position to the new level.
 *
 * Real code throughout: monster turns (process_monsters), the recall
 * activation (process_world), the game's own message line and -more-
 * prompt (display_message / message_flush from ui-input.c on an 80x24
 * terminal), town generation (prepare_next_level) and the Borg's keypress
 * hook.  Fixture interventions: the arena and the runes the Borg laid
 * earlier (with its own glyph records), the monster placement, an active
 * Word of Recall at one tick, the EVENT_MESSAGE_FLUSH signal that the
 * game's static on_leave_level() sends, and Escape answers at old-level
 * prompts between monster rounds. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-event.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-make.h"
#include "mon-move.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "trap.h"
#include "ui-input.h"
#include "ui-output.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-flow-glyph.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-messages.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static const char *mode;
static struct monster *enemy;
static int broken_messages;
static int hit_messages;
static struct keypress (*borg_hook)(int flush_first);

/* Observation only: report each message the game sends. */
static void message_log(game_event_type type, game_event_data *data, void *user)
{
    const char *text = data->message.msg;
    if (!text) return;
    printf("ENGINE turn=%ld level=%dx%d: %s\n", (long)turn, cave->width,
        cave->height, text);
    if (prefix(text, "The rune of protection is broken!")) ++broken_messages;
    if (strstr(text, " bites you.") || strstr(text, " hits you.")) ++hit_messages;
}

/* Observation only: show the top line and the key the Borg answers with. */
static struct keypress observed_hook(int flush_first)
{
    char line[96];
    int x, y;
    for (x = 0; x < 80; ++x) {
        int a; wchar_t c;
        Term_what(x, 0, &a, &c);
        line[x] = (c > 0 && c < 128) ? (char)c : '?';
    }
    while (x > 0 && line[x - 1] == ' ') --x;
    line[x] = '\0';
    Term_locate(&x, &y);
    printf("SCREEN level=%dx%d prompt=%s top=\"%s\"\n", cave->width,
        cave->height, inkey_flag ? "command" : "more", line);
    struct keypress key = borg_hook(flush_first);
    printf("KEY %d\n", (int)key.code);
    return key;
}

/* An old-level command prompt between monster rounds: the Borg reads and
 * parses the line; the fixture answers the command with Escape. */
static void old_level_prompt(void)
{
    struct keypress key;
    int guard = 0;
    inkey_flag = true;
    borg_keypress(ESCAPE);
    do {
        key = inkey_hack(0);
        Term_erase(0, 0, 255);
        msg_flag = false;
    } while (key.code != ESCAPE && ++guard < 4);
    if (key.code != ESCAPE) quit("old-level prompt did not drain");
    borg_flush();
    inkey_flag = false;
}

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
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

/* A rune the Borg laid earlier: the game's glyph, and the Borg's record of
 * it as borg_defend_aux_glyph() and borg_flow_glyph() write it. */
static void laid_rune(struct loc grid)
{
    square_add_glyph(cave, grid, GLYPH_WARDING);
    track_glyph.x[track_glyph.num] = grid.x;
    track_glyph.y[track_glyph.num] = grid.y;
    track_glyph.num++;
}

static bool rune_mode(void)
{
    return prefix(mode, "rune-");
}

int setup_tests(void **state)
{
    const char *profile = getenv("ISW198_PROFILE");
    setvbuf(stdout, NULL, _IONBF, 0);
    mode = getenv("ISW198_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(198);
    if (!player_make_simple("Human", "Warrior", "Iswarded198")) { printf("SETUPFAIL birth\n"); return 1; }
    /* The standard 80x24 main screen. */
    term_init(&screen, 80, 24, 256); Term_activate(&screen); angband_term[0] = &screen;

    /* The old level: a full-size open deep level (198x66). */
    cave = t_build_arena(AUTO_MAX_Y, AUTO_MAX_X);
    player->cave = cave_new(AUTO_MAX_Y, AUTO_MAX_X);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 40;
    player_exp_gain(player, player_exp[28] * player->expfact / 100);
    character_dungeon = true;
    character_generated = true;
    for (int y = 1; y < AUTO_MAX_Y - 1; ++y) for (int x = 1; x < AUTO_MAX_X - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* "-town" modes stand outside the town's 66x22 grid; "-inside" modes
     * stand at a grid that also exists in the town. */
    struct loc old = strstr(mode, "-inside") ? loc(30, 10) : loc(113, 48);
    player_place(cave, player, old);
    struct object *lantern = known_item(TV_LIGHT, "Lantern", 1);
    lantern->timeout = 2000;
    inven_wield(lantern, wield_slot(lantern));
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;

    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid;

    /* rune-*: a Sea of Runes, the 5x5 block of runes with the Borg on the
     * centre one.  hit-*: one rune, west of the Borg (it has stepped off).
     * hit-noglyph: no rune. */
    if (rune_mode()) {
        for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx)
            laid_rune(loc(old.x + dx, old.y + dy));
    } else if (!strstr(mode, "noglyph")) {
        laid_rune(loc(old.x - 1, old.y));
    }

    /* An awake skeleton human (level 12, one blow, no spells, never afraid): outside
     * the Sea of Runes in rune-*, next to the Borg in hit-*. */
    struct loc at = rune_mode() ? loc(old.x + 3, old.y) : loc(old.x + 1, old.y);
    enemy = t_add_monster(cave, at, "skeleton human");
    if (!enemy) { printf("SETUPFAIL monster\n"); return 1; }
    enemy->m_timed[MON_TMD_SLEEP] = 0;
    mflag_on(enemy->mflag, MFLAG_AWARE);
    update_view(cave, player);
    update_mon(enemy, cave, true);
    /* The panel the game's new-level display update chooses. */
    Term->offset_y = z_info->dungeon_hgt; Term->offset_x = z_info->dungeon_wid;
    verify_panel();

    borg.status.active = true;
    borg_update_entrypoint(true);
    borg_hook = inkey_hack;
    inkey_hack = observed_hook;
    event_add_handler(EVENT_MESSAGE, message_log, NULL);
    event_add_handler(EVENT_MESSAGE, display_message, NULL);
    event_add_handler(EVENT_MESSAGE_FLUSH, message_flush, NULL);
    printf("SETUP seed=198 mode=%s class=%s clevel=%d depth=%d hp=%d level=%dx%d screen=80x24\n",
        mode, player->class->name, player->lev, player->depth, player->chp,
        cave->width, cave->height);
    printf("STATE old borg=(%d,%d) on_rune=%d tracked=%d monster=%s at (%d,%d) visible=%d\n",
        borg.c.x, borg.c.y, square_iswarded(cave, player->grid) ? 1 : 0,
        track_glyph.num, enemy->race->name, enemy->grid.x, enemy->grid.y,
        monster_is_visible(enemy) ? 1 : 0);
    return 0;
}

int teardown_tests(void *state)
{
    event_remove_handler(EVENT_MESSAGE, message_log, NULL);
    event_remove_handler(EVENT_MESSAGE, display_message, NULL);
    event_remove_handler(EVENT_MESSAGE_FLUSH, message_flush, NULL);
    inkey_hack = NULL;
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    printf("MODE %s\n", mode);

    /* 1. Monster turns on the old level until the monster breaks an outer
     *    rune (rune-*) or bites the Borg (hit-*).  Each earlier round ends
     *    with an old-level prompt where the Borg reads the messages. */
    int rounds = 0;
    for (;;) {
        ++rounds;
        enemy->energy = z_info->move_energy;
        /* The noise map the game recomputes after each player turn. */
        make_noise(player, NULL, NULL);
        process_monsters(0);
        reset_monsters();
        if (rune_mode() ? broken_messages > 0 : hit_messages > 0) break;
        if (rounds >= 400) { printf("SETUPFAIL no monster message\n"); return 1; }
        old_level_prompt();
    }
    printf("STATE monster_rounds=%d broken=%d bites=%d hp=%d/%d tracked=%d\n",
        rounds, broken_messages, hit_messages, player->chp, player->mhp,
        track_glyph.num);

    /* 2. The world tick of the same game turn: Word of Recall activates. */
    player->word_recall = 1;
    process_world(cave);
    printf("STATE recall generate_level=%d depth=%d\n",
        player->upkeep->generate_level ? 1 : 0, player->depth);
    require(player->upkeep->generate_level);

    /* 3. Leaving the level: the game flushes the message line with -more-
     *    (the Borg stores the line and answers with a space), then makes
     *    the town. */
    event_signal(EVENT_MESSAGE_FLUSH);
    prepare_next_level(player);
    player->upkeep->generate_level = false;
    /* The panel choice of the game's new-level display update
     * (new_level_display_update() in ui-display.c). */
    Term->offset_y = z_info->dungeon_hgt; Term->offset_x = z_info->dungeon_wid;
    verify_panel();
    printf("STATE new level=%dx%d depth=%d player=(%d,%d) borg=(%d,%d) borg_in_bounds=%d tracked=%d\n",
        cave->width, cave->height, player->depth, player->grid.x,
        player->grid.y, borg.c.x, borg.c.y,
        square_in_bounds(cave, borg.c) ? 1 : 0, track_glyph.num);

    /* 4. The first command prompt on the new level, with a clear top line:
     *    the Borg parses the line it stored, then thinks. */
    Term_erase(0, 0, 255);
    inkey_flag = true;
    struct keypress key = inkey_hack(0);
    printf("DECISION first_key=%d\n", (int)key.code);
    printf("STATE after borg=(%d,%d) borg_in_bounds=%d tracked=%d depth=%d\n",
        borg.c.x, borg.c.y, square_in_bounds(cave, borg.c) ? 1 : 0,
        track_glyph.num, borg.trait[BI_CDEPTH]);
    printf("OUTCOME survived=1\n");
    ok;
}

const char *suite_name = "native-c/iswarded-198";
struct test tests[] = {
    {"first_key_after_level_change", scenario},
    {NULL, NULL}};
