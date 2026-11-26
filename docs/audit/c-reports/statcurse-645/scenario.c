/* Synthetic native C stat-curse fixture for issue #645 (statcurse-645).
 * Excluded from the source fix.  The borg detected the four stat curses
 * (dullness, sickliness, weakness, clumsiness) but applied their stat
 * penalties to the current-stat traits (BI_CSTR..), which the
 * effective-stat pass immediately overwrites with stat_cur, so the
 * penalties never reached the derived stats.  The engine adds the
 * curse's own modifiers into stat_add (calc_effects), so its stat_use
 * includes them.
 *
 * The level: an open lit arena floor, no monsters.  The character is a
 * Human Warrior with a torch; the mode equips one known item:
 *  plain      control, plain Leather Gloves (byte-identical before/after)
 *  weak       gloves carrying the weakness curse (STR -10)
 *  weak_clum  gloves carrying weakness and clumsiness (STR -10, DEX -10)
 *  sickly     Iron Helm carrying sickliness (STR/DEX/CON -5)
 *  dull       Iron Helm carrying dullness (INT/WIS -5)
 *
 * Each mode requires the borg's derived stats (BI_STR+i) to equal the
 * engine's stat_use for every stat.
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
#include "obj-curse.h"
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
static struct object *mode_item;

/* Give the mode's item its curses, then wield it and let the player
 * learn what is on it (the wear-time notice path). */
static void equip_mode_item(void)
{
    struct object *obj = NULL;
    int helm = streq(mode, "sickly") || streq(mode, "dull");
    int gloves = streq(mode, "plain") || streq(mode, "weak")
        || streq(mode, "weak_clum");

    if (gloves) {
        obj = known_item(TV_GLOVES, "Set of Leather Gloves", 1);
        if (streq(mode, "weak"))
            append_object_curse(obj, lookup_curse("weakness"), 100);
        if (streq(mode, "weak_clum")) {
            append_object_curse(obj, lookup_curse("weakness"), 100);
            append_object_curse(obj, lookup_curse("clumsiness"), 100);
        }
    } else if (helm) {
        obj = known_item(TV_HELM, "Iron Helm", 1);
        if (streq(mode, "sickly"))
            append_object_curse(obj, lookup_curse("sickliness"), 100);
        if (streq(mode, "dull"))
            append_object_curse(obj, lookup_curse("dullness"), 100);
    }
    if (!obj) { fprintf(stderr, "MISSING MODE ITEM %s\n", mode); quit("fixture mode item missing"); }
    mode_item = obj;
    inven_wield(obj, wield_slot(obj));
    /* The player learns the item's modifiers and curses by wearing it. */
    object_learn_on_wield(player, obj);
    if (streq(mode, "weak") || streq(mode, "weak_clum"))
        player_learn_curse(player, &curses[lookup_curse("weakness")]);
    if (streq(mode, "weak_clum"))
        player_learn_curse(player, &curses[lookup_curse("clumsiness")]);
    if (streq(mode, "sickly"))
        player_learn_curse(player, &curses[lookup_curse("sickliness")]);
    if (streq(mode, "dull"))
        player_learn_curse(player, &curses[lookup_curse("dullness")]);
}

int setup_tests(void **state)
{
    const char *profile = getenv("SC645_PROFILE");
    mode = getenv("SC645_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(645);
    if (!player_make_simple("Human", "Warrior", "StatCurse645")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* An open floor arena; nothing here influences stats. */
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
    equip_mode_item();
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    player->chp = player->mhp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg.c = player->grid;
    borg.status.active = true;
    borg.ready_morgoth = 0;
    borg.time.level = borg.time.town = borg.time.now;
    borg_alloc_detection();
    borg.avoidance = player->chp;
    /* The borg arrives on the level: the first production update takes
     * the new-level reset, which later state depends on. */
    borg_update();
    if (borg.status.old_depth != borg.trait[BI_CDEPTH]) { printf("SETUPFAIL arrival old_depth=%d depth=%d\n", borg.status.old_depth, borg.trait[BI_CDEPTH]); return 1; }
    printf("SETUP seed=645 mode=%s class=%s clevel=%d hp=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp, player->depth);
    return 0;
}

static int scenario_stats(void *state)
{
    /* One production notice pass: the borg re-derives its equipment and
     * stat model from the same worn items the engine just summed. */
    borg_equipment();
    borg_inventory();
    borg_notice(true);

    int stats_ok = 1;
    static const char *names[STAT_MAX] = {"STR", "INT", "WIS", "DEX", "CON", "CHR"};
    for (int i = 0; i < STAT_MAX; ++i) {
        int borg_stat = borg.trait[BI_STR + i];
        int game_stat = player->state.stat_use[i];
        int match     = (borg_stat == game_stat) ? 1 : 0;
        if (!match)
            stats_ok = 0;
        printf("STATINFO stat=%s borg=%d game=%d match=%d\n", names[i],
               borg_stat, game_stat, match);
    }
    printf("CURSEINFO weak=%d clum=%d sick=%d dull=%d str_index=%d carry=%d\n",
           borg.trait[BI_CRSWEAK] ? 1 : 0, borg.trait[BI_CRSCLUM] ? 1 : 0,
           borg.trait[BI_CRSSICK] ? 1 : 0, borg.trait[BI_CRSDULL] ? 1 : 0,
           borg.trait[BI_STR_INDEX], borg.trait[BI_CARRY]);
    evidence_map("The arena: @ = the Borg, open lit floor, no monsters");
    printf("STATE stats_ok=%d carry=%d str_index=%d\n", stats_ok,
           borg.trait[BI_CARRY], borg.trait[BI_STR_INDEX]);
    printf("OUTCOME stats_ok=%d carry=%d\n", stats_ok, borg.trait[BI_CARRY]);
    /* Corrected contract: the borg's derived stats equal the game's. */
    require(stats_ok == 1);
    ok;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/statcurse-645";
struct test tests[] = {
    {"stats_match_engine", scenario_stats},
    {NULL, NULL}};
