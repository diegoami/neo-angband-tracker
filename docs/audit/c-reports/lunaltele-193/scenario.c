/* Synthetic native C Lunal-mode teleport fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#193 (lunaltele-193).
 *
 * The Borg runs with Lunal mode configured (borg_lunal_mode = TRUE in the
 * profile borg.txt).  A monster next to it casts a spell through the
 * game's own monster spell code (do_mon_spell): teleport-to, teleport-away
 * or, as a control, shriek.  An EVENT_MESSAGE handler records each line the
 * game prints and hands it to the Borg's message parser (borg_parse), the
 * function its screen reader calls with each message line.  The Borg then
 * runs its map/message update (borg_update) and one production stair
 * decision (borg_flow_stair_more, as called by the dungeon thinker when it
 * is bored on a level).  The decision is printed together with the Lunal
 * flag. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-event.h"
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
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
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
#include "borg/borg-messages.h"
#include "borg/borg-prepared.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40

static char captured[16][256];
static int  n_captured;
static struct monster *caster;
static const char *mode;

static void capture(game_event_type type, game_event_data *data, void *user)
{
    if (n_captured < 16 && data->message.msg)
        my_strcpy(captured[n_captured++], data->message.msg, 256);
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

int setup_tests(void **state)
{
    const char *profile = getenv("LT193_PROFILE");
    mode = getenv("LT193_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(193);
    if (!player_make_simple("Human", "Warrior", "LunalTele193")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 10;
    player_exp_gain(player, player_exp[13] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* One down staircase and one up staircase on the level. */
    square_set_feat(cave, loc(24, 18), FEAT_MORE);
    square_set_feat(cave, loc(3, 3), FEAT_LESS);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    /* Control mode: the same profile with Lunal mode not in effect. */
    if (streq(mode, "teleto-nolunal")) borg.lunal_mode = false;
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    update_view(cave, player);
    borg.c = player->grid; borg.goal.g = player->grid;
    borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    /* The spell caster, three grids east of the Borg. */
    const char *race = prefix(mode, "teleaway") ? "mage" : streq(mode, "shriek") ? "shrieker mushroom patch" : "tengu";
    caster = t_add_monster(cave, loc(13, 12), race);
    if (!caster) { printf("SETUPFAIL caster\n"); return 1; }
    mflag_on(caster->mflag, MFLAG_AWARE); update_mon(caster, cave, true);
    if (!monster_is_visible(caster)) { printf("SETUPFAIL visible\n"); return 1; }
    borg.status.active = true;
    /* The Borg's own map update sees the arena, the stairs and the caster. */
    borg_update();
    event_add_handler(EVENT_MESSAGE, capture, NULL);
    printf("SETUP seed=193 mode=%s class=%s clevel=%d depth=%d caster=%s(%d,%d) borg=(%d,%d) "
           "stairs_down=%d stairs_up=%d lunal_config=%d\n",
           mode, player->class->name, player->lev, player->depth, race,
           caster->grid.x, caster->grid.y, borg.c.x, borg.c.y,
           track_more.num, track_less.num, borg_cfg[BORG_LUNAL_MODE] ? 1 : 0);
    return 0;
}

int teardown_tests(void *state)
{
    event_remove_handler(EVENT_MESSAGE, capture, NULL);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    int spell = RSF_TELE_TO;
    bool seen = true;
    if (streq(mode, "teleaway")) spell = RSF_TELE_AWAY;
    else if (streq(mode, "teleaway-unseen")) {
        spell = RSF_TELE_AWAY;
        /* Fixture intervention: the spell is cast as by a monster the
         * player cannot see, so the game prints its unseen-caster line. */
        seen = false;
    }
    else if (streq(mode, "shriek")) spell = RSF_SHRIEK;

    struct loc from = player->grid;
    int lunal_before = borg.lunal_mode ? 1 : 0;

    /* The game's monster spell code casts the spell (as make_ranged_attack
     * does once the monster has chosen it) and prints its message. */
    do_mon_spell(spell, caster, seen && monster_is_visible(caster));
    update_stuff(player);
    printf("TRACK player-moved (%d,%d)->(%d,%d) caster-at (%d,%d)\n", from.x,
           from.y, player->grid.x, player->grid.y, caster->grid.x, caster->grid.y);

    int before = borg_msg_num;
    for (int i = 0; i < n_captured; ++i) {
        printf("TRACK game-message <%s>\n", captured[i]);
        borg_parse(captured[i]);
    }
    borg_parse(NULL);
    for (int i = before; i < borg_msg_num; ++i)
        printf("TRACK borg-reaction <%s>\n", borg_msg_buf + borg_msg_pos[i]);
    if (borg_msg_num == before) printf("TRACK borg-reaction <none>\n");

    /* Several Borg update passes, as successive game turns would run. */
    for (int pass = 0; pass < 3; ++pass) borg_update();
    borg_notice(true);
    int lunal_after = borg.lunal_mode ? 1 : 0;
    printf("STATE lunal_mode before=%d after=%d borg=(%d,%d)\n", lunal_before,
           lunal_after, borg.c.x, borg.c.y);

    /* The production stair decision the dungeon thinker makes when it has
     * nothing better to do on the level (borg_think_dungeon_util.c,
     * GOAL_BORE): head for a down staircase, unless not prepared for the
     * next depth and not in Lunal or munchkin mode. */
    const char *unprepared = borg_prepared(borg.trait[BI_CDEPTH] + 1);
    printf("STATE prepared_for_depth_%d=%s\n", borg.trait[BI_CDEPTH] + 1,
           unprepared ? unprepared : "yes");
    borg_flush();
    bool down = borg_flow_stair_more(GOAL_BORE, false, false);
    keycode_t key = borg_inkey(false);
    printf("DECISION heads_for_down_stairs=%d goal_type=%d first_key=%d\n",
           down ? 1 : 0, borg.goal.type, (int)key);
    printf("OUTCOME lunal_after=%d heads_down=%d\n", lunal_after, down ? 1 : 0);
    require(n_captured >= 1);
    /* Corrected contract: after a monster spell has moved the Borg away
     * from its grid, Lunal mode is off and the ordinary preparedness check
     * decides whether to take the stairs down. */
    if (streq(mode, "teleto") || prefix(mode, "teleaway")) {
        require(!lunal_after);
        require(!down);
    }
    ok;
}

const char *suite_name = "native-c/lunaltele-193";
struct test tests[] = {
    {"lunal_mode_after_monster_spell", scenario},
    {NULL, NULL}};
