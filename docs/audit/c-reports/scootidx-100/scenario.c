/* Synthetic native C Shoot'N'Scoot fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#100 (scootidx-100). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmd-core.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-attack.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
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
#include "ui-input.h"
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
#include "borg/borg-magic.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 40
#define W 40
#define PY 20
#define PX 20

static struct monster *foe;
static struct monster *caster;
static struct object *book;

static void print_message(game_event_type type, game_event_data *data, void *user)
{
    if (data && data->message.msg) printf("MSG %s\n", data->message.msg);
}

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 0, AVERAGE);
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

/* "north" and "pack": the adjacent monster stands north of the Borg, the
 * direction slot numbered 1, which is also the kill index of the caster.
 * "south": the adjacent monster stands south, direction slot 0 (an unused
 * kill index). */
static struct loc mode_grid(const char *mode)
{
    if (streq(mode, "south")) return loc(PX, PY + 1);
    return loc(PX, PY - 1);
}

/* "pack": the adjacent monster is an olog, a pack monster close to the
 * Borg's level, so it qualifies for Shoot'N'Scoot without the danger test. */
static const char *mode_race(const char *mode)
{
    if (streq(mode, "pack")) return "olog";
    return "mummified orc";
}

int setup_tests(void **state)
{
    const char *profile = getenv("SC100_PROFILE");
    const char *mode    = getenv("SC100_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(100);
    if (!player_make_simple("Human", "Rogue", "Scoot100")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 30;
    player_exp_gain(player, player_exp[36] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* A walled corridor running west from the grid north of the Borg.  The
     * caster at its far end can see that grid but not the Borg's grid. */
    for (int y = PY - 2; y <= PY; ++y) for (int x = PX - 8; x <= PX - 2; ++x)
        square_set_feat(cave, loc(x, y), FEAT_GRANITE);
    for (int x = PX - 7; x <= PX - 2; ++x)
        square_set_feat(cave, loc(x, PY - 1), FEAT_FLOOR);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    struct object *sling = known_item(TV_BOW, "Sling", 1);
    inven_wield(sling, wield_slot(sling));
    known_item(TV_SHOT, "Iron Shot", 30);
    known_item(TV_SCROLL, "Phase Door", 5);
    player_learn_all_runes(player); calc_inventory(player);
    /* The starting kit's [First Spells]. */
    for (int z = 0; z < z_info->pack_size && !book; ++z)
        if (player->upkeep->inven[z] && tval_is_book_k(player->upkeep->inven[z]->kind)) book = player->upkeep->inven[z];
    if (!book) { printf("SETUPFAIL book\n"); return 1; }
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Phase Door")) spell_learn(i);
    player->chp = player->mhp; player->csp = player->msp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg_cheat_spells();
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid; borg.ready_morgoth = 0;
    borg.time.now = 1000;
    borg.time.level = borg.time.now;
    borg.time.antisummon = 0;
    borg.time.town = borg.time.now;
    borg_notice(true);
    borg.avoidance = player->chp;
    /* An awake ogre mage at the far end of the corridor.  The Borg saw it
     * a moment ago from the grid north of where it now stands; from its
     * current grid the caster is out of sight.  Placed first, so it is the
     * Borg's kill index 1. */
    caster = t_add_monster(cave, loc(PX - 6, PY - 1), "ogre mage");
    if (!caster) { printf("SETUPFAIL caster\n"); return 1; }
    monster_swap(player->grid, loc(PX, PY - 1)); update_view(cave, player); borg.c = player->grid;
    if (observe(caster, "caster")) return 1;
    monster_swap(player->grid, loc(PX, PY)); update_view(cave, player); borg.c = player->grid;
    update_mon(caster, cave, true);
    foe = t_add_monster(cave, mode_grid(mode), mode_race(mode));
    if (!foe) { printf("SETUPFAIL foe\n"); return 1; }
    if (observe(foe, "foe")) return 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=100 mode=%s class=%s clevel=%d hp=%d sp=%d/%d depth=%d missiles=%d\n",
           mode, player->class->name, player->lev, player->chp, player->csp, player->msp,
           player->depth, borg.trait[BI_AMISSILES]);
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

static int scenario(void *state)
{
    const char *mode = getenv("SC100_MODE");
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    int k = borg_grids[foe->grid.y][foe->grid.x].kill;
    int dir = -1;
    for (int i = 0; i < 8; ++i)
        if (borg.c.x + ddx_ddd[i] == foe->grid.x && borg.c.y + ddy_ddd[i] == foe->grid.y) dir = i;
    /* The two values the adjacent-monster test can read: the danger of the
     * adjacent monster itself, and the danger of whatever monster holds the
     * kill index equal to the direction number. */
    int danger = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    int own = borg_danger_one_kill(foe->grid.y, foe->grid.x, 1, k, true, false);
    int slot = borg_danger_one_kill(foe->grid.y, foe->grid.x, 1, dir, true, false);
    printf("TRACK adjacent=%s kill_index=%d direction=%d ranged=%d caster_in_view=%d\n",
           foe->race->name, k, dir, borg_kills[k].ranged_attack,
           monster_is_in_view(caster) ? 1 : 0);
    for (int j = 1; j < borg_kills_nxt; ++j)
        printf("KILL k=%d race=%s pos=(%d,%d) awake=%d\n", j, r_info[borg_kills[j].r_idx].name,
               borg_kills[j].pos.x, borg_kills[j].pos.y, borg_kills[j].awake);
    printf("TRACK danger here=%d adjacent_monster=%d direction_slot_monster=%d threshold=%d\n",
           danger, own, slot, borg.avoidance * 3 / 10);
    printf("DANGER here=%d adjacent_monster=%d direction_slot_monster=%d threshold=%d hp=%d\n",
           danger, own, slot, borg.avoidance * 3 / 10, borg.avoidance);

    /* The production caution routine, once; read back the queued keys. */
    bool rc = borg_caution();
    keycode_t keys[64];
    int len = drain_keys(keys, 64);
    printf("DECISION rc=%d keys=%d sequence=", rc ? 1 : 0, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Escape>");
        else if (keys[i] >= 32 && keys[i] < 127) printf("%c", (char)keys[i]);
        else printf("<%d>", (int)keys[i]);
    }
    putchar('\n');
    uint16_t after = messages_num();
    int scoot = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Shoot N Scoot")) scoot = 1;
    }

    /* Carry out the Borg's choice with the real cast command: resolve the
     * book letter and spell letter it queued, then cast that spell. */
    int spell_index = -1, cast = 0;
    const char *spell_name = "-";
    struct loc from = player->grid;
    int sp_before = player->csp;
    player->upkeep->energy_use = 0;
    if (len >= 3 && keys[0] == 'm' && gear_to_label(player, book) == (char)keys[1]) {
        int *spells = NULL;
        int n = spell_collect_from_book(player, book, &spells);
        int pos = (int)keys[2] - 'a';
        if (pos >= 0 && pos < n) spell_index = spells[pos];
        mem_free(spells);
    }
    if (spell_index >= 0) {
        spell_name = spell_by_index(player, spell_index)->name;
        player->upkeep->energy_use = 0;
        event_add_handler(EVENT_MESSAGE, print_message, NULL);
        cmdq_push(CMD_CAST);
        cmd_set_arg_choice(cmdq_peek(), "spell", spell_index);
        cmdq_execute(CTX_GAME);
        event_remove_handler(EVENT_MESSAGE, print_message, NULL);
        cast = 1;
    }
    update_mon(caster, cave, true);
    int seen = square_isview(cave, caster->grid) ? 1 : 0;
    printf("ENGINE spell=%s cast=%d grid=(%d,%d)->(%d,%d) sp=%d->%d energy_used=%d caster_in_view=%d\n",
           spell_name, cast, from.x, from.y, player->grid.x, player->grid.y,
           sp_before, player->csp, (int)player->upkeep->energy_use, seen);
    printf("TRACK engine spell=%s grid=(%d,%d)->(%d,%d) sp=%d->%d\n", spell_name,
           from.x, from.y, player->grid.x, player->grid.y, sp_before, player->csp);
    printf("OUTCOME scoot=%d cast_phase=%d\n", scoot, cast && streq(spell_name, "Phase Door"));
    if (streq(mode, "north"))
        require(!scoot && !cast); /* fails before: the caster's danger is read */
    else if (streq(mode, "pack"))
        require(scoot && cast && streq(spell_name, "Phase Door"));
    else
        require(!scoot && !cast);
    ok;
}

const char *suite_name = "native-c/scootidx-100";
struct test tests[] = {
    {"shoot_scoot_from_adjacent_monster", scenario},
    {NULL, NULL}};
