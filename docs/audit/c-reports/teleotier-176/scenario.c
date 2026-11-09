/* Synthetic native C Teleport Other defence fixture, excluded from the source
 * change.  Issue: diegoami/neo-angband-tracker#176 (teleotier-176). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40
#define PX 10
#define PY 12
#define LEV 16
#define RACE "hill giant"

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
    object_prep(obj, kind, 10, MAXIMISE);
    obj->number = number;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

/* p1 passed to borg_defend(), as a multiple of borg.avoidance (x4). */
static int p1_quarters = 0;

int setup_tests(void **state)
{
    const char *profile = getenv("TO176_PROFILE");
    const char *mode    = getenv("TO176_MODE");
    if (!profile || !mode) return 1;
    if (streq(mode, "over3")) p1_quarters = 14;        /* 3.5 x avoidance */
    else if (streq(mode, "upper")) p1_quarters = 11;   /* 2.75 x: above 5/2 */
    else if (streq(mode, "lower")) p1_quarters = 9;    /* 2.25 x */
    else if (streq(mode, "under2")) p1_quarters = 6;   /* 1.5 x */
    else { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(176);
    if (!player_make_simple("Human", "Mage", "TeleOTier176")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 15;
    player_exp_gain(player, player_exp[LEV - 2] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* Only the book with Teleport Other, and only that spell learned. */
    known_item(TV_MAGIC_BOOK, "[Magical Defences]", 1);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Teleport Other")) spell_learn(i);
    player->chp = player->mhp; player->csp = player->msp;
    /* Afraid: +20 to the spell failure rate, which puts Teleport Other
     * (41%) between the fail-rate tiers (40, 45, 50). */
    player->timed[TMD_AFRAID] = 100;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg_cheat_spells();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    borg.c = player->grid;
    borg.time.level = borg.time.now;
    borg_notice(true);
    borg.avoidance = borg.trait[BI_CURHP];
    struct monster *mon = t_add_monster(cave, loc(PX + 3, PY), RACE);
    if (!mon) { printf("SETUPFAIL mon\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill\n"); return 1; }
    borg_kills[k].awake = true;
    borg_near_monster_type(12);
    printf("SETUP seed=176 mode=%s class=%s clevel=%d depth=%d hp=%d sp=%d/%d monster=%s\n", mode,
           player->class->name, player->lev, player->depth, borg.trait[BI_CURHP],
           borg.trait[BI_CURSP], borg.trait[BI_MAXSP], RACE);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_defend(void *state)
{
    keycode_t keys[16];
    int len = 0;
    keycode_t key;
    int p1 = borg.avoidance * p1_quarters / 4;
    int grid = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("STATE avoidance=%d p1=%d grid_danger=%d fail=%d afraid=%d\n", borg.avoidance, p1,
           grid, borg_spell_fail_rate(TELEPORT_OTHER), borg.trait[BI_ISAFRAID] ? 1 : 0);
    borg_flush();
    bool rc = borg_defend(p1);
    while ((key = borg_inkey(true)) != 0 && len < 16) keys[len++] = key;
    printf("DECISION defend=%d keys=%d sequence=", rc ? 1 : 0, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');
    int cast = 0;
    for (int i = 0; i + 2 < len; ++i) if (keys[i] == 'm') cast = 1;
    printf("OUTCOME cast=%d keys=%d\n", (rc && cast) ? 1 : 0, len);
    ok;
}

const char *suite_name = "native-c/teleotier-176";
struct test tests[] = {
    {"defend_tele_away", scenario_defend},
    {NULL, NULL}};
