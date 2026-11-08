/* Synthetic native C Sea of Runes reverse-flow fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#345 (glyphflow-345). */
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
#include "player-util.h"
#include "trap.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-glyph.h"
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

static term screen;
#define CX 99
#define CY 33

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

/* A rune is laid on a grid: the game feature, and the Borg's own map entry
 * as its map update records it after the read. */
static void lay_rune(int x, int y)
{
    square_add_glyph(cave, loc(x, y), GLYPH_WARDING);
    borg_grids[y][x].glyph = true;
}

int setup_tests(void **state)
{
    const char *profile = getenv("GF345_PROFILE");
    const char *mode    = getenv("GF345_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(345);
    if (!player_make_simple("Human", "Warrior", "GlyphFlow345")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* A full-size level: the Sea of Runes scan only looks at grids at
     * least 15 rows and 50 columns from the edges.  Open floor. */
    cave = t_build_arena(AUTO_MAX_Y, AUTO_MAX_X);
    player->cave = cave_new(AUTO_MAX_Y, AUTO_MAX_X);
    player_place(cave, player, loc(CX, CY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 99;
    player_exp_gain(player, player_exp[48] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < AUTO_MAX_Y - 1; ++y) for (int x = 1; x < AUTO_MAX_X - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *lantern = known_item(TV_LIGHT, "Lantern", 1);
    lantern->timeout = 2000;
    inven_wield(lantern, wield_slot(lantern));
    known_item(TV_SCROLL, "Rune of Protection", 40);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    for (int y = 0; y < AUTO_MAX_Y; ++y) for (int x = 0; x < AUTO_MAX_X; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 1;
    borg.status.active = true;
    printf("SETUP seed=345 mode=%s class=%s clevel=%d depth=%d runes=40\n",
           mode, player->class->name, player->lev, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

/* One production call; record its answer, the first queued key and the
 * Borg's notes from the call. */
static bool glyph_call(const char *tag)
{
    uint16_t before = messages_num();
    borg_flush();
    bool rc = borg_flow_glyph();
    keycode_t key = borg_inkey(false);
    printf("DECISION call=%s rc=%d key=%d borg=(%d,%d)\n", tag, rc ? 1 : 0,
           (int)key, borg.c.x, borg.c.y);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s %s\n", tag, message_str((uint16_t)i));
    return rc;
}

static int scenario_glyph(void *state)
{
    const char *mode = getenv("GF345_MODE");

    /* 1. Choose the centre of a new Sea of Runes: the Borg stands on the
     *    open floor it chooses as the centre (it does not need to move). */
    glyph_call("choose");
    /* 2. Arrive: the centre is stored and the Borg reads a Rune of
     *    Protection; the rune is laid on its grid. */
    bool read = glyph_call("arrive");
    if (read) lay_rune(CX, CY);

    if (streq(mode, "offcenter")) {
        /* Control: the Borg stands on an unglyphed grid of the ring, one
         * step east of the centre. */
        player_place(cave, player, loc(CX + 1, CY));
        borg.c = player->grid;
    }

    /* 3. Next calls: pick an outlying rune grid around the centre and act
     *    on it (a step toward it, or reading a rune on arrival). */
    bool rc = glyph_call("outlying");
    if (!rc) rc = glyph_call("follow");
    int ring = 0;
    for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx)
        if ((dx || dy) && borg_grids[CY + dy][CX + dx].glyph) ++ring;
    printf("STATE center_rune=%d ring_runes=%d read=%d\n",
           borg_grids[CY][CX].glyph ? 1 : 0, ring, read ? 1 : 0);
    printf("OUTCOME progress=%d\n", rc ? 1 : 0);
    require(read);
    require(rc); /* fails before: no outlying grid is reachable to the scan */
    ok;
}

const char *suite_name = "native-c/glyphflow-345";
struct test tests[] = {
    {"sea_of_runes_outlying_rune", scenario_glyph},
    {NULL, NULL}};
