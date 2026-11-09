/* Synthetic native C panel-geometry fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#348 (panelrows-348).
 *
 * The Borg's grid map (borg_grids) has 66 rows of 198 grids, the size of
 * the largest dungeon level.  Its "scan the visible panel" loops run over
 * the panel the game is showing, w_y .. w_y + SCREEN_HGT and
 * w_x .. w_x + SCREEN_WID, and SCREEN_HGT / SCREEN_WID grow with the
 * terminal.  When the terminal shows more rows or columns than the level
 * has, the game clamps the panel origin to 0 and the panel runs past the
 * level's last row or column.
 *
 * Real code: the game's panel placement (center_panel / modify_panel in
 * ui-output.c), the Borg's own copy of the panel origin (borg-update.c
 * reads Term->offset_y/x into w_y/w_x; the fixture performs that copy),
 * and the production scans borg_flow_light() and borg_flow_vault().
 * Fixture interventions: the terminal size, the arena, the Borg's map
 * entries (features copied from the level; BORG_GLOW on a lit room as the
 * Borg's call-light code records it), the vault flag, a carried mattock. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-util.h"
#include "ui-output.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-light.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-misc.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-trait.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static const char *mode;
static int term_w, term_h;
static bool vault_mode;
static bool dig_mode;
static struct loc start;

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 0, MAXIMISE);
    obj->number = number;
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

/* Terminal size and scan per mode. */
static bool pick_mode(void)
{
    static const struct { const char *name; int w, h; bool vault; int x, y; bool dig; } modes[] = {
        /* light: Borg in the dark at (20,50); lit room x 5..15, y 55..62 */
        {"light-tall", 80, 70, false, 20, 50, false},
        {"light-wide", 230, 24, false, 20, 50, false},
        {"light-normal", 80, 24, false, 20, 50, false},
        {"light-fit", 212, 68, false, 20, 50, false},
        /* vault: Borg at (180,60); magma at (181,62) beside a permanent wall */
        {"vault-tall", 80, 70, true, 180, 60, false},
        {"vault-wide", 230, 24, true, 180, 60, false},
        {"vault-normal", 80, 24, true, 180, 60, false},
        /* dig: a Druid with Turn Stone to Mud, same vault layout */
        {"dig-tall", 80, 70, true, 180, 60, true},
        {"dig-wide", 230, 24, true, 180, 60, true},
        {"dig-normal", 80, 24, true, 180, 60, true},
    };
    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        if (streq(mode, modes[i].name)) {
            term_w = modes[i].w; term_h = modes[i].h; vault_mode = modes[i].vault;
            dig_mode = modes[i].dig;
            start = loc(modes[i].x, modes[i].y);
            return true;
        }
    }
    return false;
}

int setup_tests(void **state)
{
    const char *profile = getenv("PR348_PROFILE");
    mode = getenv("PR348_MODE");
    if (!profile || !mode || !pick_mode()) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(348);
    if (!player_make_simple("Human", dig_mode ? "Druid" : "Warrior", "PanelRows348")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, term_w, term_h, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* A full-size level (198x66), open floor inside permanent walls. */
    cave = t_build_arena(AUTO_MAX_Y, AUTO_MAX_X);
    player->cave = cave_new(AUTO_MAX_Y, AUTO_MAX_X);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 10;
    player_exp_gain(player, player_exp[19] * player->expfact / 100);
    character_dungeon = true;
    if (vault_mode) {
        square_set_feat(cave, loc(182, 62), FEAT_PERM);
        square_set_feat(cave, loc(181, 62), FEAT_MAGMA);
    } else {
        for (int y = 55; y <= 62; ++y) for (int x = 5; x <= 15; ++x)
            sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    }
    player_place(cave, player, start);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    if (dig_mode) {
        /* The Druid's two town books, every spell up to its level learned
         * and cast once before. */
        known_item(TV_NATURE_BOOK, "[Lesser Charms]", 1);
        known_item(TV_NATURE_BOOK, "[Gifts of Nature]", 1);
        for (int i = 0; i < player->class->magic.total_spells; ++i)
            if (spell_by_index(player, i)->slevel <= player->lev)
                player->spell_flags[i] |= PY_SPELL_LEARNED | PY_SPELL_WORKED;
    } else if (vault_mode) {
        struct object *mattock = known_item(TV_DIGGING, "Mattock", 1);
        inven_wield(mattock, wield_slot(mattock));
    }
    player->upkeep->update |= PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp; player->csp = player->msp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    for (int y = 0; y < AUTO_MAX_Y; ++y) for (int x = 0; x < AUTO_MAX_X; ++x) {
        borg_grid *ag = &borg_grids[y][x];
        ag->feat = square(cave, loc(x, y))->feat;
        ag->info |= BORG_MARK;
        if (square_isglow(cave, loc(x, y))) ag->info |= BORG_GLOW;
    }
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.vault = vault_mode;
    borg.status.active = true;

    /* The game places the panel around the player (real code), and the
     * Borg copies its origin at the start of borg_update(). */
    center_panel();
    w_y = Term->offset_y;
    w_x = Term->offset_x;
    printf("SETUP seed=348 mode=%s class=%s clevel=%d depth=%d level=%dx%d\n",
           mode, player->class->name, player->lev, player->depth,
           cave->width, cave->height);
    printf("GEOMETRY term=%dx%d panel_wid=%d panel_hgt=%d w_x=%d w_y=%d last_col=%d last_row=%d map_cols=%d map_rows=%d\n",
           term_w, term_h, SCREEN_WID, SCREEN_HGT, w_x, w_y,
           w_x + SCREEN_WID - 1, w_y + SCREEN_HGT - 1, AUTO_MAX_X, AUTO_MAX_Y);
    if (dig_mode)
        printf("SPELL stone_to_mud_okay=%d sp=%d/%d\n", borg_spell_okay_fail(TURN_STONE_TO_MUD, 30) ? 1 : 0,
               borg.trait[BI_CURSP], borg.trait[BI_MAXSP]);
    else if (vault_mode)
        printf("DIG can_dig_quartz=%d dig=%d\n", borg_can_dig(false, FEAT_QUARTZ) ? 1 : 0,
               borg.trait[BI_DIG]);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    uint16_t before = messages_num();
    int found = 0, offmap = 0;
    bool rc;

    borg_flush();
    if (dig_mode) {
        rc = borg_excavate_vault(z_info->max_range - 2);
        found = borg_temp_n;
        for (int i = 0; i < borg_temp_n; ++i)
            if (borg_temp_y[i] >= AUTO_MAX_Y || borg_temp_x[i] >= AUTO_MAX_X) ++offmap;
    } else if (vault_mode) {
        rc = borg_flow_vault(35);
        found = borg_temp_n;
        for (int i = 0; i < borg_temp_n; ++i)
            if (borg_temp_y[i] >= AUTO_MAX_Y || borg_temp_x[i] >= AUTO_MAX_X) ++offmap;
    } else {
        rc = borg_flow_light(GOAL_FLEE);
        found = borg_glow_n;
        for (int i = 0; i < borg_glow_n; ++i)
            if (borg_glow_y[i] >= AUTO_MAX_Y || borg_glow_x[i] >= AUTO_MAX_X) ++offmap;
    }
    keycode_t key = borg_inkey(false);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    printf("DECISION call=%s rc=%d key=%d borg=(%d,%d)\n",
           dig_mode ? "borg_excavate_vault" : vault_mode ? "borg_flow_vault" : "borg_flow_light", rc ? 1 : 0,
           (int)key, borg.c.x, borg.c.y);
    printf("OUTCOME completed=1 targets=%d offmap=%d progress=%d\n", found, offmap, rc ? 1 : 0);
    require(offmap == 0);
    require(rc);
    ok;
}

const char *suite_name = "native-c/panelrows-348";
struct test tests[] = {
    {"panel_scan_stays_on_map", scenario},
    {NULL, NULL}};
