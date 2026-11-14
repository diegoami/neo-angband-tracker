/* Synthetic native C resist-electricity note fixture, excluded from the
 * source fix. Issue: diegoami/neo-angband-tracker#174 (resielec-174).
 *
 * The electricity defence routine notes "# Attempting to cast RAcid"
 * (borg-fight-defend.c, the MONBLOW_ELEC sibling of the acid routine).
 * The scenario stands a Warrior with an elemental ring next to an awake
 * monster whose melee blows are electric (or acid for the unchanged
 * control), lets borg_caution() -> borg_defend() choose the resist
 * defence through the production code, and records the note text and the
 * queued keys. */
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
#include "borg/borg-fight-defend.h"
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

/* Set the Borg's current hit points (avoidance follows). */
static void set_hp(int cur, int max)
{
    player->chp = cur;
    borg_notice(true);
    borg.avoidance = player->chp;
}

int setup_tests(void **state)
{
    const char *profile = getenv("R174_PROFILE");
    const char *mode    = getenv("R174_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(174);
    if (!player_make_simple("Human", "Warrior", "Resielec174")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 18;
    player_exp_gain(player, player_exp[31] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    /* Carried defence: the lightning ring. It passes the routine's
     * capability check (borg_equips_ring) without any spell. */
    struct object *ring = known_item(TV_RING, "Lightning", 1);
    inven_wield(ring, wield_slot(ring));
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    /* values-based RES_ELEC / RES_ACID derive into the Borg's traits only
     * once the player knows the runes. */
    player_learn_all_runes(player);
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
    /* The threat, adjacent and awake: an electric melee monster for the
     * defect case, a plain melee monster for the unchanged control. */
    struct monster *foe = NULL;
    if (streq(mode, "control"))
        foe = t_add_monster(cave, loc(11, 13), "black ogre");
    else
        foe = t_add_monster(cave, loc(11, 13), "energy hound");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    mflag_on(foe->mflag, MFLAG_AWARE); update_mon(foe, cave, true);
    if (!mflag_has(foe->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(foe->grid.y, foe->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[k].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    /* Fixture intervention: hit points. Avoidance follows the current
     * HP; at full HP the routine's improvement gates are reachable with
     * an ordinary elemental melee threat. */
    set_hp(player->mhp, player->mhp);
    int d = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    int p1 = borg_danger(borg.c.y, borg.c.x, 1, false, false);
    bool save = borg.temp.res_elec;
    borg.temp.res_elec = true;
    int p2 = borg_danger(borg.c.y, borg.c.x, 1, false, false);
    borg.temp.res_elec = save;
    printf("SETUP seed=174 mode=%s class=%s clevel=%d hp=%d/%d danger=%d "
           "p1=%d p2=%d avoid=%d relec=%d racid=%d kills=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp,
           d, p1, p2, borg.avoidance,
           borg.trait[BI_RELEC] ? 1 : 0, borg.trait[BI_RACID] ? 1 : 0,
           borg_kills_cnt);
    for (int i = 1; i < borg_kills_nxt; ++i) {
        if (!borg_kills[i].r_idx) continue;
        printf("SETUP kill=%d race=%s power=%d awake=%d at=(%d,%d)\n", i,
            borg_race_name(borg_kills[i].r_idx), borg_kills[i].power,
            borg_kills[i].awake ? 1 : 0, borg_kills[i].pos.y,
            borg_kills[i].pos.x);
    }
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_resist(void *state)
{
    const char *mode = getenv("R174_MODE");
    keycode_t keys[64];
    int len = 0;
    /* The resist defences are reached through borg_caution()'s production
     * "defense before running away" step; no heal is carried, so the heal
     * step returns false first. */
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("STATE danger=%d curhp=%d avoidance=%d relec=%d racid=%d\n",
           danger, borg.trait[BI_CURHP], borg.avoidance,
           borg.trait[BI_RELEC] ? 1 : 0, borg.trait[BI_RACID] ? 1 : 0);
    uint16_t before = messages_num();
    bool rc = borg_caution();
    keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < 64) keys[len++] = key;
    char keybuf[65];
    for (int i = 0; i < len; ++i) keybuf[i] = (char)keys[i];
    keybuf[len] = 0;
    printf("DECISION rc=%d keys=\"%s\"\n", rc ? 1 : 0, keybuf);
    uint16_t after = messages_num();
    int cast_note = 0, defense_note = 0;
    char note[256] = "";
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (prefix(m, "# Performing defense type ")) defense_note = 1;
        if (prefix(m, "# Attempting to cast ")) {
            cast_note = 1;
            my_strcpy(note, m, 256);
        }
    }
    printf("OUTCOME cast_note=%d defense_note=%d note_elec=%d note_acid=%d\n",
           cast_note, defense_note,
           prefix(note, "# Attempting to cast RElec") ? 1 : 0,
           prefix(note, "# Attempting to cast RAcid") ? 1 : 0);
    if (streq(mode, "elec")) {
        /* The contract: the electricity routine names the electricity
         * resistance it is attempting. */
        require(rc);
        require(cast_note);
        require(defense_note);
        require(prefix(note, "# Attempting to cast RElec"));
    } else {
        /* Unchanged control: a plain melee threat gives the routine no
         * improvement, so no cast note is written at all and the whole
         * run must be byte-identical between the variants. */
        require(!cast_note);
    }
    ok;
}

const char *suite_name = "native-c/resielec-174";
struct test tests[] = {
    {"resist_electricity_note_names_the_right_element", scenario_resist},
    {NULL, NULL}};
