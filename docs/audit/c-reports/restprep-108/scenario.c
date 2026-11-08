/* Synthetic native C weak-from-hunger fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#80 (restprep-108). */
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
#define H 24
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
    const char *profile = getenv("RP108_PROFILE");
    const char *mode    = getenv("RP108_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(108);
    if (!player_make_simple("Human", "Priest", "RestPrep108")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[streq(mode, "clevel30") ? 28 : 33] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    known_item(TV_PRAYER_BOOK, "[Cleansing Power]", 1);
    /* The Priest has learned Protection from Evil and cast it before. */
    int pfe_sidx = -1;
    for (int i = 0; i < player->class->magic.total_spells; i++) {
        const struct class_spell *s = spell_by_index(player, i);
        if (s && streq(s->name, "Protection from Evil")) pfe_sidx = i;
    }
    if (pfe_sidx < 0) { printf("SETUPFAIL spell\n"); return 1; }
    player->spell_flags[pfe_sidx] |= PY_SPELL_LEARNED | PY_SPELL_WORKED;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp;
    player->csp = player->msp;
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
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=108 mode=%s class=%s clevel=%d hp=%d/%d sp=%d/%d food=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp,
           player->csp, player->msp, player->timed[TMD_FOOD], player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_rest(void *state)
{
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    printf("STATE clevel=%d cursp=%d maxsp=%d pfe_castable=%d depth=%d\n", borg.trait[BI_CLEVEL],
           borg.trait[BI_CURSP], borg.trait[BI_MAXSP],
           borg_spell_okay(PROTECTION_FROM_EVIL) ? 1 : 0, borg.trait[BI_CDEPTH]);
    bool prepared = borg_prep_leave_level_spells();
    keycode_t key = borg_inkey(false);
    printf("DECISION prepared=%d key=%d no_rest_prep=%d\n", prepared ? 1 : 0, (int)key,
           (int)borg.no_rest_prep);
    bool may_rest = borg_check_rest(borg.c.y, borg.c.x);
    printf("DECISION may_rest=%d\n", may_rest ? 1 : 0);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    printf("OUTCOME rests_after_prep=%d\n", may_rest ? 1 : 0);
    require(prepared);
    require(!may_rest); /* fails before at character level 33 and above: the timer wrapped */
    ok;
}

const char *suite_name = "native-c/restprep-108";
struct test tests[] = {
    {"no_rest_after_prep_spell", scenario_rest},
    {NULL, NULL}};
