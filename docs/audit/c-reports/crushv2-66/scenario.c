/* Synthetic native C Crush danger-reduction diagnostic, excluded from the source fix. */
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
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "borg/borg.h"
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
#include "borg/borg-item-use.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include <stdio.h>
#include <stdlib.h>

static term screen;
static struct monster *mon;
static struct object *rituals;

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

static int observe(struct monster *m, const char *tag)
{
    mflag_on(m->mflag, MFLAG_AWARE); update_mon(m, cave, true);
    if (!mflag_has(m->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL %s visible\n", tag); return 1; }
    if (!observe_kill_diff(m->grid.y, m->grid.x, 0, 0)) { printf("SETUPFAIL %s observe\n", tag); return 1; }
    int k = borg_grids[m->grid.y][m->grid.x].kill;
    if (k <= 0) { printf("SETUPFAIL %s kill_index\n", tag); return 1; }
    borg_kills[k].awake = 1;
    return 0;
}

static const char *mode_race(const char *mode)
{
    if (streq(mode, "archer")) return "cave orc";         /* crushable, innate arrows */
    if (streq(mode, "melee")) return "ogre";              /* crushable, no ranged attack */
    if (streq(mode, "tough")) return "dark dwarven lord"; /* caster, too many hit points to crush */
    return "blacklock mage";                              /* crushable ranged caster */
}

int setup_tests(void **state)
{
    const char *profile = getenv("CR66_PROFILE");
    const char *mode = getenv("CR66_MODE");
    if (!profile) return 1;
    if (!mode) mode = "caster";
    printf("SETUPSTEP profile\n");
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(66);
    if (!player_make_simple("Human", "Necromancer", "CrushRange")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 255, 60, 256); Term_activate(&screen); angband_term[0] = &screen;
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 20;
    character_dungeon = true;
    rituals = known_item(TV_SHADOW_BOOK, "[Dark Rituals]", 1);
    player_place(cave, player, loc(11, 11));
    player_exp_gain(player, player_exp[24] * player->expfact / 100);
    while (player_stat_inc(player, STAT_INT)) {}
    player_learn_all_runes(player); calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_TORCH | PU_HP | PU_MANA | PU_SPELLS;
    update_stuff(player);
    for (int i = 0; i < player->class->magic.total_spells; ++i)
        if (streq(spell_by_index(player, i)->name, "Crush")) spell_learn(i);
    player->chp = player->mhp; player->csp = player->msp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_do_spell = true; borg_notice(true);
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg_cheat_spells();
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    mon = t_add_monster(cave, loc(11, 16), mode_race(mode));   /* distance 5, in view */
    if (!mon) { printf("SETUPFAIL monster\n"); return 1; }
    if (observe(mon, "monster")) return 1;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=66 class=%s level=%d depth=%d hp=%d/%d sp=%d/%d\n",
        player->class->name, player->lev, player->depth, player->chp, player->mhp,
        player->csp, player->msp);
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
    const char *mode = getenv("CR66_MODE");
    keycode_t keys[64];
    int len;
    if (!mode) mode = "caster";
    printf("MODE %s\n", mode);
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;
    borg_kill *kill = &borg_kills[k];
    printf("TRACK race=%s hp=%d power=%d crush_limit=%d distance=%d spell_freq=%d\n",
        mon->race->name, mon->hp, kill->power, borg.trait[BI_CLEVEL] * 4,
        distance(borg.c, mon->grid), mon->race->freq_spell);

    /* The danger values the Crush evaluation compares (same calls as
     * borg_attack_aux_crush). */
    borg_crush_spell = false;
    int p1 = borg_danger(borg.c.y, borg.c.x, 4, true, false);
    borg_crush_spell = true;
    int p2 = borg_danger(borg.c.y, borg.c.x, 4, true, false);
    borg_crush_spell = false;
    borg_crush_spell = false;
    int k1 = borg_danger_one_kill(borg.c.y, borg.c.x, 4, k, true, true);
    borg_crush_spell = true;
    int k2 = borg_danger_one_kill(borg.c.y, borg.c.x, 4, k, true, true);
    borg_crush_spell = false;
    printf("TRACK danger without_crush=%d with_crush=%d reduction=%d monster_without=%d monster_with=%d\n", p1, p2, p1 - p2, k1, k2);

    /* The production attack chooser, acting mode. */
    bool acted = borg_attack(false);
    len = drain_keys(keys, 64);
    printf("DECISION acted=%d keys=%d sequence=", acted, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');
    /* Carry out the Borg's choice with the real cast command: resolve the
     * book letter and spell letter the Borg queued, then cast that spell. */
    int cast = 0, spell_index = -1, mon_hp_before = mon->hp, php_before = player->chp;
    const char *spell_name = "-";
    if (len >= 3 && keys[0] == 'm' && gear_to_label(player, rituals) == (char)keys[1]) {
        int *spells = NULL;
        int n = spell_collect_from_book(player, rituals, &spells);
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
    bool alive = square_monster(cave, loc(11, 16)) != NULL;
    printf("ENGINE spell=%s cast=%d monster_alive=%d monster_hp_before=%d player_hp=%d->%d energy_used=%d\n",
        spell_name, cast, alive ? 1 : 0, mon_hp_before, php_before, player->chp,
        (int)player->upkeep->energy_use);
    printf("TRACK engine spell=%s monster_alive=%d player_hp=%d->%d\n", spell_name, alive ? 1 : 0, php_before, player->chp);
    printf("OUTCOME acted=%d keys=%d cast_crush=%d monster_alive=%d\n", acted, len,
        cast && streq(spell_name, "Crush"), alive ? 1 : 0);
    if (streq(mode, "caster") || streq(mode, "archer")) {
        eq(acted ? 1 : 0, 1); require(len >= 3); require(keys[0] == 'm');
        require(cast && streq(spell_name, "Crush")); require(!alive);
    }
    else { eq(acted ? 1 : 0, 0); eq(len, 0); }
    ok;
}
const char *suite_name = "native-c/crushv2-66";
struct test tests[] = {{"crush_counts_ranged_danger", scenario}, {NULL, NULL}};
