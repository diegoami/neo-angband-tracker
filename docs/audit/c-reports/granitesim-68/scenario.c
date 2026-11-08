/* Synthetic native C ranged-move simulation fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#68 (granitesim-68). */
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
#include "cmd-core.h"
#include "obj-tval.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-projection.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
static struct monster *foe;
#define H 30
#define W 30

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

static const char *feat_label(int feat)
{
    if (feat == FEAT_FLOOR) return "floor";
    if (feat == FEAT_PASS_RUBBLE) return "passable_rubble";
    if (feat == FEAT_GRANITE) return "granite";
    if (feat == FEAT_LAVA) return "lava";
    return "other";
}

int setup_tests(void **state)
{
    const char *profile = getenv("G68_PROFILE");
    const char *mode    = getenv("G68_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(68);
    if (!player_make_simple("Human", "Warrior", "GraniteSim68")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 10;
    player_exp_gain(player, player_exp[10] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* A dead-end pocket south of the room: the monster's grid (11,16) is
     * walled in on every side except the corridor mouth (11,15), which
     * holds the mode's terrain. */
    for (int y = 15; y <= 17; ++y) for (int x = 10; x <= 12; ++x)
        if (!(x == 11 && y == 16)) square_set_feat(cave, loc(x, y), FEAT_GRANITE);
    int mouth = FEAT_PASS_RUBBLE;
    if (streq(mode, "floor")) mouth = FEAT_FLOOR;
    if (streq(mode, "granite")) mouth = FEAT_GRANITE;
    square_set_feat(cave, loc(11, 15), mouth);
    player_place(cave, player, loc(11, 11));
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    known_item(TV_SCROLL, "Phase Door", 5);
    known_item(TV_POTION, "Cure Light Wounds", 5);
    /* Temporary telepathy, so the walled-in monster is sensed. */
    player_inc_timed(player, TMD_TELEPATHY, 500, false, false, false);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY;
    }
    update_view(cave, player);
    borg.c = player->grid; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    borg_notice(true);
    borg.avoidance = player->chp;
    foe = t_add_monster(cave, loc(11, 16), "blacklock mage");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    mflag_on(foe->mflag, MFLAG_AWARE); update_mon(foe, cave, true);
    if (!monster_is_visible(foe)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(foe->grid.y, foe->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[k].awake = 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=68 mode=%s class=%s clevel=%d hp=%d/%d depth=%d speed=%d mouth=%s\n",
           mode, player->class->name, player->lev, player->chp, player->mhp, player->depth,
           borg.trait[BI_SPEED], feat_label(mouth));
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("G68_MODE");
    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    borg_kill *kill = &borg_kills[k];
    printf("MODE %s\n", mode);
    printf("TRACK monster=%s pos=(%d,%d) speed=%d mouth=%s mouth_projectable=%d own_projectable=%d\n",
        foe->race->name, kill->pos.x, kill->pos.y, kill->speed,
        feat_label(borg_grids[15][11].feat), borg_projectable(15, 11, borg.c.y, borg.c.x) ? 1 : 0,
        borg_projectable(kill->pos.y, kill->pos.x, borg.c.y, borg.c.x) ? 1 : 0);
    int one = borg_danger_one_kill(borg.c.y, borg.c.x, 1, k, true, false);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("STATE danger=%d monster_danger=%d avoidance=%d hp=%d\n", danger, one, borg.avoidance, borg.trait[BI_CURHP]);
    uint16_t before = messages_num();
    borg_flush();
    bool rc = borg_caution();
    keycode_t keys[64]; char seq[256]; int len = 0, n = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < 64) keys[len++] = key;
    seq[0] = 0;
    for (int i = 0; i < len && n < 240; ++i)
        n += keys[i] == ESCAPE ? snprintf(seq + n, sizeof(seq) - n, "<Esc>")
                               : snprintf(seq + n, sizeof(seq) - n, "%c", (char)keys[i]);
    printf("DECISION rc=%d keys=%d sequence=%s\n", rc ? 1 : 0, len, seq);
    uint16_t after = messages_num();
    int recall_note = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Fleeing the level (recall)")) recall_note = 1;
    }
    /* Carry out a queued scroll read with the real command: the letter after
     * 'r' names the inventory item the Borg chose. */
    int read = 0;
    const char *scroll = "-";
    player->upkeep->energy_use = 0;
    for (int i = 0; i + 1 < len; ++i) {
        if (keys[i] != 'r') continue;
        for (int j = 0; j < z_info->pack_size; ++j) {
            struct object *obj = player->upkeep->inven[j];
            if (obj && gear_to_label(player, obj) == (char)keys[i + 1] && tval_is_scroll(obj)) {
                scroll = obj->kind->name;
                cmdq_push(CMD_READ_SCROLL);
                cmd_set_arg_item(cmdq_peek(), "item", obj);
                cmdq_execute(CTX_GAME);
                read = 1;
                break;
            }
        }
        break;
    }
    printf("ENGINE read=%d scroll=%s word_recall=%d energy_used=%d\n", read, scroll,
        player->word_recall, (int)player->upkeep->energy_use);
    printf("TRACK engine read=%d scroll=%s word_recall_active=%d\n", read, scroll, player->word_recall > 0 ? 1 : 0);
    printf("OUTCOME acted=%d danger=%d recall_flee=%d recall_active=%d\n", rc ? 1 : 0, danger,
        recall_note, player->word_recall > 0 ? 1 : 0);
    if (streq(mode, "granite")) {
        eq(danger, 0); eq(rc ? 1 : 0, 0);
    } else {
        /* A fast caster one step from a grid with a clear shot can move and
         * cast in the same turn: the Borg must count its ranged danger. */
        require(danger > 0); eq(recall_note, 1); require(player->word_recall > 0);
    }
    ok;
}

const char *suite_name = "native-c/granitesim-68";
struct test tests[] = {{"ranged_move_simulation", scenario}, {NULL, NULL}};
