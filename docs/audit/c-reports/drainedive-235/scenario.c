/* Synthetic native C drained-dive fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#235 (drainedive-235). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "monster.h"
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
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-cave.h"
#include "borg/borg-prepared.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-power.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-trait.h"
#include "borg/borg-messages.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static int scenario_not_drained(void *state);

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND tval=%d name=%s\n", tval, name); quit("fixture item kind missing"); }
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

int setup_tests(void **state)
{
    const char *profile = getenv("DD235_PROFILE");
    if (!profile) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(235);
    if (!player_make_simple("Human", "Warrior", "DrainDive235")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 15;
    player->lev = 49;
    player->max_lev = 49;
    character_dungeon = true;
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH; update_stuff(player);
    player->chp = player->mhp;

    /* Supplies so the borg is prepared for depth 20 and 25 (no restock or
     * preparedness reason) with a small sellable count (< 12). */
    struct object *lantern = known_item(TV_LIGHT, "Lantern", 1);
    lantern->timeout = 2000;
    inven_wield(lantern, wield_slot(lantern));
    known_item(TV_FOOD, "Ration of Food", 4);
    known_item(TV_SCROLL, "Phase Door", 1);
    known_item(TV_SCROLL, "Teleportation", 4);
    /* Worn, not carried: aux's band for levels 10-20 requires See Invisible
     * (BI_SINV) and Free Action (BI_FRACT); a carried ring derives nothing. */
    struct object *ring = known_item(TV_RING, "See Invisible", 1);
    inven_wield(ring, wield_slot(ring));
    struct object *ring2 = known_item(TV_RING, "Free Action", 1);
    inven_wield(ring2, wield_slot(ring2));

    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    memcpy(safe_items, borg_items, QUIVER_END * sizeof(borg_item));
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x)
        square_set_feat(player->cave, loc(x, y), square(cave, loc(x, y))->feat);
    borg.c = player->grid; borg.avoidance = 0; borg.ready_morgoth = 0;
    borg.status.active = true;
    borg_update_entrypoint(true);
    /* Prepared for the current depth: no No_Deeper gate, no unique hunt. */
    borg_cfg[BORG_NO_DEEPER]   = 30;
    borg_cfg[BORG_KILLS_UNIQUES] = false;
    borg_numb_live_unique      = 0;
    /* Static preparedness path: the profile's dynamic formulas demand
     * CCW potions the fixture does not inject, which would leave the borg
     * unprepared for its own depth and muddy the drained-dive observable. */
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    printf("SETUP seed=235 class=%s depth=%d clevel=%d\n",
           player->class->name, player->depth, player->lev);
    /* DD235_MODE=not-drained: the unchanged control, the same prepared
     * Borg with full experience; the power dive must still be chosen. */
    if (getenv("DD235_MODE") && streq(getenv("DD235_MODE"), "not-drained")) {
        tests[0].name = "undrained_borg_still_power_dives";
        tests[0].func = scenario_not_drained;
    }
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_drained(void *state)
{
    /* A drained, fully prepared Borg at depth 15: borg_prepared(15) and
     * borg_prepared(20) are NULL (rings supply See Invisible + Free Action;
     * clevel 49 skips the CCW bands), so no branch has set g or
     * try_not_to_descend when the drained check runs.  Not bored: the bored
     * heading-up branches come before the dive chain and would pre-empt it.
     * The power-dive check is borg_prepared(CDEPTH + 5) = borg_prepared(20)
     * == NULL, so the "# power dive, playing too shallow." note must be
     * suppressed by the drained status alone.  The drain is real: exp below
     * max_exp with clevel != 50 makes the derivation hold ISFIXEXP true
     * (borg-trait.c). */
    player->exp    = 0;
    player->max_exp = 10000;
    borg.trait[BI_ISFIXEXP] = 1;
    {
        uint16_t total = messages_num();
        for (uint16_t i = 0; i < total; ++i) (void)message_str(i);
    }

    fprintf(stderr, "PROBE sinv=%d fract=%d fixexp=%d\n",
           borg.trait[BI_SINV] ? 1 : 0, borg.trait[BI_FRACT] ? 1 : 0,
           borg.trait[BI_ISFIXEXP] ? 1 : 0);
    fprintf(stderr, "PRE exp=%d max_exp=%d fixexp=%d\n",
           (int)player->exp, (int)player->max_exp, borg.trait[BI_ISFIXEXP] ? 1 : 0);
    borg_leave_level(false);
    fprintf(stderr, "POST fixexp=%d active=%d\n",
           borg.trait[BI_ISFIXEXP] ? 1 : 0, borg.status.active ? 1 : 0);

    uint16_t t2 = messages_num();
    for (uint16_t i = 0; i < t2 && i < 14; ++i)
        printf("NOTE %02d %s\n", (int)i, message_str(i));
    int dives = 0;
    uint16_t total = messages_num();
    for (uint16_t i = 0; i < total; ++i) {
        const char *m = message_str(i);
        if (m && strstr(m, "power dive"))
            ++dives;
    }
    printf("OUTCOME dive_notes=%d drained=%d\n", dives,
           borg.trait[BI_ISFIXEXP] ? 1 : 0);
    require(borg.trait[BI_ISFIXEXP]);
    require(dives == 0); /* fails before: the drained Borg power-dives */
    ok;
}

/* Control: the same fully prepared Borg at depth 15, not drained
 * (exp == max_exp). Both versions must take the power dive. */
static int scenario_not_drained(void *state)
{
    player->exp    = 10000;
    player->max_exp = 10000;
    borg.trait[BI_ISFIXEXP] = 0;
    {
        uint16_t total = messages_num();
        for (uint16_t i = 0; i < total; ++i) (void)message_str(i);
    }
    borg_leave_level(false);
    uint16_t t2 = messages_num();
    for (uint16_t i = 0; i < t2 && i < 14; ++i)
        printf("NOTE %02d %s\n", (int)i, message_str(i));
    int dives = 0;
    for (uint16_t i = 0; i < t2; ++i) {
        const char *m = message_str(i);
        if (m && strstr(m, "power dive"))
            ++dives;
    }
    printf("OUTCOME dive_notes=%d drained=%d\n", dives,
           borg.trait[BI_ISFIXEXP] ? 1 : 0);
    require(!borg.trait[BI_ISFIXEXP]);
    require(dives == 1);
    ok;
}

const char *suite_name = "native-c/drainedive-235";
struct test tests[] = {
    {"drained_borg_does_not_power_dive", scenario_drained},
    {NULL, NULL}};
