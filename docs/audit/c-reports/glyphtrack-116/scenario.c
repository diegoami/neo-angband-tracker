/* Synthetic native C Sea of Runes arrival fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#116 (glyphtrack-116). */
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
#include "borg/borg-flow-take.h"
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
    const char *profile = getenv("GT116_PROFILE");
    const char *mode    = getenv("GT116_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(116);
    if (!player_make_simple("Human", "Warrior", "GlyphTrack116")) { printf("SETUPFAIL birth\n"); return 1; }
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
    printf("SETUP seed=116 mode=%s class=%s clevel=%d depth=%d runes=40\n",
           mode, player->class->name, player->lev, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

/* One production call; record its answer, the queued keys and the Borg's
 * notes from the call. */
static bool glyph_call(const char *tag)
{
    uint16_t before = messages_num();
    borg_flush();
    bool rc = borg_flow_glyph();
    int count = 0;
    keycode_t first = 0, key;
    while ((key = borg_inkey(true)) != 0 && count < 32) {
        if (!count) first = key;
        ++count;
    }
    printf("DECISION call=%s rc=%d first_key=%d keys_queued=%d borg=(%d,%d)\n", tag,
           rc ? 1 : 0, (int)first, count, borg.c.x, borg.c.y);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s %s\n", tag, message_str((uint16_t)i));
    return rc;
}

static int scenario_glyph(void *state)
{
    const char *mode = getenv("GT116_MODE");
    int ring = 0;

    /* 1. Choose the centre of a new Sea of Runes: the Borg stands on the
     *    open floor it chooses as the centre (it does not need to move). */
    glyph_call("choose");
    /* 2. Arrive: the centre is stored, the Borg reads a Rune of Protection
     *    and tracks the new rune; the rune is laid on its grid. */
    bool first = glyph_call("arrive");
    lay_rune(CX, CY);
    printf("STATE tracked_runes=%d needs_new_sea=%d\n", track_glyph.num, borg_needs_new_sea ? 1 : 0);
    if (streq(mode, "new-sea")) {
        /* Control: the first arrival, the rune is new. */
        printf("OUTCOME first_rc=%d\n", first ? 1 : 0);
        require(first);
        ok;
    }

    /* 3. The ring of 24 runes is laid by the fixture, as the Borg's own
     *    earlier reads would have left it, so the sea is complete. */
    for (int i = 0; i < 24; ++i)
        lay_rune(CX + borg_ddx_ddd[i], CY + borg_ddy_ddd[i]);
    for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx)
        if ((dx || dy) && borg_grids[CY + dy][CX + dx].glyph) ++ring;
    printf("STATE ring_runes=%d center_rune=%d\n", ring, borg_grids[CY][CX].glyph ? 1 : 0);

    /* 4. The complete sea makes its centre the goal; the Borg arrives and
     *    reads another rune onto the rune it already tracks. */
    glyph_call("goal");

    /* A potion lies three grids east, in view: an ordinary item for the
     * Borg's next flow (placed by the fixture in the Borg's object list). */
    struct object_kind *potion = lookup_kind(TV_POTION, lookup_sval(TV_POTION, "Cure Light Wounds"));
    borg_takes[1].kidx  = potion->kidx;
    borg_takes[1].known = true;
    borg_takes[1].x     = CX + 3;
    borg_takes[1].y     = CY;
    borg_takes[1].value = 100;
    borg_takes[1].tval  = TV_POTION;
    borg_takes_nxt      = 2;
    borg_takes_cnt      = 1;

    /* The caller's sequence (borg-think-dungeon.c): the glyph flow, and if
     * it reports no action, its remaining flows in the same turn.  Here
     * the flow towards a nearby object is the next one. */
    uint16_t before = messages_num();
    borg_flush();
    bool again    = borg_flow_glyph();
    bool followup = false;
    if (!again) followup = borg_flow_take(false, 5);
    int count = 0;
    keycode_t key, keys[32];
    while ((key = borg_inkey(true)) != 0 && count < 32) keys[count++] = key;
    printf("DECISION call=arrive-again rc=%d followup_step=%d keys_queued=%d\n",
           again ? 1 : 0, followup ? 1 : 0, count);
    printf("TRACK keys");
    for (int i = 0; i < count; ++i) printf(" %d", (int)keys[i]);
    printf("\n");
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK arrive-again %s\n", message_str((uint16_t)i));
    printf("OUTCOME first_rc=%d second_rc=%d tracked_runes=%d extra_step=%d\n",
           first ? 1 : 0, again ? 1 : 0, track_glyph.num, followup ? 1 : 0);
    require(first);
    require(again); /* fails before: the read is queued but the call says false */
    ok;
}

const char *suite_name = "native-c/glyphtrack-116";
struct test tests[] = {
    {"sea_of_runes_arrival_on_known_rune", scenario_glyph},
    {NULL, NULL}};
