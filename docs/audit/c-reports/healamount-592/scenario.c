/* Synthetic native C cure-potion heal-amount fixture, excluded from the
 * source fix. Issue: diegoami/neo-angband-tracker#592 (healamount-592). */
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
    const char *profile = getenv("HA592_PROFILE");
    const char *mode    = getenv("HA592_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(592);
    if (!player_make_simple("Human", "Warrior", "HealAmount592")) { printf("SETUPFAIL birth\n"); return 1; }
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
    known_item(TV_POTION, "Cure Serious Wounds", 3);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    /* A light cut: the random skip test in borg_heal() is bypassed for a cut
     * Borg, so the heal-amount test decides alone. */
    player->timed[TMD_CUT] = 5;
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
    /* One awake black ogre adjacent (south-east). */
    struct monster *foe = t_add_monster(cave, loc(11, 13), "black ogre");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    mflag_on(foe->mflag, MFLAG_AWARE); update_mon(foe, cave, true);
    if (!mflag_has(foe->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(foe->grid.y, foe->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[k].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    /* Fixture intervention: hit points. The danger D at the Borg's grid is
     * read first; current HP is then set so that D lies 32 above it (inside
     * [HP+25, HP+40): above the original estimate of a Cure Serious Wounds,
     * below its real 40) or, in the control, 15 above it (below both). Max HP
     * is set so 45% of it is lost (the Cure Serious Wounds band). */
    set_hp(player->mhp, player->mhp);
    int d = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    int gap = streq(mode, "control") ? 15 : 32;
    int cur = d - gap;
    if (cur < 10) { printf("SETUPFAIL danger too low d=%d\n", d); return 1; }
    set_hp(cur, cur * 100 / 55);
    printf("SETUP seed=592 mode=%s class=%s clevel=%d hp=%d/%d danger=%d cut=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp, d,
           player->timed[TMD_CUT]);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_heal(void *state)
{
    const char *mode = getenv("HA592_MODE");
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    int pct_down = (borg.trait[BI_MAXHP] - borg.trait[BI_CURHP]) * 100 / borg.trait[BI_MAXHP];
    printf("STATE danger=%d curhp=%d maxhp=%d pct_down=%d cut=%d\n", danger,
           borg.trait[BI_CURHP], borg.trait[BI_MAXHP], pct_down, borg.trait[BI_ISCUT] ? 1 : 0);
    bool rc = borg_caution();
    keycode_t key = borg_inkey(false);
    printf("DECISION rc=%d key=%d\n", rc ? 1 : 0, (int)key);
    uint16_t after = messages_num();
    int csw = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Healing Level 2")) csw = 1;
    }
    printf("OUTCOME quaffs_csw=%d\n", csw);
    /* The danger is below current HP plus the 40 a Cure Serious Wounds heals,
     * so the Borg should drink one (in the control too). */
    require(csw);
    ok;
}

const char *suite_name = "native-c/healamount-592";
struct test tests[] = {
    {"cure_serious_heal_is_valued_at_40", scenario_heal},
    {NULL, NULL}};
