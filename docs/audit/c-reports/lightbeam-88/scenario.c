/* Synthetic native C corridor fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#88 (lightbeam-88). */
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
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-think.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-misc.h"
#include "borg/borg-light.h"
#include "borg/borg-item-val.h"
#include "borg/borg-item-use.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 60
#define W 40

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
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
    const char *profile = getenv("LB88_PROFILE");
    const char *mode    = getenv("LB88_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(88);
    if (!player_make_simple("Human", "Warrior", "LightBeam88")) { printf("SETUPFAIL birth\n"); return 1; }
    /* A standard 80x24 screen: a 22-row map panel. */
    term_init(&screen, 80, 24, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    /* Fixture intervention: where the map panel sits is set here (the UI does it in play). */
    int row = 30, offset = 24;
    if (streq(mode, "top")) { row = 10; offset = 0; }
    Term->offset_y = offset; Term->offset_x = 0;
    player_place(cave, player, loc(20, row));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    struct object *wand = known_item(TV_WAND, "Light", 1);
    wand->pval = 5;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory();
    borg_do_spell = true;
    borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    /* The corridor ahead is unexplored: the grid just beyond the light, to the north, is unknown. */
    if (streq(mode, "scrolled_south"))
        borg_grids[row + borg.trait[BI_LIGHT] + 1][20].feat = FEAT_NONE;
    else
        borg_grids[row - borg.trait[BI_LIGHT] - 1][20].feat = FEAT_NONE;
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=88 mode=%s class=%s hp=%d/%d depth=%d\n", mode, player->class->name,
           player->chp, player->mhp, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_beam(void *state)
{
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    printf("STATE row=%d panel_offset=%d row_on_panel=%d panel_height=%d light=%d wand_charges=%d\n",
           borg.c.y, (int)Term->offset_y, borg.c.y - (int)Term->offset_y, borg_panel_hgt(),
           borg.trait[BI_LIGHT], borg_items[borg_slot(TV_WAND, sv_wand_light)].pval);
    bool cast = borg_light_beam(false);
    int keys = 0;
    char seq[64] = "";
    keycode_t k;
    while ((k = borg_inkey(true)) != 0) {
        size_t n = strlen(seq);
        if (n < sizeof(seq) - 3) snprintf(seq + n, 3, "%s%c", keys ? " " : "", (char)k);
        keys++;
    }
    printf("DECISION beam_cast=%d keys=\"%s\"\n", cast ? 1 : 0, seq);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    printf("OUTCOME beam_cast=%d\n", cast ? 1 : 0);
    require(cast); /* fails before in the scrolled modes: the absolute row 30 exceeds the panel height */
    ok;
}

const char *suite_name = "native-c/lightbeam-88";
struct test tests[] = {
    {"light_beam_north", scenario_beam},
    {NULL, NULL}};
