/* Native C observer for the dead-unique fix-up in borg_update_kill_old(), excluded from the source fix. */
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
#include "game-input.h"
#include "mon-lore.h"
#include "ui-prefs.h"
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
#include "borg/borg-prepared.h"
#include "borg/borg-escape.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-think-dungeon.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



static term screen;
static struct monster_race *grip, *quaker;
static char borg_answer;

static void print_message(game_event_type type, game_event_data *data, void *user)
{
    if (data && data->message.msg) printf("MSG %s\n", data->message.msg);
}

static bool answer_with_borg_key(const char *prompt)
{
    printf("PROMPT %s-> %c\n", prompt, borg_answer ? borg_answer : '?');
    return borg_answer == 'y';
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

static struct object *recall;

static int observe(struct monster *mon)
{
    mon_clear_timed(mon, MON_TMD_SLEEP, MON_TMD_FLG_NOTIFY);
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!mflag_has(mon->mflag, MFLAG_VISIBLE)) return -1;
    if (!observe_kill_diff(mon->grid.y, mon->grid.x, 0, 0)) return -1;
    return borg_grids[mon->grid.y][mon->grid.x].kill;
}

static void unique_summary(const char *tag)
{
    printf("STATE %s live_uniques=%d hunted_depth=%d grip_dead=%d quaker_dead=%d\n",
        tag, borg_numb_live_unique, borg_depth_hunted_unique,
        borg_race_death[grip->ridx] ? 1 : 0, borg_race_death[quaker->ridx] ? 1 : 0);
    printf("TRACK %s first_living_unique=%s\n", tag,
        borg_numb_live_unique ? r_info[borg_first_living_unique].name : "-");
}

int setup_tests(void **state)
{
    const char *profile = getenv("U94_PROFILE");
    const char *mode = getenv("U94_MODE");
    if (!profile) return 1;
    if (!mode) mode = "grip";
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(94);
    if (!player_make_simple("Human", "Warrior", "UniqueDead")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(40, 40); player->cave = cave_new(40, 40);
    player->depth = cave->depth = player->cave->depth = 15;
    player->max_depth = player->recall_depth = 60;
    player_exp_gain(player, player_exp[29] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < 39; ++y) for (int x = 1; x < 39; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    recall = known_item(TV_SCROLL, "Word of Recall", 1);
    player_place(cave, player, loc(20, 20));
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;

    /* The character's unique kills, recorded as the game records a slain
     * unique (mon-util.c:1101, load.c:533-535): every unique shallower
     * than depth 40 except Grip, Fang and Nar, plus Quaker (depth 43). */
    grip = lookup_monster("Grip, Farmer Maggot's Dog");
    quaker = lookup_monster("Quaker, Master of Earth");
    if (!grip || !quaker) { printf("SETUPFAIL races\n"); return 1; }
    int killed = 0;
    for (int i = 1; i < z_info->r_max; i++) {
        struct monster_race *r = &r_info[i];
        if (!r->name || !rf_has(r->flags, RF_UNIQUE)) continue;
        if (r == grip || streq(r->name, "Fang, Farmer Maggot's Dog")
            || streq(r->name, "N\xc3\xa1r, the Dwarf")) continue;
        if (r->level < 40 || r == quaker) {
            r->max_num = 0; get_lore(r)->pkills = 1; killed++;
        }
    }
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_alloc_detection();
    update_view(cave, player);
    borg.c = player->grid; borg.goal.g = player->grid;
    borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.status.active = true;
    for (int y = 0; y < 40; ++y) for (int x = 0; x < 40; ++x)
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    /* First map update on this level: the new-level branch runs the
     * production unique scan (borg-update.c:2273-2338). */
    borg_find_all_items(); borg_notice(true);
    borg_update();
    printf("SETUP seed=94 mode=%s class=%s clevel=%d depth=%d max_depth=%d recall_depth=%d uniques_killed=%d grip_ridx=%d quaker_ridx=%d\n",
        mode, player->class->name, player->lev, player->depth, player->max_depth,
        player->recall_depth, killed, grip->ridx, quaker->ridx);
    unique_summary("start");
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    const char *mode = getenv("U94_MODE");
    if (!mode) mode = "grip";
    printf("MODE %s\n", mode);
    /* Earlier sightings fill the Borg's monster records 1..N-1 so the
     * next record is slot N: N = Grip's race index (grip) or 5 (control). */
    int want = streq(mode, "grip") ? (int)grip->ridx : 5;
    int placed = 0;
    for (int y = 3; y < 38 && borg_kills_nxt < want; y += 3)
        for (int x = 3; x < 38 && borg_kills_nxt < want; x += 3) {
            if (y == 21 && x == 24) continue;
            if (distance(loc(x, y), player->grid) > 16) continue;
            struct monster *m = t_add_monster(cave, loc(x, y), "wild dog");
            if (!m || observe(m) <= 0) { printf("SETUPFAIL filler\n"); return 1; }
            placed++;
        }
    require(borg_kills_nxt == want);
    /* A Maia of Aule changes into Quaker, Master of Earth (one of its
     * three stock shapes) with the game's own shape-change routine. */
    struct monster *maia = t_add_monster(cave, loc(24, 21), "Maia of Aul\xc3\xab");
    require(maia != NULL);
    int tries = 0;
    while (maia->race != quaker && tries < 50) {
        monster_revert_shape(maia);
        monster_change_shape(maia);
        tries++;
    }
    require(maia->race == quaker);
    int slot = observe(maia);
    printf("TRACK filler_records=%d shape_tries=%d new_record_slot=%d race_seen=%s max_num=%d\n",
        placed, tries, slot, slot > 0 ? r_info[borg_kills[slot].r_idx].name : "-",
        quaker->max_num);
    printf("TRACK race_at_slot_index=%s\n", r_info[slot].name);

    /* Later the Borg arrives on a new level: the same production unique
     * scan runs again (borg-update.c:2273-2338). */
    borg.status.old_depth = 1;
    borg_update();
    unique_summary("next_level");

    /* Production recall decision (borg-escape.c:55-122) on that level. */
    borg.goal.recalling = false;
    borg_notice(true);
    borg_flush();
    bool acted = borg_recall();
    char keys[40]; int nk = 0; keycode_t key;
    while (nk < 39 && (key = borg_inkey(true)) != 0)
        keys[nk++] = (key > 31 && key < 127) ? (char)key : '.';
    keys[nk] = 0;
    borg_flush();
    borg_answer = 0;
    for (int i = 0; i + 1 < nk; i++)
        if (keys[i] == 'r') { borg_answer = (i + 2 < nk) ? keys[i + 2] : 0; break; }
    printf("DECISION recall=%d keys=<%s> answer=%c\n", acted ? 1 : 0, keys,
        borg_answer ? borg_answer : '?');
    /* Carry out the read with the real command; the game's recall-depth
     * question gets the Borg's queued answer. */
    if (acted) {
        get_check_hook = answer_with_borg_key;
        event_add_handler(EVENT_MESSAGE, print_message, NULL);
        cmdq_push(CMD_READ_SCROLL);
        cmd_set_arg_item(cmdq_peek(), "item", recall);
        cmdq_execute(CTX_GAME);
        event_remove_handler(EVENT_MESSAGE, print_message, NULL);
        get_check_hook = NULL;
    }
    printf("ENGINE recall_active=%d recall_depth=%d max_depth=%d\n",
        player->word_recall ? 1 : 0, player->recall_depth, player->max_depth);
    printf("OUTCOME grip_dead=%d live_uniques=%d hunted_depth=%d reset_recall=%d recall_depth=%d\n",
        borg_race_death[grip->ridx] ? 1 : 0, borg_numb_live_unique,
        borg_depth_hunted_unique, borg_answer == 'y' ? 1 : 0, player->recall_depth);
    /* Correct contract: the dead unique the Borg saw (Quaker) is the one
     * recorded dead; living Grip stays alive in the Borg's list, and the
     * recall decision follows the true living uniques. */
    require(acted);
    eq(borg_race_death[quaker->ridx] ? 1 : 0, 1);
    eq(borg_race_death[grip->ridx] ? 1 : 0, 0);
    eq(borg_depth_hunted_unique, 17);
    eq(borg_answer, 'y');
    eq(player->recall_depth, 15);
    ok;
}
const char *suite_name = "native-c/uniquedead-94";
struct test tests[] = {{"dead_unique_fixup_marks_the_seen_race", scenario}, {NULL, NULL}};
