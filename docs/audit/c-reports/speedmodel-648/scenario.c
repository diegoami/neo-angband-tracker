/* Synthetic native C speed-model fixture for issue #648 (speedmodel-648).
 * Excluded from the source fix.  The borg's timed-speed block added +10 for
 * haste or sprint and otherwise +5 for terror, and never applied the slow
 * penalty.  The engine applies +10 haste/sprint, -10 slow and +10 terror,
 * independently of each other (player-calcs.c, the timed block of
 * calc_bonuses()).
 *
 * The level: an open lit arena floor, no monsters.  The character is a
 * Human Warrior with a torch; nothing else influences speed.
 *
 * Modes set the player's timed effects through the real engine function
 * player_set_timed(), recompute the engine's bonuses (PU_BONUS), run one
 * production borg_notice() and compare the borg's BI_SPEED with the
 * engine's player->state.speed:
 *  none         control, no timed effect (byte-identical before/after)
 *  fast         control, TMD_FAST only (byte-identical before/after)
 *  slow         TMD_SLOW: the borg ignores the -10
 *  terror       TMD_TERROR: the borg adds 5 instead of 10
 *  fast_terror  TMD_FAST + TMD_TERROR: the borg drops the terror bonus
 *               entirely (else-if) and rates 5, engine adds both (+20)
 *  slow_terror  TMD_SLOW + TMD_TERROR: engine nets 0, the borg adds 5
 */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
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
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-util.h"
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

static void evidence_map(const char *caption)
{
    printf("MAP %s\n", caption);
    for (int y = 0; y < cave->height; ++y) {
        char row[256];
        int x;
        for (x = 0; x < cave->width && x < 255; ++x) {
            struct loc grid = loc(x, y);
            wchar_t c = f_info[square(cave, grid)->feat].d_char;
            if (loc_eq(grid, player->grid))
                c = L'@';
            row[x] = (c > 31 && c < 127) ? (char)c : '?';
        }
        row[x] = 0;
        printf("MAPROW %s\n", row);
    }
    printf("MAPEND\n");
}

static term screen;
#define H 24
#define W 40

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

static const char *mode;

/* Set one timed effect through the real engine API (no message). */
static void set_timed(int idx, int v)
{
    if (v > 0)
        player_set_timed(player, idx, v, false, false);
}

int setup_tests(void **state)
{
    const char *profile = getenv("SM648_PROFILE");
    mode = getenv("SM648_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(648);
    if (!player_make_simple("Human", "Warrior", "SpeedModel648")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* An open floor arena; nothing here influences speed. */
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(20, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 4000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    player->chp = player->mhp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg.c = player->grid;
    borg.status.active = true;
    borg.ready_morgoth = 0;
    borg.time.level = borg.time.town = borg.time.now;
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    borg.avoidance = player->chp;
    /* The borg arrives on the level: the first production update takes
     * the new-level reset, which later state depends on. */
    borg_update();
    if (borg.status.old_depth != borg.trait[BI_CDEPTH]) { printf("SETUPFAIL arrival old_depth=%d depth=%d\n", borg.status.old_depth, borg.trait[BI_CDEPTH]); return 1; }
    printf("SETUP seed=648 mode=%s class=%s clevel=%d hp=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp, player->depth);
    return 0;
}

static int scenario_speed(void *state)
{
    int fast  = streq(mode, "fast") || streq(mode, "fast_terror");
    int slow  = streq(mode, "slow") || streq(mode, "slow_terror");
    int terror = streq(mode, "terror") || streq(mode, "fast_terror")
        || streq(mode, "slow_terror");

    /* Set the timed effects through the real engine API, then recompute
     * the engine's view of the character (PU_BONUS runs calc_bonuses,
     * whose timed block sets player->state.speed). */
    set_timed(TMD_FAST, fast ? 30 : 0);
    set_timed(TMD_SPRINT, 0);
    set_timed(TMD_SLOW, slow ? 30 : 0);
    set_timed(TMD_TERROR, terror ? 30 : 0);
    player->upkeep->update |= PU_BONUS;
    update_stuff(player);

    /* One production borg notice pass: the borg re-derives every trait,
     * including its speed model, from the same player state. */
    borg_notice(true);

    int borg_speed   = borg.trait[BI_SPEED];
    int engine_speed = player->state.speed;
    printf("SPEEDINFO mode=%s fast=%d slow=%d terror=%d borg_speed=%d engine_speed=%d match=%d\n",
           mode, fast, slow, terror, borg_speed, engine_speed,
           borg_speed == engine_speed);
    /* The energy a borg turn would collect at the modelled speed, the
     * number the danger and approach math actually uses. */
    printf("ENERGYINFO borg_energy=%d engine_energy=%d\n",
           extract_energy[borg_speed], extract_energy[engine_speed]);
    evidence_map("The arena: @ = the Borg, open lit floor, no monsters");
    printf("STATE borg_speed=%d engine_speed=%d\n", borg_speed, engine_speed);
    int speed_ok = (borg_speed == engine_speed) ? 1 : 0;
    printf("OUTCOME speed_ok=%d borg=%d engine=%d\n", speed_ok, borg_speed, engine_speed);
    /* Corrected contract: the borg's modelled speed equals the game's. */
    require(speed_ok == 1);
    ok;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/speedmodel-648";
struct test tests[] = {
    {"speed_model_matches_engine", scenario_speed},
    {NULL, NULL}};
