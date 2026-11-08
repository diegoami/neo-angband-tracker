/* Synthetic native C recall-in-danger mana fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#157 (recallmana-157). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
#include "mon-util.h"
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
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow-stairs.h"
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
#define H 24
#define W 40

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

static int observe(struct monster *mon, const char *tag)
{
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!mflag_has(mon->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL %s visible\n", tag); return 1; }
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) { printf("SETUPFAIL %s observe\n", tag); return 1; }
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL %s kill_index\n", tag); return 1; }
    borg_kills[k].awake = 1;
    return 0;
}

int setup_tests(void **state)
{
    const char *profile = getenv("RM157_PROFILE");
    const char *mode    = getenv("RM157_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(157);
    if (!player_make_simple("Human", "Mage", "RecallMana157")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    player_exp_gain(player, player_exp[33] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* An up staircase two steps west. */
    {
        int feat = FEAT_LESS;
        square_set_feat(cave, loc(8, 12), feat);
    }
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    known_item(TV_POTION, "Restore Mana", 2);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* The Borg has seen both staircases. */
    {
        struct borg_track *t = &track_less;
        track_less.num = track_more.num = 0;
        t->x[t->num] = 8; t->y[t->num] = 12; t->num++;
    }
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    /* Badly hurt: 9 hit points, so the danger is above twice the hit points. */
    player->chp = 9;
    /* Main case: mana below a quarter of the maximum. Control: half. */
    player->csp = streq(mode, "halfmana") ? player->msp / 2 : player->msp / 10;
    borg_notice(true);
    borg.avoidance = player->chp;
    /* A dangerous monster right next to the Borg (south-east), awake. */
    struct monster *foe = t_add_monster(cave, loc(11, 13), "Cave bear");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    if (observe(foe, "foe")) return 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=157 mode=%s class=%s clevel=%d hp=%d depth=%d up=%d down=%d\n",
           mode, player->class->name, player->lev, player->chp, player->depth, track_less.num, track_more.num);
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
    const char *mode = getenv("RM157_MODE");
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    /* Word of Recall has been read and is about to activate. */
    player->word_recall = 15;
    borg.goal.recalling = 150;
    borg_notice(true);
    printf("STATE sp=%d maxsp=%d recalling=%d\n", borg.trait[BI_CURSP], borg.trait[BI_MAXSP], borg.goal.recalling);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    for (int k = 1; k < borg_kills_nxt; ++k)
        printf("KILL k=%d r=%d pos=(%d,%d) awake=%d power=%d\n", k, borg_kills[k].r_idx,
               borg_kills[k].pos.x, borg_kills[k].pos.y, borg_kills[k].awake, borg_kills[k].power);
    bool rc = borg_caution();
    keycode_t key = borg_inkey(false);
    printf("STATE danger=%d avoidance=%d hp=%d fleeing=%d\n", danger,
           borg.avoidance, borg.trait[BI_CURHP], borg.goal.fleeing ? 1 : 0);
    printf("DECISION rc=%d key=%d\n", rc ? 1 : 0, (int)key);
    uint16_t after = messages_num();
    int restore = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Buying time waiting for Recall.(1)")) restore = 1;
    }
    printf("OUTCOME restore_mana=%d\n", restore);
    if (streq(mode, "lowmana")) {
        /* fails before: the mana test compares mana with itself */
        require(restore);
    } else {
        require(!restore);
    }
    ok;
}

const char *suite_name = "native-c/recallmana-157";
struct test tests[] = {
    {"restore_mana_while_waiting_for_recall", scenario_caution},
    {NULL, NULL}};
