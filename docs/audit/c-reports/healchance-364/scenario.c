/* Synthetic native C borg_heal() skip-gate fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#364 (healchance-364). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
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
#include "player-util.h"
#include "ui-menu.h"
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

/* z-rand.c keeps its WELL index here; Rand_state_init() does not reset it. */
extern uint32_t state_i;

static term screen;
#define H 24
#define W 40
#define TRIALS 100

static struct object *potions;
static struct monster *foe;
static int target_hp;

static void print_message(game_event_type type, game_event_data *data, void *user)
{
    if (data && data->message.msg) printf("MSG %s\n", data->message.msg);
}

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 30, MAXIMISE);
    obj->number = number;
    obj->known  = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static const char *mode_race(const char *mode)
{
    const char *over = getenv("HC364_RACE");
    if (over) return over;
    if (streq(mode, "danger")) return "fire giant";
    if (streq(mode, "unique")) return "Golfimbul, the Hill Orc Chief";
    return "cave orc"; /* control and risky */
}

int setup_tests(void **state)
{
    const char *profile = getenv("HC364_PROFILE");
    const char *mode    = getenv("HC364_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(364);
    if (!player_make_simple("Human", "Warrior", "HealChance364")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    player_exp_gain(player, player_exp[29] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    potions = known_item(TV_POTION, "Healing", 3);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    /* Fixture intervention: the "risky" mode turns on borg_plays_risky. */
    borg_cfg[BORG_PLAYS_RISKY] = streq(mode, "risky");
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
    /* One awake monster adjacent (south-east). */
    foe = t_add_monster(cave, loc(11, 13), mode_race(mode));
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    mflag_on(foe->mflag, MFLAG_AWARE); update_mon(foe, cave, true);
    if (!mflag_has(foe->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    if (!observe_kill_diff(foe->grid.y, foe->grid.x, 0, 0)) { printf("SETUPFAIL observe\n"); return 1; }
    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL kill_index\n"); return 1; }
    borg_kills[k].awake = 1;
    /* 60 % of the hit points lost: the 51-65 % band of the gate. */
    target_hp = player->mhp * 40 / 100;
    player->chp = target_hp;
    borg_notice(true);
    borg.avoidance = player->chp;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=364 mode=%s class=%s clevel=%d hp=%d/%d depth=%d race=%s unique=%d risky=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp, player->depth,
           foe->race->name, rf_has(foe->race->flags, RF_UNIQUE) ? 1 : 0,
           borg_cfg[BORG_PLAYS_RISKY] ? 1 : 0);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int drain_keys(keycode_t *keys, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) keys[len++] = key;
    return len;
}

/* One production decision: the borg_think_dungeon() unique scan, then
 * borg_caution(), with the RNG seeded per trial. */
static bool decide(uint32_t seed, keycode_t *keys, int *len, int *healed, bool show)
{
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg.goal.shop = borg.goal.ware = borg.goal.item = -1;
    borg_notice(true);
    borg.avoidance = borg.trait[BI_CURHP];
    borg_near_monster_type(borg.trait[BI_MAXCLEVEL] < 15 ? z_info->max_sight : 12);
    uint16_t before = messages_num();
    Rand_quick = false;
    state_i = 0;
    Rand_state_init(seed);
    bool rc = borg_caution();
    *len = drain_keys(keys, 64);
    *healed = 0;
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        if (strstr(m, "# Healing Level")) *healed = 1;
        if (show) printf("TRACK %s\n", m);
    }
    return rc;
}

static void print_keys(const keycode_t *keys, int len)
{
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Esc>");
        else if (keys[i] >= 32 && keys[i] < 127) printf("%c", (char)keys[i]);
        else printf("<%d>", (int)keys[i]);
    }
}

static int scenario_heal(void *state)
{
    const char *mode = getenv("HC364_MODE");
    keycode_t keys[64];
    int len, healed, heals = 0;
    borg_near_monster_type(borg.trait[BI_MAXCLEVEL] < 15 ? z_info->max_sight : 12);
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    int pct_down = (borg.trait[BI_MAXHP] - borg.trait[BI_CURHP]) * 100 / borg.trait[BI_MAXHP];
    printf("STATE danger=%d curhp=%d maxhp=%d pct_down=%d unique=%d risky=%d in_band=%d\n",
           danger, borg.trait[BI_CURHP], borg.trait[BI_MAXHP], pct_down,
           borg.near.unique, borg_cfg[BORG_PLAYS_RISKY] ? 1 : 0,
           (danger >= borg.trait[BI_CURHP] && danger < borg.trait[BI_MAXHP]) ? 1 : 0);
    printf("TRACK danger=%d hp=%d/%d pct_down=%d race=%s\n", danger,
           borg.trait[BI_CURHP], borg.trait[BI_MAXHP], pct_down, foe->race->name);

    /* 100 independent production decisions, seeds 1..100. */
    for (int t = 1; t <= TRIALS; ++t) {
        bool rc = decide((uint32_t)t, keys, &len, &healed, false);
        heals += healed;
        printf("TRIAL seed=%d rc=%d healed=%d keys=", t, rc ? 1 : 0, healed);
        print_keys(keys, len);
        putchar('\n');
    }
    printf("TRACK heals=%d of %d decisions\n", heals, TRIALS);
    /* Risky mode: the same seeds with borg_plays_risky off, for comparison. */
    int plain_heals = -1;
    if (streq(mode, "risky")) {
        borg_cfg[BORG_PLAYS_RISKY] = false;
        plain_heals = 0;
        for (int t = 1; t <= TRIALS; ++t) {
            bool rc = decide((uint32_t)t, keys, &len, &healed, false);
            plain_heals += healed;
            printf("TRIAL plain seed=%d rc=%d healed=%d keys=", t, rc ? 1 : 0, healed);
            print_keys(keys, len);
            putchar('\n');
        }
        borg_cfg[BORG_PLAYS_RISKY] = true;
        printf("TRACK same seeds without borg_plays_risky: heals=%d of %d\n", plain_heals, TRIALS);
    }
    for (int i = 0; i < z_info->pack_size; ++i)
        if (borg_items[i].iqty) printf("PACK %c %s\n", all_letters_nohjkl[i], borg_items[i].desc);

    /* Witness decision (seed 1000): carry out a queued quaff with the real
     * quaff command. */
    bool rc = decide(1000, keys, &len, &healed, true);
    printf("DECISION rc=%d healed=%d keys=", rc ? 1 : 0, healed);
    print_keys(keys, len);
    putchar('\n');
    /* Carry out a queued quaff with the real quaff command on the item
     * whose inventory letter the Borg chose. */
    int hp_before = player->chp, quaffed = 0, healing_left = potions->number;
    const char *drunk = "-";
    int start = 0;
    while (start < len && keys[start] == ESCAPE) start++;
    if (len - start >= 2 && keys[start] == 'q') {
        struct object *obj = NULL;
        for (struct object *o = player->gear; o; o = o->next)
            if (!object_is_equipped(player->body, o) && gear_to_label(player, o) == (char)keys[start + 1]) obj = o;
        if (obj && tval_is_potion(obj)) {
            drunk = obj->kind->name;
            player->upkeep->energy_use = 0;
            event_add_handler(EVENT_MESSAGE, print_message, NULL);
            cmdq_push(CMD_QUAFF);
            cmd_set_arg_item(cmdq_peek(), "item", obj);
            cmdq_execute(CTX_GAME);
            event_remove_handler(EVENT_MESSAGE, print_message, NULL);
            quaffed = 1;
        }
    }
    int drank_healing = quaffed && streq(drunk, "Healing");
    printf("ENGINE quaffed=%d potion=%s hp=%d->%d healing_potions=%d->%d\n", quaffed, drunk,
           hp_before, player->chp, healing_left, potions->number);
    printf("TRACK engine potion=%s hp=%d->%d\n", drunk, hp_before, player->chp);
    printf("OUTCOME heals=%d plain_heals=%d witness_healed=%d drank_healing=%d hp_after=%d\n", heals,
           plain_heals, healed, drank_healing, player->chp);

    /* Expected per the comments on the three adjustments (corrected signs). */
    if (streq(mode, "danger")) {
        require(danger >= target_hp && danger < player->mhp);
        require(heals >= 90);           /* original: 0 */
        require(drank_healing);
    } else if (streq(mode, "unique")) {
        require(borg.near.unique);
        require(heals > 23);            /* more often than the 23 of 100 the same seeds give with no unique (control) */
    } else if (streq(mode, "risky")) {
        require(heals < plain_heals);   /* risky must heal less often than not risky */
    } else {
        require(danger > 0 && danger < target_hp);
    }
    ok;
}

const char *suite_name = "native-c/healchance-364";
struct test tests[] = {
    {"heal_gate_adjustments", scenario_heal},
    {NULL, NULL}};
