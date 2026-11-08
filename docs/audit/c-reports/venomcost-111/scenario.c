/* Synthetic native C perma-spell (Venom) fixture, excluded from the source
 * change.  Issue: diegoami/neo-angband-tracker#111 (venomcost-111). */
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
#include "borg/borg-fight-perm.h"
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

int setup_tests(void **state)
{
    const char *profile = getenv("VC111_PROFILE");
    const char *mode    = getenv("VC111_MODE");
    const char *race    = "Bullroarer the Hobbit";
    int lev = 42;
    if (!profile || !mode) return 1;
    if (streq(mode, "unique") || streq(mode, "poisoned")) { /* defaults */ }
    else if (streq(mode, "unique50")) lev = 50;
    else if (streq(mode, "nounique")) race = "cave orc";
    else { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(111);
    if (!player_make_simple("Human", "Blackguard", "VenomCost111")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 20;
    player_exp_gain(player, player_exp[lev - 2] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* Only the second Blackguard book, so Venom is the only perma spell the
     * Borg can consider. */
    known_item(TV_SHADOW_BOOK, "[Fear and Torment]", 1);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Venom")) spell_learn(i);
    player->chp = player->mhp; player->csp = player->msp;
    if (streq(mode, "poisoned")) player->timed[TMD_ATT_POIS] = 50;
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
    struct monster *mon = t_add_monster(cave, loc(PX + 3, PY), race);
    if (!mon) { printf("SETUPFAIL mon\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    /* The same scan borg_think_dungeon() runs before the perma spells. */
    borg_near_monster_type(12);
    printf("SETUP seed=111 mode=%s class=%s clevel=%d depth=%d sp=%d/%d monster=%s\n", mode,
           player->class->name, player->lev, player->depth, borg.trait[BI_CURSP],
           borg.trait[BI_MAXSP], race);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_perma(void *state)
{
    keycode_t keys[16];
    int len = 0;
    keycode_t key;
    printf("STATE near_unique=%d mon_unique=%d venom_active=%d venom_fail=%d venom_cost=%d smite_cost=%d cursp=%d\n",
           borg.near.unique, borg.mon.unique ? 1 : 0, borg.temp.venom ? 1 : 0,
           borg_spell_fail_rate(VENOM), borg_get_spell_power(VENOM),
           borg_get_spell_power(SMITE_EVIL), borg.trait[BI_CURSP]);
    borg_flush();
    bool rc = borg_perma_spell();
    while ((key = borg_inkey(true)) != 0 && len < 16) keys[len++] = key;
    printf("DECISION perma=%d keys=%d sequence=", rc ? 1 : 0, len);
    for (int i = 0; i < len; ++i) printf("%c", (char)keys[i]);
    putchar('\n');
    printf("OUTCOME cast=%d keys=%d no_rest_prep=%d\n",
           (rc && len == 3 && keys[0] == 'm') ? 1 : 0, len, borg.no_rest_prep);
    ok;
}

const char *suite_name = "native-c/venomcost-111";
struct test tests[] = {
    {"perma_venom", scenario_perma},
    {NULL, NULL}};
