/* Synthetic native C stun-danger fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#589 (stunprot-589). */
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
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow-kill.h"
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

/* Wear a fully known Soft Leather Armour that gives sound resistance and/or
 * protection from stun, as the mode asks. */
static void wear_armour(bool rsound, bool pstun)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(TV_SOFT_ARMOR, lookup_sval(TV_SOFT_ARMOR, "Soft Leather Armour"));
    if (!kind) quit("fixture armour kind missing");
    object_prep(obj, kind, 35, MAXIMISE);
    if (rsound) obj->el_info[ELEM_SOUND].res_level = 1;
    if (pstun) of_on(obj->flags, OF_PROT_STUN);
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    inven_wield(obj, wield_slot(obj));
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
    const char *profile = getenv("SP589_PROFILE");
    const char *mode    = getenv("SP589_MODE");
    if (!profile || !mode) return 1;
    bool rsound  = streq(mode, "rsound") || streq(mode, "both") || streq(mode, "rsound-unstunned");
    bool pstun   = streq(mode, "pstun") || streq(mode, "both");
    bool stunned = !streq(mode, "rsound-unstunned");
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(589);
    if (!player_make_simple("Human", "Warrior", "StunProt589")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 35;
    player_exp_gain(player, player_exp[33] * player->expfact / 100);
    character_dungeon = true;
    wear_armour(rsound, pstun);
    player_learn_all_runes(player);
    for (struct object *o = player->gear; o; o = o->next) player_know_object(player, o);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    /* Fixture intervention: the Borg is already stunned (a light stun). */
    if (stunned) player->timed[TMD_STUN] = 10;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    borg_equipment(); borg_inventory(); borg_notice(true); borg_notice_player();
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW | BORG_GLOW;
    }
    update_view(cave, player);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    /* An awake gravity hound three grids east (breath range). */
    struct monster *hound = t_add_monster(cave, loc(15, 12), "gravity hound");
    if (!hound) { printf("SETUPFAIL hound\n"); return 1; }
    if (observe(hound, "hound")) return 1;
    borg.status.active = true;
    printf("SETUP seed=589 mode=%s class=%s clevel=%d hp=%d stun=%d\n", mode,
           player->class->name, player->lev, player->chp, player->timed[TMD_STUN]);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_danger(void *state)
{
    const char *mode = getenv("SP589_MODE");
    borg_notice(true); borg_notice_player();
    bool pstun_worn = streq(mode, "pstun") || streq(mode, "both");
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    /* The same danger with only the Borg's "stunned" traits cleared: the
     * difference is the extra danger it assigns to being stunned again. */
    int16_t s1 = borg.trait[BI_ISSTUN], s2 = borg.trait[BI_ISHEAVYSTUN];
    borg.trait[BI_ISSTUN] = borg.trait[BI_ISHEAVYSTUN] = 0;
    int calm = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    borg.trait[BI_ISSTUN] = s1; borg.trait[BI_ISHEAVYSTUN] = s2;
    int penalty = danger - calm;
    printf("STATE rsound=%d isstun=%d avoidance=%d\n", borg.trait[BI_RSND] ? 1 : 0,
           borg.trait[BI_ISSTUN] ? 1 : 0, borg.avoidance);
    printf("OUTCOME danger=%d calm=%d stun_penalty=%d\n", danger, calm, penalty);
    if (streq(mode, "rsound-unstunned")) {
        /* Control: not stunned, so no stun penalty in either build. */
        require(penalty == 0);
    } else if (pstun_worn) {
        /* Gravity cannot stun a player protected from stun. */
        require(penalty == 0);
    } else {
        /* Sound resistance does not stop gravity's stun. */
        require(penalty > 0);
    }
    ok;
}

const char *suite_name = "native-c/stunprot-589";
struct test tests[] = {
    {"stun_danger_follows_stun_protection", scenario_danger},
    {NULL, NULL}};
