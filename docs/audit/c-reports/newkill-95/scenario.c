/* Native C observer for a newly seen pass-wall monster, excluded from the source fix. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "cmds.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "option.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-timed.h"
#include "player-util.h"
#include "target.h"
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "z-virt.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item-use.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-power.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-think-dungeon.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static term screen;
static struct monster *foe;
static struct object *shots;

static void print_message(game_event_type type, game_event_data *data, void *user)
{
    if (data && data->message.msg) printf("MSG %s\n", data->message.msg);
}

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
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

static const char *feat_label(int f)
{
    if (f == FEAT_FLOOR) return "floor";
    if (f == FEAT_GRANITE) return "granite";
    if (f == FEAT_NONE) return "unknown";
    return "other";
}

int setup_tests(void **state)
{
    const char *profile = getenv("T95_PROFILE");
    const char *mode = getenv("T95_MODE");
    if (!profile) return 1;
    if (!mode) mode = "wall";
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(1000);
    if (!player_make_simple("Human", "Warrior", "NewKill")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 20;
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* The only fixture terrain change: one granite grid four steps east
     * of the player in the wall case (a lone pillar in a lit room). */
    struct loc spot = loc(15, 11);
    if (streq(mode, "wall")) square_set_feat(cave, spot, FEAT_GRANITE);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    struct object *sling = known_item(TV_BOW, "Sling", 1);
    inven_wield(sling, wield_slot(sling));
    shots = known_item(TV_SHOT, "Iron Shot", 20);
    player_place(cave, player, loc(11, 11));
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    update_view(cave, player);
    /* The Borg's map knows the real terrain of every grid, including the
     * pillar: this is what borg_update_map() records from a seen grid. */
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW | BORG_MARK;
    }
    printf("SETUP seed=1000 class=%s level=%d depth=%d hp=%d/%d mode=%s\n",
        player->class->name, player->lev, player->depth, player->chp,
        player->mhp, mode);
    printf("TERRAIN game=%s borg_map=%s at (%d,%d)\n",
        feat_label(square(cave, spot)->feat),
        feat_label(borg_grids[spot.y][spot.x].feat), spot.y, spot.x);
    /* A stock earth spirit (PASS_WALL, visible) placed one grid east and
     * moved into the spot with the game's own monster_swap(), the move a
     * pass-wall monster makes into rock. */
    foe = t_add_monster(cave, loc(16, 11), "earth spirit");
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    monster_swap(loc(16, 11), spot);
    update_view(cave, player);
    mon_clear_timed(foe, MON_TMD_SLEEP, MON_TMD_FLG_NOTIFY);
    mflag_on(foe->mflag, MFLAG_AWARE); update_mon(foe, cave, true);
    if (!mflag_has(foe->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    borg.status.active = true;
    borg_update_entrypoint(true);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int drain_keys(keycode_t *keys, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) keys[len++] = key;
    return len;
}

static int scenario(void *state)
{
    const char *mode = getenv("T95_MODE");
    keycode_t keys[64];
    char keybuf[65];
    int len;
    if (!mode) mode = "wall";
    printf("MODE %s\n", mode);
    struct loc spot = foe->grid;
    int before_feat = borg_grids[spot.y][spot.x].feat;
    /* Production first sighting: guess the race, create the record. */
    bool seen = observe_kill_diff(spot.y, spot.x, 0, 0);
    int k = borg_grids[spot.y][spot.x].kill;
    int after_feat = borg_grids[spot.y][spot.x].feat;
    printf("TRACK seen=%d kill=%d race=%s pass_wall=%d\n", seen ? 1 : 0, k,
        k > 0 ? r_info[borg_kills[k].r_idx].name : "-",
        k > 0 ? (rf_has(r_info[borg_kills[k].r_idx].flags, RF_PASS_WALL) ? 1 : 0) : -1);
    printf("TRACK borg_map before_sighting=%s after_sighting=%s game=%s\n",
        feat_label(before_feat), feat_label(after_feat),
        feat_label(square(cave, spot)->feat));
    /* The production attack chooser, acting mode. */
    bool acted = borg_attack(false);
    len = drain_keys(keys, 64);
    for (int i = 0; i < len; ++i) keybuf[i] = (char)keys[i];
    keybuf[len] = 0;
    int fired = strchr(keybuf, 'f') != NULL ? 1 : 0;
    printf("DECISION acted=%d keys=\"%s\" fired=%d\n", acted ? 1 : 0, keybuf, fired);
    int hp_before = foe->hp, ammo_before = shots->number;
    player->upkeep->energy_use = 0;
    if (fired) {
        /* Carry out the Borg's choice with the real fire command at the
         * monster's grid (the target the Borg's '*' keys select). */
        event_add_handler(EVENT_MESSAGE, print_message, NULL);
        target_set_location(spot.y, spot.x);
        cmdq_push(CMD_FIRE);
        cmd_set_arg_item(cmdq_peek(), "item", shots);
        cmd_set_arg_target(cmdq_peek(), "target", DIR_TARGET);
        cmdq_execute(CTX_GAME);
        event_remove_handler(EVENT_MESSAGE, print_message, NULL);
    }
    int ammo_after = (shots && shots->number) ? shots->number : 0;
    printf("ENGINE monster_hp=%d->%d quiver_shots=%d->%d energy_used=%d\n",
        hp_before, foe->hp, ammo_before, ammo_after,
        (int)player->upkeep->energy_use);
    printf("OUTCOME map_granite=%d fired=%d damage=%d\n",
        after_feat == FEAT_GRANITE ? 1 : 0, fired, hp_before - foe->hp);
    require(seen);
    require(k > 0);
    if (streq(mode, "wall")) {
        /* Correct contract: the Borg's map keeps the seen granite under
         * the pass-wall monster, and the Borg's own "never shoot walls"
         * rule holds. */
        eq(after_feat, FEAT_GRANITE);
        eq(fired, 0);
    } else {
        /* Control: the same monster on a floor grid is shot identically. */
        eq(after_feat, FEAT_FLOOR);
        eq(fired, 1);
    }
    ok;
}
const char *suite_name = "native-c/newkill-95";
struct test tests[] = {{"newly_seen_pass_wall_monster_keeps_wall", scenario}, {NULL, NULL}};
