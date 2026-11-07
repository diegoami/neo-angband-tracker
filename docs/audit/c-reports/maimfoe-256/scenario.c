/* Synthetic native C Maim Foe return-value diagnostic, excluded from the source fix. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "cmds.h"
#include "game-event.h"
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
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static struct monster *lich, *kobold;

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

static int observe(struct monster *mon, const char *tag)
{
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!mflag_has(mon->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL %s visible\n", tag); return 1; }
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL %s observe\n", tag); return 1; }
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL %s kill_index\n", tag); return 1; }
    borg_kills[k].awake = 1;
    if (borg_kills[k].power <= 0) { printf("SETUPFAIL %s power\n", tag); return 1; }
    printf("SETUP %s kill=%d power=%d\n", tag, k, borg_kills[k].power);
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("MF256_PROFILE");
    const char *mode = getenv("MF256_MODE");
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
    if (!player_make_simple("Human", "Blackguard", "MaimFoeReturn")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    known_item(TV_SHADOW_BOOK, "[Fear and Torment]", 1);
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[29] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Maim Foe")) spell_learn(i);
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg_cheat_spells();
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    if (!mode || streq(mode, "wrong-return")) {
        lich = t_add_monster(cave, loc(12, 11), "master lich");   /* adjacent east, kill 1 */
        if (!lich) { printf("SETUPFAIL lich\n"); return 1; }
        if (observe(lich, "lich")) return 1;
        kobold = t_add_monster(cave, loc(10, 11), "kobold");      /* adjacent west, kill 2 */
        if (!kobold) { printf("SETUPFAIL kobold\n"); return 1; }
        if (observe(kobold, "kobold")) return 1;
        /* Fixture intervention: the target list borg_attack would have
         * filled — giant first, kobold last. */
        borg_temp_x[0] = 12; borg_temp_y[0] = 11;
        borg_temp_x[1] = 10; borg_temp_y[1] = 11;
        borg_temp_n = 2;
    } else if (streq(mode, "single")) {
        lich = t_add_monster(cave, loc(12, 11), "master lich");
        if (!lich) { printf("SETUPFAIL lich\n"); return 1; }
        if (observe(lich, "lich")) return 1;
        borg_temp_x[0] = 12; borg_temp_y[0] = 11;
        borg_temp_n = 1;
    }
    printf("SETUP blows=%d power_maim=%d\n",
        borg.trait[BI_CLEVEL] / 15, borg_get_spell_power(MAIM_FOE));
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d sp=%d/%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        player->csp, player->msp);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("MF256_MODE");
    if (!mode) mode = "wrong-return";
    printf("MODE %s\n", mode);

    /* The production chooser's real-path call: simulation off, exactly as
     * borg_attack() re-invokes the winning attack. */
    borg_simulate = false;
    int v = borg_calculate_attack_effectiveness(BF_SPELL_MAIM_FOE);
    borg_simulate = true;
    printf("REALCALL value=%d\n", v);
    printf("OUTCOME value=%d\n", v);
    if (streq(mode, "wrong-return")) {
        /* The misreturn case: two adjacent monsters, the best-scoring one
         * examined first. The real call must report the best score; the
         * original returns the last monster's. */
        require(v >= 100);
    } else if (streq(mode, "single")) {
        /* Control: one monster; the last and the best are the same.
         * Complete logs are byte-identical. */
        require(v > 0);
    } else {
        /* Control: no monsters; both variants return nothing. Complete
         * logs are byte-identical. */
        require(v == 0);
    }
    ok;
}
const char *suite_name = "native-c/maimfoe-256";
struct test tests[] = {{"maim_foe_returns_best_score", scenario}, {NULL, NULL}};
