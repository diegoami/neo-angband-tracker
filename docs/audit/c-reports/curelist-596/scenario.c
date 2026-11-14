/* Synthetic native C cure-choice fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#596 (curelist-596). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "object.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-prefs.h"
#include "ui-term.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-recover.h"
#include "borg/borg-item.h"
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

/* Set the game's and the Borg's hit points. */
static void set_hp(int cur, int max)
{
    player->mhp = max;
    player->chp = cur;
    borg_notice(true);
    borg.avoidance = player->chp;
}

int setup_tests(void **state)
{
    const char *profile = getenv("CL596_PROFILE");
    const char *mode    = getenv("CL596_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(596);
    if (!player_make_simple("Human", "Warrior", "CureList596")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 10;
    player_exp_gain(player, player_exp[8] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    /* Carried: Elvish Waybread and Cure Light Wounds potions ("blind"), or
     * Cure Light Wounds only ("control"). */
    if (streq(mode, "blind")) known_item(TV_FOOD, "Elvish Waybread", 3);
    known_item(TV_POTION, "Cure Light Wounds", 3);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true); borg_notice_player();
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    borg.status.active = true;
    borg_update_entrypoint(true);
    /* Fixture intervention: the Borg is blind (no monster around). */
    player->timed[TMD_BLIND] = 50;
    borg_notice(true); borg_notice_player();
    int d = 0;
    printf("SETUP seed=596 mode=%s class=%s clevel=%d hp=%d/%d blind=%d danger=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp,
           borg.trait[BI_ISBLIND] ? 1 : 0, d);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

/* z-rand.c keeps its WELL index here; Rand_state_init() does not reset it. */
extern uint32_t state_i;

static int scenario_cure(void *state)
{
    const char *mode = getenv("CL596_MODE");
    int waybread = 0, clw = 0, other = 0, none = 0;
    /* 100 independent production decisions, seeds 1..100. */
    for (int t = 1; t <= 100; ++t) {
        borg_flush();
        memset(&borg.goal, 0, sizeof(borg.goal));
        borg_notice(true); borg_notice_player();
        state_i = 0; Rand_state_init((uint32_t)t);
        uint16_t before = messages_num();
        bool rc = borg_recover();
        uint16_t after = messages_num();
        int w = 0, c = 0;
        for (int i = (int)(after - before) - 1; i >= 0; --i) {
            const char *m = message_str((uint16_t)i);
            if (strstr(m, "Waybread")) w = 1;
            if (strstr(m, "Cure Light Wounds")) c = 1;
        }
        if (w) waybread++; else if (c) clw++; else if (rc) other++; else none++;
        printf("TRIAL seed=%d rc=%d waybread=%d clw=%d\n", t, rc ? 1 : 0, w, c);
    }
    printf("OUTCOME eats_waybread=%d quaffs_clw=%d other=%d nothing=%d\n", waybread, clw, other, none);
    /* Waybread does not cure blindness: a blind Borg must not choose it. */
    require(waybread == 0);
    require(clw > 0);
    ok;
}

const char *suite_name = "native-c/curelist-596";
struct test tests[] = {
    {"blind_borg_uses_a_cure_that_works", scenario_cure},
    {NULL, NULL}};
