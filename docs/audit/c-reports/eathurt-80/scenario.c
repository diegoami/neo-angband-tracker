/* Synthetic native C weak-from-hunger fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#80 (eathurt-80). */
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
    const char *profile = getenv("EH80_PROFILE");
    const char *mode    = getenv("EH80_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(80);
    if (!player_make_simple("Human", "Druid", "EatHurt80")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[9] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    /* The Druid has eaten all its food: remove the starting rations. */
    for (struct object *o = player->gear, *next; o; o = next) {
        next = o->next;
        if (tval_is_edible(o)) {
            bool none_left = false;
            struct object *gone = gear_object_for_use(player, o, o->number, false, &none_left);
            if (gone->known) object_delete(player->cave, NULL, &gone->known);
            object_delete(cave, player->cave, &gone);
        }
    }
    known_item(TV_POTION, "Healing", 2);
    /* The Druid has learned Remove Hunger and cast it before. */
    int hunger_sidx = -1;
    for (int i = 0; i < player->class->magic.total_spells; i++) {
        const struct class_spell *s = spell_by_index(player, i);
        if (s && streq(s->name, "Remove Hunger")) hunger_sidx = i;
    }
    if (hunger_sidx < 0) { printf("SETUPFAIL spell\n"); return 1; }
    player->spell_flags[hunger_sidx] |= PY_SPELL_LEARNED | PY_SPELL_WORKED;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp;
    player->csp = player->msp;
    /* Weak from hunger: below the Weak threshold, above Faint. */
    player_set_timed(player, TMD_FOOD, PY_FOOD_WEAK - 100, false, false);
    /* Main case: full hit points. Control: hurt by 5 hit points. */
    if (streq(mode, "hurt")) player->chp = player->mhp - 5;
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
    printf("SETUP seed=80 mode=%s class=%s clevel=%d hp=%d/%d sp=%d/%d food=%d depth=%d\n",
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

static int scenario_caution(void *state)
{
    const char *mode = getenv("EH80_MODE");
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    printf("STATE curhp=%d maxhp=%d cursp=%d weak=%d healing=%d hunger_spell=%d\n",
           borg.trait[BI_CURHP], borg.trait[BI_MAXHP], borg.trait[BI_CURSP],
           borg.trait[BI_ISWEAK] ? 1 : 0, borg.trait[BI_AHEAL],
           borg_spell_okay(REMOVE_HUNGER) ? 1 : 0);
    bool rc = borg_caution();
    keycode_t key = borg_inkey(false);
    printf("DECISION rc=%d key=%d\n", rc ? 1 : 0, (int)key);
    uint16_t after = messages_num();
    int quaffed = 0, cast = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Quaffing")) quaffed = 1;
        if (strstr(m, "Casting Remove Hunger")) cast = 1;
    }
    printf("OUTCOME quaffed_potion=%d cast_remove_hunger=%d\n", quaffed, cast);
    if (streq(mode, "fullhp")) {
        /* fails before: "when hurting" is true at full hit points */
        require(!quaffed && cast);
    } else {
        require(quaffed && !cast);
    }
    ok;
}

const char *suite_name = "native-c/eathurt-80";
struct test tests[] = {
    {"weak_druid_food_choice", scenario_caution},
    {NULL, NULL}};
