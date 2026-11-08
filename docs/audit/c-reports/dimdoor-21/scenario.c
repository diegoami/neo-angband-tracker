/* Synthetic native C Dimension Door target fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#21 (dimdoor-21).
 *
 * A level 40 Mage stands in a lit room with an invisible dread.  The rest
 * of the level is granite except for a second room further east.  The
 * player has mapped the whole level (the game's enlightenment routine), so
 * the Borg's own map holds both rooms and the granite.  The dread casts a
 * nether bolt through the game's monster spell code; an EVENT_MESSAGE
 * handler records each line the game prints and hands it to the Borg's
 * message parser (borg_parse).  The Borg's map/message update then raises
 * its regional fear around the unseen attacker, and the Borg's production
 * escape routine (borg_escape, as borg_caution calls it) runs with the
 * danger of the Borg's grid.  The keys the Borg queues are decoded (the
 * free-targeting cursor walk, then the spell selection) and the spell is
 * cast through the game's spell code at that target.  The fixture records
 * the grid the Borg aimed at and the grid the game actually put the player
 * on. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-event.h"
#include "game-input.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
#include "mon-spell.h"
#include "mon-util.h"
#include "monster.h"
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
#include "player-util.h"
#include "target.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-escape.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-messages.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include "ui-menu.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 66
#define W 198
#define PX 66
#define PY 30

static char captured[32][256];
static int  n_captured;
static const char *mode;
static struct monster *dread;

/* Room A holds the player (and the dread); room B lies further east. */
#define AX0 60
#define AX1 72
#define BX0 100
#define BX1 110
#define RY0 25
#define RY1 35

static void capture(game_event_type type, game_event_data *data, void *user)
{
    if (n_captured < 32 && data->message.msg)
        my_strcpy(captured[n_captured++], data->message.msg, 256);
}

/* The Borg answers the "Direction?" prompt with '5': use the current
 * target.  This is what the text UI does for that key when a target is
 * set. */
static bool aim_at_target(int *dir)
{
    if (!target_okay()) return false;
    *dir = DIR_TARGET;
    return true;
}

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

static void carve(int x0, int x1, int y0, int y1)
{
    for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) {
        square_set_feat(cave, loc(x, y), FEAT_FLOOR);
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_ROOM);
    }
}

int setup_tests(void **state)
{
    const char *profile = getenv("DD21_PROFILE");
    mode = getenv("DD21_MODE");
    if (!profile || !mode) return 1;
    if (!streq(mode, "granite") && !streq(mode, "open")) { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(21);
    if (!player_make_simple("Human", "Mage", "DimDoor21")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    /* granite: solid rock with two rooms.  open: the whole level is one
     * open lit floor (control). */
    if (streq(mode, "granite")) {
        for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
            square_set_feat(cave, loc(x, y), FEAT_GRANITE);
        carve(AX0, AX1, RY0, RY1);
        carve(BX0, BX1, RY0, RY1);
    } else {
        carve(1, W - 2, 1, H - 2);
    }
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 40;
    player_exp_gain(player, player_exp[48] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    character_dungeon = true;
    /* Only the book with Dimension Door, and only that spell learned. */
    known_item(TV_MAGIC_BOOK, "[Arcane Control]", 1);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Dimension Door")) spell_learn(i);
    /* Hurt by earlier fighting: a quarter of the maximum hit points. */
    player->chp = player->mhp / 4; player->csp = player->msp;
    /* The level has been mapped (the game's own enlightenment routine,
     * which memorises walls next to floor), and the player has explored
     * room B (granite) or the whole level (open): those floor grids are
     * memorised as the game does for grids the player has seen. */
    wiz_light(cave, player, true);
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x) {
        struct loc g = loc(x, y);
        if (!square_isfloor(cave, g)) continue;
        if (streq(mode, "open") || (x >= BX0 && x <= BX1)) square_memorize(cave, g);
    }
    update_view(cave, player);
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg_cheat_spells();
    borg_alloc_detection();
    borg.c = player->grid; borg.goal.g = player->grid;
    borg.time.level = borg.time.now;
    borg_notice(true);
    borg.avoidance = borg.trait[BI_CURHP]; borg.ready_morgoth = 0;
    /* An invisible dread in the player's room, south-east of the player. */
    dread = t_add_monster(cave, loc(PX + 3, PY + 2), "dread");
    if (!dread) { printf("SETUPFAIL dread\n"); return 1; }
    mflag_on(dread->mflag, MFLAG_AWARE); update_mon(dread, cave, true);
    if (monster_is_visible(dread)) { printf("SETUPFAIL dread visible\n"); return 1; }
    borg.status.active = true;
    /* The Borg's own map update reads the mapped level. */
    borg_update();
    event_add_handler(EVENT_MESSAGE, capture, NULL);
    get_aim_dir_hook = aim_at_target;
    printf("SETUP seed=21 mode=%s class=%s clevel=%d depth=%d hp=%d/%d sp=%d/%d "
           "borg=(%d,%d) dread=(%d,%d) dd_fail=%d map_A=%d map_B=%d map_corner=%d\n",
           mode, player->class->name, player->lev, player->depth,
           borg.trait[BI_CURHP], borg.trait[BI_MAXHP], borg.trait[BI_CURSP],
           borg.trait[BI_MAXSP], borg.c.x, borg.c.y, dread->grid.x, dread->grid.y,
           borg_spell_fail_rate(DIMENSION_DOOR), borg_grids[RY0][AX0].feat,
           borg_grids[RY0][BX0].feat, borg_grids[1][16].feat);
    return 0;
}

int teardown_tests(void *state)
{
    get_aim_dir_hook = NULL;
    event_remove_handler(EVENT_MESSAGE, capture, NULL);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static const char *feat_label(int feat)
{
    return feat == FEAT_NONE ? "unknown" : f_info[feat].name;
}

static int scenario(void *state)
{
    keycode_t keys[256];
    int len = 0;
    keycode_t key;

    /* The unseen dread casts a nether bolt through the game's monster spell
     * code (as make_ranged_attack does once the monster has chosen it). */
    do_mon_spell(RSF_BO_NETH, dread, monster_is_visible(dread));
    update_stuff(player);
    int before = borg_msg_num;
    for (int i = 0; i < n_captured; ++i) {
        printf("TRACK game-message <%s>\n", captured[i]);
        borg_parse(captured[i]);
    }
    borg_parse(NULL);
    for (int i = before; i < borg_msg_num; ++i)
        printf("TRACK borg-reaction <%s>\n", borg_msg_buf + borg_msg_pos[i]);
    n_captured = 0;
    for (int pass = 0; pass < 3; ++pass) borg_update();
    borg_notice(true);
    borg.avoidance = borg.trait[BI_CURHP];

    int fear_here = borg_fear_region[borg.c.y / 11][borg.c.x / 11];
    int pos_danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("STATE hp=%d/%d sp=%d avoidance=%d fear_region_here=%d pos_danger=%d dd_fail=%d\n",
           borg.trait[BI_CURHP], borg.trait[BI_MAXHP], borg.trait[BI_CURSP],
           borg.avoidance, fear_here, pos_danger, borg_spell_fail_rate(DIMENSION_DOOR));

    /* The production escape routine, called as borg_caution calls it. */
    borg_flush();
    bool rc = borg_escape(pos_danger);
    while ((key = borg_inkey(true)) != 0 && len < 256) keys[len++] = key;
    printf("DECISION escape=%d keys=%d sequence=", rc ? 1 : 0, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');

    /* Decode the keys: '*' 'p' (target mode at the player), cursor moves,
     * '5' (select), then 'm' book spell (cast) and '5' (use target). */
    int i = 0;
    struct loc aim = player->grid;
    bool targeted = false, cast = false;
    int book = -1, slot = -1;
    if (len >= 2 && keys[0] == '*' && keys[1] == 'p') {
        for (i = 2; i < len && keys[i] != '5'; ++i) {
            int d = keys[i] - '0';
            if (d < 1 || d > 9) break;
            aim = loc_sum(aim, loc(ddx[d], ddy[d]));
        }
        if (i < len && keys[i] == '5') { targeted = true; ++i; }
    }
    if (targeted && i + 3 < len + 1 && keys[i] == 'm') {
        book = keys[i + 1];
        slot = keys[i + 2];
        cast = (i + 3 < len && keys[i + 3] == '5');
    }
    int dd = -1;
    for (int s = 0; s < player->class->magic.total_spells; ++s)
        if (borg_magics[s].spell_enum == DIMENSION_DOOR) dd = s;
    bool is_dd = cast && dd >= 0
        && slot == all_letters_nohjkl[borg_magics[dd].book_offset]
        && book == all_letters_nohjkl[borg.book_idx[borg_magics[dd].book]];
    printf("DECISION targeted=%d aim=(%d,%d) aim_feat=%s borg_aim_feat=%s aim_danger=%d cast_dimension_door=%d\n",
           targeted ? 1 : 0, aim.x, aim.y, f_info[square(cave, aim)->feat].name,
           feat_label(borg_grids[aim.y][aim.x].feat),
           borg_danger(aim.y, aim.x, 2, true, false), is_dd ? 1 : 0);

    struct loc from = player->grid;
    struct loc land = from;
    if (is_dd) {
        int idx = -1;
        for (int s = 0; s < player->class->magic.total_spells; ++s)
            if (streq(spell_by_index(player, s)->name, "Dimension Door")) idx = s;
        target_set_location(aim.y, aim.x);
        spell_cast(idx, DIR_TARGET, NULL);
        update_stuff(player);
        land = player->grid;
        for (int m = 0; m < n_captured; ++m)
            printf("TRACK game-message <%s>\n", captured[m]);
    }
    int land_fear = borg_fear_region[land.y / 11][land.x / 11];
    bool in_room_a = land.x >= AX0 && land.x <= AX1 && land.y >= RY0 && land.y <= RY1;
    printf("TRACK moved from (%d,%d) to (%d,%d), aimed at (%d,%d)\n", from.x, from.y,
           land.x, land.y, aim.x, aim.y);
    printf("OUTCOME aim_x=%d aim_y=%d landed_x=%d landed_y=%d on_aim=%d aim_distance=%d dread_distance=%d "
           "dread_los=%d same_room=%d fear_region_at_landing=%d\n",
           aim.x, aim.y, land.x, land.y, loc_eq(land, aim) ? 1 : 0,
           distance(land, aim), distance(land, dread->grid),
           los(cave, land, dread->grid) ? 1 : 0, in_room_a ? 1 : 0, land_fear);

    require(is_dd);
    /* Corrected contract: Dimension Door aims at a grid the player can
     * stand on, so the game puts the player where the Borg aimed. */
    require(loc_eq(land, aim));
    ok;
}

const char *suite_name = "native-c/dimdoor-21";
struct test tests[] = {
    {"dimension_door_target", scenario},
    {NULL, NULL}};
