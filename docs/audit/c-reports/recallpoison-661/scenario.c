/* Synthetic native C Recall-decision fixture for issue #661 (recallpoison-661).
 * Excluded from the source fix.  A poisoned Borg can keep resting for a
 * pending Word of Recall even when the poison already in its system can
 * kill it before Recall activates: the borg turns the poison timer into
 * a boolean (BI_ISPOISONED), the rest-for-Recall branch checks the
 * location but never compares the poison timer against the hit points,
 * and the engine processes poison damage before the Recall countdown in
 * each world tick.  While fleeing, the heal cascade (borg_heal) is
 * skipped, so the rest branch wins every turn and the poison kills the
 * borg during the wait - the recorded D0017 death died at 7 HP with
 * nine Cure Critical Wounds potions and Herbal Curing available.
 *
 * The level: an open, lit arena floor, no monsters, a quiet square
 * (danger 0), a Human Warrior at a fixed low HP, a pending Recall
 * (word_recall 20), and a poison timer per mode.  The production
 * borg_caution() decision is observed from the keys it queues.
 *
 * Modes (HP / poison timer / cure available):
 *  lethal_cure     7 / 40 / Cure Critical Wounds.  The pending poison
 *                  outdamages the HP before lift-off: the borg must
 *                  spend the turn curing, not resting.
 *  lethal_no_cure  7 / 40 / nothing.  No cure exists; the borg must
 *                  not rest into the poison.
 *  boundary        8 / 8 / Cure Critical Wounds.  The poison can kill
 *                  exactly on the last tick before lift-off: not safe
 *                  to rest.
 *  survivable      control, 20 / 8 / Cure Critical Wounds.  The poison
 *                  cannot kill before lift-off: resting is right
 *                  (byte-identical before/after).
 *  unpoisoned      control, 7 / none / Cure Critical Wounds: resting
 *                  is right (byte-identical before/after).
 */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
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
#include "ui-input.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-caution.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-think.h"
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
static int mode_hp(void)
{
    if (streq(mode, "lethal_cure") || streq(mode, "lethal_no_cure")
        || streq(mode, "unpoisoned"))
        return 7;
    if (streq(mode, "boundary")) return 8;
    if (streq(mode, "survivable")) return 20;
    return -2;
}
static int mode_poison(void)
{
    if (streq(mode, "lethal_cure") || streq(mode, "lethal_no_cure"))
        return 40;
    if (streq(mode, "boundary") || streq(mode, "survivable"))
        return 8;
    return 0; /* unpoisoned */
}

int setup_tests(void **state)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *profile = getenv("RP661_PROFILE");
    mode = getenv("RP661_MODE");
    if (!profile || !mode || mode_hp() == -2) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(661);
    if (!player_make_simple("Human", "Warrior", "RecallPoison661")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* An open, lit arena floor; no monsters: a quiet square, danger 0. */
    cave = t_build_arena(H, W);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(20, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 4000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    /* The cure the recorded case had in reach. */
    if (streq(mode, "lethal_cure") || streq(mode, "boundary")
        || streq(mode, "survivable") || streq(mode, "unpoisoned"))
        known_item(TV_POTION, "Cure Critical Wounds", 1);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_MANA | PU_TORCH | PU_INVEN | PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);

    /* The recorded state: low HP, an active poison timer, and a Recall
     * already counting down (15..35 world turns in the engine). */
    player->chp = mode_hp();
    if (mode_poison())
        player_set_timed(player, TMD_POISONED, mode_poison(), false, false);
    player->word_recall = 20;

    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg.c = player->grid;
    borg.status.active = true;
    borg.ready_morgoth = 0;
    borg.time.level = borg.time.town = borg.time.now;
    borg_do_spell = true;
    /* The recorded situation: the borg is fleeing the level and waiting
     * for the Recall it started.  While fleeing, borg_caution skips the
     * standing heal cascade and reaches the rest-for-Recall branch -
     * exactly the path the recorded death took. */
    borg.goal.fleeing = true;
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    borg.avoidance = player->chp;
    /* The borg arrives on the level: the first production update takes
     * the new-level reset, which later state depends on.  It also
     * populates borg.goal.recalling from player->word_recall. */
    borg_update();
    if (borg.status.old_depth != borg.trait[BI_CDEPTH]) { printf("SETUPFAIL arrival old_depth=%d depth=%d\n", borg.status.old_depth, borg.trait[BI_CDEPTH]); return 1; }
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY;
    }
    track_less.num = track_more.num = 0;
    borg_notice(true);
    printf("SETUP seed=661 mode=%s class=%s clevel=%d hp=%d poison=%d recall=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp,
           player->timed[TMD_POISONED], player->word_recall, player->depth);
    return 0;
}

static int drain_keys(keycode_t *keys, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) keys[len++] = key;
    return len;
}

static int scenario_recall_wait(void *state)
{
    keycode_t keys[64];
    const char *seedenv = getenv("RP661_SEED");

    if (!borg.goal.recalling) { printf("SETUPFAIL recalling=%d\n", borg.goal.recalling); return 1; }

    /* Pin the paranoia roll inside the heal cascade so the cure
     * decision is deterministic (the roll is a production behaviour:
     * q < 75 allows cheap cures). */
    Rand_state_init(seedenv ? (unsigned int)strtol(seedenv, NULL, 10) : 6611);

    /* The production decision under examination. */
    bool acted = borg_caution();
    int len = drain_keys(keys, 64);
    printf("DECISION caution=%d keys=%d sequence=", acted ? 1 : 0, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == KC_ENTER) printf("<Enter>");
        else printf("%c", (char)keys[i]);
    }
    putchar('\n');

    int rested  = (len == 5 && keys[0] == 'R');
    int cured   = (len >= 2 && keys[0] == 'q');
    int poison  = player->timed[TMD_POISONED];
    int word    = player->word_recall;
    /* The pending poison damage before lift-off, as the engine ticks
     * it: one point per world turn while the timer lives. */
    int decr    = adj_con_fix[player->state.stat_ind[STAT_CON]] + 1;
    int turns   = (poison + decr - 1) / decr;
    int pending = (turns < word) ? turns : word;
    printf("STATE hp=%d poison=%d recall=%d pending=%d rested=%d cured=%d\n",
           borg.trait[BI_CURHP], poison, word, pending, rested, cured);
    printf("OUTCOME rested=%d cured=%d\n", rested, cured);

    if (streq(mode, "survivable") || streq(mode, "unpoisoned")) {
        /* Controls: the poison cannot kill before lift-off (or there is
         * none) - resting for Recall is the right decision. */
        require(rested == 1);
    } else {
        /* The pending poison reaches the current HP before lift-off:
         * resting is death; the borg must cure when it can and must
         * never rest here. */
        require(pending >= borg.trait[BI_CURHP]);
        require(rested == 0);
        if (streq(mode, "lethal_cure") || streq(mode, "boundary"))
            require(cured == 1);
    }

    ok;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/recallpoison-661";
struct test tests[] = {
    {"recall_wait_respects_pending_poison", scenario_recall_wait},
    {NULL, NULL}};
