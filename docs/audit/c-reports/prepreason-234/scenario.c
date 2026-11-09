/* Synthetic native C leave-level fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#234 (prepreason-234). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "object.h"
#include "obj-desc.h"
#include "obj-gear.h"
#include "obj-knowledge.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "store.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-home-notice.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-prepared.h"
#include "borg/borg-think-dungeon-util.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40

static void make_known(struct object *obj)
{
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    player_know_object(player, obj);
}

static struct object *new_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 100, MAXIMISE);
    obj->number = number;
    return obj;
}

static struct object *carry(struct object *obj)
{
    make_known(obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static struct object *carry_item(int tval, const char *name, int number)
{
    return carry(new_item(tval, name, number));
}

static void wear(struct object *obj)
{
    inven_wield(obj, wield_slot(obj));
}

static struct object *artifact_item(const char *name)
{
    const struct artifact *art = lookup_artifact_name(name);
    if (!art) { fprintf(stderr, "MISSING ARTIFACT %s\n", name); quit("fixture artifact missing"); }
    struct object *obj = object_new();
    object_prep(obj, lookup_kind(art->tval, art->sval), 100, MAXIMISE);
    obj->artifact = art;
    copy_artifact_data(obj, art);
    return carry(obj);
}

/* Modes: current depth, no_deeper setting, *Healing* potions carried. */
struct mode_def { const char *name; int depth; int no_deeper; int ezheal; };
static const struct mode_def modes[] = {
    {"nodeeper100", 99, 100, 15},
    {"default127", 99, 0, 15},
    {"prepared97", 97, 100, 30},
};
static const struct mode_def *md;

int setup_tests(void **state)
{
    const char *profile = getenv("PR234_PROFILE");
    const char *mode    = getenv("PR234_MODE");
    if (!profile || !mode) return 1;
    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++)
        if (streq(modes[i].name, mode)) md = &modes[i];
    if (!md) { printf("SETUPFAIL mode\n"); return 1; }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(234);
    if (!player_make_simple("Human", "Warrior", "PrepReason234")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = cave->depth = player->cave->depth = md->depth;
    player->max_depth = 99;
    player_exp_gain(player, player_exp[48] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* An up staircase far east. */
    square_set_feat(cave, loc(35, 12), FEAT_LESS);
    /* Remove the starting kit; the endgame kit follows. */
    for (struct object *o = player->gear, *next; o; o = next) {
        next = o->next;
        bool none_left = false;
        struct object *used = gear_object_for_use(player, o, o->number, false, &none_left);
        if (used->known) object_delete(NULL, NULL, &used->known);
        object_delete(NULL, NULL, &used);
    }
    /* Worn: every resist the depth-99 checks ask for, free action,
     * telepathy and +20 speed, all from real game items. */
    wear(artifact_item("'Bladeturner'"));
    wear(artifact_item("'Dal-i-thalion'"));
    wear(carry_item(TV_AMULET, "ESP", 1));
    wear(carry_item(TV_RING, "Speed", 1));
    wear(carry_item(TV_RING, "Speed", 1));
    wear(carry_item(TV_HAFTED, "Mace of Disruption", 1));
    {
        struct object *lantern = carry_item(TV_LIGHT, "Lantern", 1);
        lantern->timeout = 15000;
        wear(lantern);
    }
    /* Carried supplies. */
    carry_item(TV_FOOD, "Ration of Food", 8);
    carry_item(TV_FLASK, "Flask of Oil", 5);
    carry_item(TV_SCROLL, "Word of Recall", 4);
    carry_item(TV_SCROLL, "Teleportation", 8);
    carry_item(TV_SCROLL, "Teleport Level", 3);
    carry_item(TV_SCROLL, "Phase Door", 10);
    carry_item(TV_POTION, "Healing", 5);
    carry_item(TV_POTION, "Speed", 15);
    carry_item(TV_POTION, "*Healing*", md->ezheal);
    /* Every rune known, as for a character this far into the game. */
    player_learn_all_runes(player);
    for (struct object *o = player->gear; o; o = o->next) player_know_object(player, o);
    /* Fixture intervention: maximised STR, DEX and CON (18/100), as after
     * the potions of strength, dexterity and toughness of a long game. */
    player->stat_max[STAT_STR] = player->stat_cur[STAT_STR] = 18 + 100;
    player->stat_max[STAT_DEX] = player->stat_cur[STAT_DEX] = 18 + 100;
    player->stat_max[STAT_CON] = player->stat_cur[STAT_CON] = 18 + 100;
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_cfg[BORG_USES_DYNAMIC_CALCS] = false;
    /* Fixture intervention: the same as the borg.txt line
     * "borg_no_deeper = 100" (stop before Morgoth's level). */
    if (md->no_deeper) borg_cfg[BORG_NO_DEEPER] = md->no_deeper;
    /* The home holds 20 Potions of *Healing* stored on earlier trips. */
    for (int i = 0; i < z_info->store_max; i++) {
        struct store *s = &stores[i];
        s->stock_num = 0;
        object_pile_free(NULL, NULL, s->stock_k);
        object_pile_free(NULL, NULL, s->stock);
        s->stock_k = NULL;
        s->stock = NULL;
    }
    {
        struct store *home = NULL;
        for (int i = 0; i < z_info->store_max; i++)
            if (stores[i].feat == FEAT_HOME) home = &stores[i];
        if (!home) { printf("SETUPFAIL home\n"); return 1; }
        struct object *obj = new_item(TV_POTION, "*Healing*", 20);
        make_known(obj);
        if (!store_carry(home, obj, false)) { printf("SETUPFAIL store\n"); return 1; }
    }
    borg_find_all_items();
    borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* The Borg has seen the up staircase. */
    track_less.num = track_more.num = 0;
    track_less.x[0] = 35; track_less.y[0] = 12; track_less.num = 1;
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    /* Just arrived on this level; last in town a while ago. */
    borg.time.now   = 50000;
    borg.time.level = borg.time.now;
    borg.time.town  = borg.time.now - 5000;
    borg.status.active = true;
    printf("SETUP seed=234 mode=%s class=%s clevel=%d depth=%d max_depth=%d no_deeper=%d\n",
           mode, player->class->name, player->lev, player->depth, player->max_depth,
           borg_cfg[BORG_NO_DEEPER]);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_leave(void *state)
{
    char cur[128], next[128];
    const char *r;
    borg_notice(true);
    printf("TRAITS hp=%d speed=%d str=%d dex=%d con=%d light=%d food=%d recall=%d tele=%d "
           "fract=%d esp=%d rfire=%d rcold=%d racid=%d relec=%d rpois=%d rconf=%d rblind=%d "
           "rkaos=%d rdis=%d ezheal=%d heal=%d aspeed=%d bigfight=%d\n",
           borg.trait[BI_MAXHP], borg.trait[BI_SPEED], borg.trait[BI_STR], borg.trait[BI_DEX],
           borg.trait[BI_CON], borg.trait[BI_LIGHT], borg.trait[BI_FOOD], borg.trait[BI_RECALL],
           borg.trait[BI_ATELEPORT], borg.trait[BI_FRACT], borg.trait[BI_ESP],
           borg.trait[BI_SRFIRE], borg.trait[BI_SRCOLD], borg.trait[BI_SRACID],
           borg.trait[BI_SRELEC], borg.trait[BI_SRPOIS], borg.trait[BI_SRCONF],
           borg.trait[BI_SRBLIND], borg.trait[BI_SRKAOS], borg.trait[BI_SRDIS],
           borg.trait[BI_AEZHEAL], borg.trait[BI_AHEAL], borg.trait[BI_ASPEED],
           borg.trait[BI_PREP_BIG_FIGHT]);
    /* The reasons each depth gives on its own (copied at once). */
    r = borg_prepared(borg.trait[BI_CDEPTH]);
    my_strcpy(cur, r ? r : "(prepared)", sizeof cur);
    r = borg_prepared(borg.trait[BI_CDEPTH] + 1);
    my_strcpy(next, r ? r : "(prepared)", sizeof next);
    printf("PREP cur=%s next=%s\n", cur, next);
    borg_notice_home(NULL, false);
    printf("HOME ezheal=%d\n", num_ezheal_true);

    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    bool rc = borg_leave_level(false);
    char keys[128];
    int nk = 0;
    for (keycode_t k; nk < 120 && (k = borg_inkey(true)) != 0; )
        keys[nk++] = (k >= 32 && k < 127) ? (char)k : '?';
    keys[nk] = '\0';
    uint16_t after = messages_num();
    int collect = 0, recall = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (strstr(m, "Returning to town to Collect stock")) collect = 1;
        if (strstr(m, "Recalling to town")) recall = 1;
    }
    printf("DECISION rc=%d rising=%d keys=%s\n", rc ? 1 : 0, borg.goal.rising ? 1 : 0, keys);
    printf("OUTCOME collect=%d recall=%d\n", collect, recall);
    /* Correct contract: when the current depth's own reason is
     * "Collect from house", the Borg heads to town to collect it. */
    if (strstr(cur, "Collect from house"))
        require(collect && recall); /* fails before when the next depth says "No deeper" */
    else
        require(!collect);
    ok;
}

const char *suite_name = "native-c/prepreason-234";
struct test tests[] = {
    {"collect_from_house_decision", scenario_leave},
    {NULL, NULL}};
