/* Synthetic native C dead-unique start-up fixture, excluded from the source
 * fix.  Issue: diegoami/neo-angband-tracker#101 (deaduniq-101).
 *
 * A late-game Warrior (maximum depth 99) on depth 98 has already killed
 * Sauron before the Borg is started (r_info max_num 0, as the savefile
 * loader sets it for a unique with a recorded kill).  It carries Scrolls of
 * Mass Banishment, wears a Metal Cap and carries a spare Hard Leather Cap.
 * The Borg is initialised with the production borg_init(), then the
 * fixture runs the first decision pass in the order borg_think() uses
 * (borg-think.c:211-217: borg_notice, borg_update, borg.power =
 * borg_power()) and the production equipment decision borg_wear_stuff(),
 * which compares each candidate swap against that borg.power. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-event.h"
#include "game-world.h"
#include "init.h"
#include "mon-lore.h"
#include "mon-make.h"
#include "mon-util.h"
#include "monster.h"
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
#include "ui-menu.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-item-wear.h"
#include "borg/borg-junk.h"
#include "borg/borg-power.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40

static const char *mode;

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 50, MINIMISE);
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
    const char *profile = getenv("DU101_PROFILE");
    mode = getenv("DU101_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(101);
    if (!player_make_simple("Human", "Warrior", "DeadUnique101")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = cave->depth = player->cave->depth = 98;
    player->max_depth = 99;
    player_exp_gain(player, player_exp[39] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    struct object *cap = known_item(TV_HELM, "Metal Cap", 1);
    inven_wield(cap, wield_slot(cap));
    /* The birth kit's spare torches are removed so that the only spare
     * wearable is the decision being observed. */
    for (struct object *o = player->gear; o; ) {
        struct object *next = o->next;
        if ((o->tval == TV_LIGHT || (strstr(mode, "nokitpotion") && o->tval == TV_POTION)) && !object_is_equipped(player->body, o)) {
            bool none_left = false;
            struct object *gone = gear_object_for_use(player, o, o->number, false, &none_left);
            if (gone->known) object_delete(player->cave, NULL, &gone->known);
            object_delete(cave, player->cave, &gone);
        }
        o = next;
    }
    if (strstr(mode, "wear"))
        known_item(TV_HELM, "Hard Leather Cap", 1);
    else {
        known_item(TV_SCROLL, "Teleportation", 5);
        known_item(TV_POTION, "Cure Critical Wounds", 5);
    }
    known_item(TV_FOOD, "Ration of Food", 5);
    /* Stocked for the final fights (borg-trait.c:2928-2947 sets
     * BI_PREP_BIG_FIGHT when fewer than 30 big heals or 15 speed potions
     * are held), so the Borg is not in its restocking state. */
    known_item(TV_POTION, "Healing", 30);
    known_item(TV_POTION, "Speed", 15);
    if (!strstr(mode, "noscrolls"))
        known_item(TV_SCROLL, "Mass Banishment", 5);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;

    /* Sauron was killed before the Borg starts: what the game records when
     * a unique dies (mon-util.c:1101) and what the savefile loader restores
     * for a unique with a kill count (load.c:533-535). */
    struct monster_race *sauron = lookup_monster("Sauron, the Sorcerer");
    if (!sauron) { printf("SETUPFAIL sauron\n"); return 1; }
    if (!prefix(mode, "alive")) {
        get_lore(sauron)->pkills = 1;
        sauron->max_num = 0;
    }

    /* Production start-up: borg_init() builds the Borg's monster tables
     * (borg_init_flow_kill) before any map update has run. */
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_alloc_detection();
    update_view(cave, player);
    borg.c = player->grid; borg.goal.g = player->grid;
    borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.status.active = true;
    borg_find_all_items();
    printf("SETUP seed=101 mode=%s class=%s clevel=%d depth=%d max_depth=%d "
           "sauron_max_num=%d sauron_table_at_init=%d mass_banishment=%d\n",
           mode, player->class->name, player->lev, player->depth,
           player->max_depth, sauron->max_num,
           borg_race_death[borg.id.sauron], strstr(mode, "noscrolls") ? 0 : 5);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    /* First decision pass after start-up, in borg_think()'s order
     * (borg-think.c:211-217). */
    borg_find_all_items();
    borg_notice(true);
    int trait_first_notice = borg.trait[BI_SAURON_DEAD];
    borg_update();
    borg.power = borg_power();
    int32_t baseline = borg.power;
    /* borg_think_dungeon() re-examines the equipment before its decisions
     * (borg-think-dungeon.c:1435); borg.power is not recomputed there. */
    borg_notice(true);
    int32_t consistent = borg_power();
    printf("TRACK first-pass sauron_dead_trait=%d table_after_update=%d "
           "baseline_power=%ld same_gear_power_now=%ld mass_banishment=%d prep_big_fight=%d\n",
           trait_first_notice, borg_race_death[borg.id.sauron],
           (long)baseline, (long)consistent, borg.trait[BI_AMASSBAN], borg.trait[BI_PREP_BIG_FIGHT]);

    /* The production equipment decision (borg_think_dungeon calls it at
     * borg-think-dungeon.c:1572 when nothing more urgent is pending). */
    borg_flush();
    bool acted = strstr(mode, "wear") ? borg_wear_stuff() : borg_drop_junk();
    char keys[40]; int nk = 0;
    keycode_t key;
    while (nk < 39 && (key = borg_inkey(true)) != 0)
        keys[nk++] = (key > 31 && key < 127) ? (char)key : '.';
    keys[nk] = 0;
    borg_flush();
    char worn_before[80];
    my_strcpy(worn_before, borg_items[INVEN_HEAD].desc, sizeof(worn_before));

    /* Carry out the request with the game's own code: a wear request
     * ('w' + letter) through the wield code, a junk request ('{' letter
     * "borg ignore", then 'd' letter) by dropping one of that item. */
    char taken[80] = "none";
    int floor_after = 0;
    if (acted) {
        printf("TRACK keys=<%s>\n", keys);
        require(nk >= 2 && (keys[0] == 'w' || keys[0] == '{'));
        struct object *obj = NULL;
        for (int i = 0; i < z_info->pack_size; i++) {
            struct object *o = player->upkeep->inven[i];
            if (o && all_letters_nohjkl[i] == keys[1]) { obj = o; break; }
        }
        require(obj != NULL);
        object_desc(taken, sizeof(taken), obj, ODESC_PREFIX | ODESC_FULL, player);
        if (keys[0] == 'w') {
            inven_wield(obj, wield_slot(obj));
        } else {
            require(strstr(keys, "borg ignore") != NULL);
            object_desc(taken, sizeof(taken), obj, ODESC_FULL | ODESC_SINGULAR, player);
            inven_drop(obj, 1);
            for (struct object *f = square_object(cave, player->grid); f; f = f->next)
                floor_after += f->number;
        }
        player->upkeep->update |= PU_BONUS | PU_INVEN;
        update_stuff(player);
        borg_find_all_items();
    }
    /* What the same equipment evaluates to once the table is consistent. */
    borg_notice(true);
    int32_t fresh = borg_power();
    printf("DECISION %s=%d item=<%s>\n", strstr(mode, "wear") ? "wear" : "junk",
           acted ? 1 : 0, taken);
    printf("ENGINE helm before=<%s> after=<%s> items_dropped_on_floor=%d\n",
           worn_before, borg_items[INVEN_HEAD].desc, floor_after);
    printf("STATE baseline_power=%ld power_after_decision=%ld sauron_dead_trait=%d\n",
           (long)baseline, (long)fresh, borg.trait[BI_SAURON_DEAD]);
    printf("OUTCOME acts=%d baseline_stale=%d\n", acted ? 1 : 0,
           baseline != consistent ? 1 : 0);
    /* Corrected contract: the first pass compares candidates against the
     * power of the equipment as it really stands: it does not swap the
     * Metal Cap for the weaker spare, and finds nothing to junk. */
    require(baseline == consistent);
    require(!acted);
    ok;
}

const char *suite_name = "native-c/deaduniq-101";
struct test tests[] = {
    {"first_pass_after_borg_start", scenario},
    {NULL, NULL}};
