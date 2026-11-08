/* Synthetic native C vault excavation flag diagnostic, excluded from the source fix. */
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
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-magic.h"
#include "borg/borg-think.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;

#define WX 13
#define WY 12

int setup_tests(void **state)
{
    const char *profile = getenv("EV236_PROFILE");
    const char *mode = getenv("EV236_MODE");
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
    if (!player_make_simple("Human", "Druid", "VaultExcavation")) { printf("SETUPFAIL birth\n"); return 1; }
    player_learn_all_runes(player);
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* The vault dig site: an excavatable quartz grid beside a permanent
     * wall (that pairing is what the excavation scan tracks). */
    square_set_feat(cave, loc(WX, WY), FEAT_QUARTZ);
    if (!mode || streq(mode, "excavate")) {
        square_set_feat(cave, loc(WX + 1, WY + 1), FEAT_PERM);
    }
    /* Carry the Gifts of Nature book. */

    {
        struct object *book = object_new();
        struct object_kind *kind = lookup_kind(TV_NATURE_BOOK, lookup_sval(TV_NATURE_BOOK, "[Gifts of Nature]"));
        if (!kind) quit("fixture book missing");
        object_prep(book, kind, 50, MAXIMISE);
        book->number = 1;
        book->known = object_new();
        object_set_base_known(player, book);
        object_flavor_aware(player, book);
        inven_carry(player, book, false, false);
        object_touch(player, book);
        player_know_object(player, book);
    }
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[11] * player->expfact / 100);
    while (player_stat_inc(player, STAT_WIS)) {}
    player_learn_all_runes(player);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Turn Stone to Mud")) spell_learn(i);
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg_cheat_spells();
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW | BORG_MARK;
    }
    borg.status.vault = true;
    printf("SETUP vault=%d spelllegal=%d wallfeat=%d perma_adjacent=%d\n",
        borg.status.vault ? 1 : 0,
        borg_spell_legal(TURN_STONE_TO_MUD) ? 1 : 0,
        borg_grids[WY][WX].feat,
        borg_grids[WY + 1][WX + 1].feat == FEAT_PERM ? 1 : 0);
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=1000 class=%s level=%d depth=%d mana=%d/%d\n",
        player->class->name, player->lev, player->depth, player->csp, player->msp);
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

static void print_keys(const keycode_t *keys, int len)
{
    for (int i = 0; i < len; ++i) {
        if (i) putchar(' ');
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');
}

static int scenario(void *state)
{
    const char *mode = getenv("EV236_MODE");
    keycode_t keys[64];
    int len;
    if (!mode) mode = "excavate";
    printf("MODE %s\n", mode);

    /* The production vault excavation routine. */
    bool acted = borg_excavate_vault(9);
    len = drain_keys(keys, 64);
    printf("DECISION acted=%d keys=%d sequence=", acted, len);
    print_keys(keys, len);
    int glow = borg_grids[WY][WX].info & BORG_GLOW ? 1 : 0;
    int dark = borg_grids[WY][WX].info & BORG_DARK ? 1 : 0;
    printf("STATE glow=%d dark=%d\n", glow, dark);
    printf("OUTCOME acted=%d dark_marked=%d\n", acted ? 1 : 0, dark);
    if (streq(mode, "excavate")) {
        /* The misflag case: the dug grid is remembered as glowing where
         * the comment says Dark. The correction sets BORG_DARK and clears
         * the glow. */
        eq(acted ? 1 : 0, 1);
        require(dark == 1);
        require(glow == 0);
    } else {
        /* Control: no perma-adjacent dig site; the routine finds nothing
         * and no flags are touched. Complete logs are byte-identical. */
        eq(acted ? 1 : 0, 0);
        require(len == 0);
        require(dark == 0);
        require(glow == 0);
    }
    ok;
}
const char *suite_name = "native-c/excavate-236";
struct test tests[] = {{"excavated_grid_marked_dark", scenario}, {NULL, NULL}};
