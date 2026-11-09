/* Synthetic native C Necromancer lit-square spell failure diagnostic, excluded from the source fix. */
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
#include "borg/borg-flow.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Modes (N296_MODE):
 *   lit   - torchless Human Necromancer on a permanently lit (glowing) square
 *   dark  - the same Necromancer, the same arena, but its own square is not glowing
 *   mage  - a Human Mage (starting torch wielded) on the same glowing arena, no unlight penalty
 * In every mode one awake Kobold stands five squares south on a glowing square.
 */

static term screen;
static struct monster *kobold;
static int spell_idx = -1;
static enum borg_spells borg_spell_id;
static const char *spell_name;

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

static const char *mode_name(void)
{
    const char *mode = getenv("N296_MODE");
    return mode ? mode : "lit";
}

int setup_tests(void **state)
{
    const char *profile = getenv("N296_PROFILE");
    const char *mode = mode_name();
    bool mage = streq(mode, "mage");
    bool dark = streq(mode, "dark");
    if (!profile) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(296);
    if (!player_make_simple("Human", mage ? "Mage" : "Necromancer", "LitSquare")) {
        printf("SETUPFAIL birth\n"); return 1;
    }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    /* Permanent light everywhere; in the dark mode the Borg's own square is not lit. */
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x) {
        if (dark && y == 11 && x == 11) continue;
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    }
    if (mage) {
        known_item(TV_MAGIC_BOOK, "[First Spells]", 1);
        spell_name = "Magic Missile"; borg_spell_id = MAGIC_MISSILE;
    } else {
        known_item(TV_SHADOW_BOOK, "[Fear and Torment]", 1);
        spell_name = "Vampire Strike"; borg_spell_id = VAMPIRE_STRIKE;
    }
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[24] * player->expfact / 100);
    /* Eight intelligence gains: the original Borg estimate is then 8% (cf. the reported 9%). */
    for (int n = 0; n < 8; ++n) player_stat_inc(player, STAT_INT);
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, spell_name)) { spell_learn(i); spell_idx = i; }
    if (spell_idx < 0) { printf("SETUPFAIL spell\n"); return 1; }
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
    kobold = t_add_monster(cave, loc(11, 16), "Kobold");
    if (!kobold) { printf("SETUPFAIL kobold\n"); return 1; }
    mflag_on(kobold->mflag, MFLAG_AWARE); update_mon(kobold, cave, true);
    if (!mflag_has(kobold->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL kobold visible\n"); return 1; }
    if (!observe_kill_diff(kobold->grid.y, kobold->grid.x, 0, 0)) { printf("SETUPFAIL kobold observe\n"); return 1; }
    int k = borg_grids[kobold->grid.y][kobold->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kobold kill_index\n"); return 1; }
    borg_kills[k].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=296 class=%s level=%d depth=%d hp=%d/%d sp=%d/%d light=%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        player->csp, player->msp, player->state.cur_light);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int drain_keys(keycode_t *keys, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) keys[len++] = key;
    return len;
}

static int scenario(void *state)
{
    const char *mode = mode_name();
    keycode_t keys[64];
    int len;
    printf("MODE %s\n", mode);
    int game = spell_chance(spell_idx);
    int estimate = borg_spell_fail_rate(borg_spell_id);
    printf("TRACK square_islit=%d square_light=%d glow=%d borg_light_flag=%d borg_glow_flag=%d\n",
        square_islit(cave, player->grid) ? 1 : 0, square_light(cave, player->grid),
        square_isglow(cave, player->grid) ? 1 : 0,
        (borg_grids[borg.c.y][borg.c.x].info & BORG_LIGHT) ? 1 : 0,
        (borg_grids[borg.c.y][borg.c.x].info & BORG_GLOW) ? 1 : 0);
    printf("TRACK spell=\"%s\" game_spell_chance=%d borg_estimate=%d threshold=25 borg_okay_fail=%d\n",
        spell_name, game, estimate, borg_spell_okay_fail(borg_spell_id, 25) ? 1 : 0);

    /* The production attack chooser, acting mode. */
    bool acted = borg_attack(false);
    len = drain_keys(keys, 64);
    printf("DECISION acted=%d keys=%d sequence=", acted, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');
    bool cast = false;
    for (int i = 0; i < len; ++i) if (keys[i] == 'm') cast = true;
    printf("OUTCOME borg_estimate=%d game_chance=%d estimate_matches_game=%d cast_spell=%d game_chance_over_threshold=%d\n",
        estimate, game, estimate == game, cast ? 1 : 0, game > 25);
    /* Corrected contract: the Borg's estimate equals the game's chance, and
     * it casts the spell only when the game's chance is within its limit. */
    eq(estimate, game);
    eq(cast ? 1 : 0, game > 25 ? 0 : 1);
    ok;
}
const char *suite_name = "native-c/necrolight-296";
struct test tests[] = {{"necromancer_lit_square_fail_estimate", scenario}, {NULL, NULL}};
