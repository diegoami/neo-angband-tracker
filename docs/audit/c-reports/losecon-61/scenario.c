/* Synthetic native C constitution-drain danger fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#61 (losecon-61). */
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
#define HURT_HP 500

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
    const char *profile = getenv("LC61_PROFILE");
    const char *mode    = getenv("LC61_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(61);
    if (!player_make_simple("Human", "Warrior", "LoseCon61")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    player_exp_gain(player, player_exp[47] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    known_item(TV_SCROLL, "Teleportation", 3);
    /* Stat drain: the mode decides which stats were drained below 8. */
    bool lowcon = streq(mode, "lowcon") || streq(mode, "bothlow");
    bool lowstr = streq(mode, "lowstr") || streq(mode, "bothlow");
    if (lowcon) player->stat_cur[STAT_CON] = 7;
    if (lowstr) player->stat_cur[STAT_STR] = 7;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
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
    track_less.num = track_more.num = 0;
    borg.c = player->grid;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    player->chp = HURT_HP;
    borg_notice(true);
    borg.avoidance = player->chp;
    /* A purple mushroom patch right next to the Borg (east). */
    struct monster *foe = t_add_monster(cave, loc(11, 12), "purple mushroom patch");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    if (observe(foe, "foe")) return 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=61 mode=%s class=%s clevel=%d hp=%d mhp=%d depth=%d str_cur=%d str_max=%d con_cur=%d con_max=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp, player->depth,
           player->stat_cur[STAT_STR], player->stat_max[STAT_STR],
           player->stat_cur[STAT_CON], player->stat_max[STAT_CON]);
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
    const char *mode = getenv("LC61_MODE");
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    for (int k = 1; k < borg_kills_nxt; ++k)
        printf("KILL k=%d r=%d pos=(%d,%d) awake=%d\n", k, borg_kills[k].r_idx,
               borg_kills[k].pos.x, borg_kills[k].pos.y, borg_kills[k].awake);
    printf("DANGER cstr=%d ccon=%d danger=%d\n", borg.trait[BI_CSTR], borg.trait[BI_CCON], danger);
    bool rc = borg_caution();
    keycode_t key = borg_inkey(false);
    printf("STATE danger=%d avoidance=%d hp=%d cstr=%d ccon=%d\n", danger,
           borg.avoidance, borg.trait[BI_CURHP], borg.trait[BI_CSTR], borg.trait[BI_CCON]);
    printf("DECISION rc=%d key=%d\n", rc ? 1 : 0, (int)key);
    uint16_t after = messages_num();
    int escaped = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Danger Level")) escaped = 1;
    }
    printf("OUTCOME escape=%d\n", escaped);
    /* Intended weighting: only a constitution below 8 makes the
     * constitution-drain blows extra scary.  Danger 456 without the extra
     * fear, 756 with it, against 500 hit points. */
    if (streq(mode, "lowcon") || streq(mode, "bothlow")) {
        /* fails before for lowcon: strength was checked */
        require(danger == 756);
        require(escaped);
    } else {
        /* fails before for lowstr: strength was checked */
        require(danger == 456);
        require(!escaped);
    }
    ok;
}

const char *suite_name = "native-c/losecon-61";
struct test tests[] = {
    {"constitution_drain_danger", scenario_caution},
    {NULL, NULL}};
