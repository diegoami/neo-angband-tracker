/* Native C Teleport Other rod fixture, excluded from the source change.
 * Issue: diegoami/neo-angband-tracker#549 (torodvalue-549). */
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
#include "player-timed.h"
#include "player-util.h"
#include "target.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-attack.h"
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-flow.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-item-use.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40
#define PX 10
#define PY 12
#define LEV 25
#define DEPTH 25

static struct monster *foe;
static struct object *device;

static void print_message(game_event_type type, game_event_data *data, void *user)
{
    if (data && data->message.msg) printf("MSG %s\n", data->message.msg);
}

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) quit("fixture item kind missing");
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
    const char *profile = getenv("TR549_PROFILE");
    const char *mode    = getenv("TR549_MODE");
    const char *race;
    if (!profile || !mode) return 1;
    if (streq(mode, "rod") || streq(mode, "wand")) race = "hill giant";
    else if (streq(mode, "weak")) race = "soldier ant";
    else { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(549);
    if (!player_make_simple("Human", "Warrior", "TORod549")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = DEPTH;
    player_exp_gain(player, player_exp[LEV - 2] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* Empty the starting kit's carried items (keep what is worn). */
    {
        struct object *obj = player->gear, *next;
        while (obj) {
            next = obj->next;
            if (!object_is_equipped(player->body, obj)) {
                bool none_left = false;
                obj = gear_object_for_use(player, obj, obj->number, false, &none_left);
                if (obj->known) object_delete(player->cave, NULL, &obj->known);
                object_delete(cave, player->cave, &obj);
            }
            obj = next;
        }
    }
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    /* The one device: a charged Rod of Teleport Other, or (wand control)
     * a Wand of Teleport Other. */
    if (streq(mode, "wand")) device = known_item(TV_WAND, "Teleport Other", 1);
    else device = known_item(TV_ROD, "Teleport Other", 1);
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
    borg.c = player->grid; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    /* Wounded: a third of the hit points left. */
    player->chp = player->mhp * 33 / 100;
    borg_notice(true);
    borg.avoidance = borg.trait[BI_CURHP];
    foe = t_add_monster(cave, loc(PX + 1, PY), race);
    if (!foe) { printf("SETUPFAIL mon\n"); return 1; }
    mon_clear_timed(foe, MON_TMD_SLEEP, MON_TMD_FLG_NOTIFY);
    mflag_on(foe->mflag, MFLAG_AWARE); update_mon(foe, cave, true);
    if (!observe_kill_diff(foe->grid.y, foe->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill\n"); return 1; }
    borg_kills[k].awake = true;
    borg_near_monster_type(12);
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=549 mode=%s class=%s clevel=%d depth=%d hp=%d/%d device_skill=%d monster=%s adjacent\n",
           mode, player->class->name, player->lev, player->depth, player->chp, player->mhp,
           borg.trait[BI_DEV], race);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int drain_keys(char *buf, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) {
        buf[len++] = key == ESCAPE ? '~' : (char)key;
    }
    buf[len] = 0;
    return len;
}

static int scenario(void *state)
{
    const char *mode = getenv("TR549_MODE");
    char keys[65];
    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    struct loc start = foe->grid;
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    printf("STATE avoidance=%d grid_danger=%d monster_power=%d atportother=%d\n",
           borg.avoidance, danger, borg_kills[k].power, borg.trait[BI_ATPORTOTHER]);
    /* The production "try not to die" step of borg_think_dungeon(). */
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    bool caution = borg_caution();
    drain_keys(keys, 64);
    printf("DECISION caution=%d keys=\"%s\"\n", caution ? 1 : 0, keys);
    char step[16] = "caution";
    if (!caution) {
        /* Nothing chosen to avoid danger: the next fight step of
         * borg_think_dungeon() is the production attack chooser. */
        bool attack = borg_attack(false);
        drain_keys(keys, 64);
        my_strcpy(step, "attack", sizeof(step));
        printf("DECISION attack=%d keys=\"%s\"\n", attack ? 1 : 0, keys);
    }
    /* Decode the device command: 'z' (zap a rod) or 'a' (aim a wand)
     * followed by the inventory letter, then '5' for the target. */
    int zap = 0, aim = 0, slot = -1, attempts = 0;
    for (int i = 0; keys[i] && keys[i + 1]; ++i) {
        if (keys[i] != 'z' && keys[i] != 'a') continue;
        for (int j = 0; all_letters_nohjkl[j]; ++j)
            if (all_letters_nohjkl[j] == keys[i + 1]) slot = j;
        if (slot < 0) continue;
        if (keys[i] == 'z') zap = 1; else aim = 1;
        break;
    }
    struct object *used = (slot >= 0) ? player->upkeep->inven[slot] : NULL;
    printf("TRACK step=%s zap=%d aim=%d slot=%d item=%s target_key=%d\n", step, zap, aim,
           slot, used ? used->kind->name : "-", strchr(keys, '5') ? 1 : 0);
    if ((zap || aim) && used == device) {
        /* Carry out the Borg's choice with the real command at the
         * monster's grid (the target the Borg's '*' keys select). */
        event_add_handler(EVENT_MESSAGE, print_message, NULL);
        printf("TRACK device_fail_permille=%d\n", get_use_device_chance(device));
        /* One attempt per command (the game would auto-repeat a failed
         * attempt on later turns); at most five attempts, and the monster
         * does not act between them. */
        for (attempts = 1; attempts <= 5; ++attempts) {
            player->upkeep->energy_use = 0;
            target_set_location(start.y, start.x);
            cmdq_push_repeat(zap ? CMD_USE_ROD : CMD_USE_WAND, 1);
            cmd_set_arg_item(cmdq_peek(), "item", device);
            cmd_set_arg_target(cmdq_peek(), "target", DIR_TARGET);
            cmdq_execute(CTX_GAME);
            printf("ENGINE attempt=%d energy_used=%d monster_grid=(%d,%d)\n", attempts,
                   (int)player->upkeep->energy_use, foe->grid.x, foe->grid.y);
            if (!loc_eq(foe->grid, start)) break;
        }
        event_remove_handler(EVENT_MESSAGE, print_message, NULL);
    }
    int dist = distance(player->grid, foe->grid);
    printf("ENGINE monster_grid=(%d,%d)->(%d,%d) distance=%d->%d rod_charging=%d\n",
           start.x, start.y, foe->grid.x, foe->grid.y, distance(player->grid, start), dist,
           (tval_is_rod(device) && device->timeout) ? 1 : 0);
    int used_device = (zap || aim) && used == device;
    printf("OUTCOME used_teleport_other=%d monster_moved=%d\n", used_device,
           loc_eq(foe->grid, start) ? 0 : 1);
    if (streq(mode, "weak")) {
        eq(used_device, 0);
    } else {
        eq(used_device, 1);
    }
    ok;
}

const char *suite_name = "native-c/torodvalue-549";
struct test tests[] = {
    {"teleport_other_device", scenario},
    {NULL, NULL}};
