/* Synthetic native C light-maintenance fixture for issue #657 (lantern-657).
 * Excluded from the source fix.  When the light slot is empty,
 * borg_maintain_light() took any matching lantern from borg_slot() before
 * looking for a torch, with no fuel check.  With a spent lantern (timeout 0)
 * and a burning wooden torch in the pack and no oil, the borg wielded the
 * spent lantern - a fuel-dependent light with no fuel produces no light -
 * and from then on hit the equipped-lantern refuel branch, which has no fuel
 * to offer, so the borg stays dark while a working torch sits in its pack.
 *
 * The level: an open, lit arena floor, no monsters, a Human Warrior.  The
 * light slot is empty in every mode; the pack contents differ per mode.
 *
 * Modes:
 *  torchpref       the reported defect: spent lantern + burning torch,
 *                  no oil.  The borg must wield the torch (which burns).
 *  no_false_need   only a spent lantern, no fuel of any kind: the borg
 *                  must report the need as unmet, not wield a dark lantern
 *                  and claim success.
 *  fueledlantern   control: fueled lantern + torch - the lantern wins,
 *                  byte-identical before/after.
 *  spentlantern_oil control: spent lantern + flask of oil - wielding the
 *                  lantern is correct because oil can light it (the
 *                  wear-then-refuel sequence), byte-identical before/after.
 *  torchonly       control: only a burning torch, byte-identical
 *                  before/after.
 *
 * The maintenance decision is observed from the keys it queues; the chosen
 * item is then wielded through the production gear swap so BI_LIGHT (the
 * borg's fuel-aware light radius, which credits a fuel-dependent light only
 * when its timeout is non-zero) shows whether the action actually restores
 * light.
 */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
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
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-light.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-think.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void evidence_map(const char *caption)
{
    printf("MAP %s\n", caption);
    for (int y = 0; y < cave->height; ++y) {
        char row[256];
        int x;
        for (x = 0; x < cave->width && x < 255; ++x) {
            struct loc grid = loc(x, y);
            wchar_t c = f_info[square(cave, grid)->feat].d_char;
            if (loc_eq(grid, player->grid))
                c = L'@';
            row[x] = (c > 31 && c < 127) ? (char)c : '?';
        }
        row[x] = 0;
        printf("MAPROW %s\n", row);
    }
    printf("MAPEND\n");
}

static term screen;
#define H 24
#define W 40

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

static const char *mode;
static int mode_valid(void)
{
    return streq(mode, "torchpref") || streq(mode, "no_false_need")
        || streq(mode, "fueledlantern") || streq(mode, "spentlantern_oil")
        || streq(mode, "torchonly");
}

int setup_tests(void **state)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *profile = getenv("LTN657_PROFILE");
    mode = getenv("LTN657_MODE");
    if (!profile || !mode || !mode_valid()) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(657);
    if (!player_make_simple("Human", "Warrior", "Lantern657")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* An open, lit arena floor (SQUARE_GLOW everywhere): the light slot
     * is the only thing under test. */
    cave = t_build_arena(H, W);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(20, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;

    /* The recorded situation starts with an EMPTY light slot: drop the
     * Warrior's starting torch on the floor (it must not sit in the
     * pack or on the body). */
    /* The recorded situation starts with an EMPTY light slot and no
     * torch in the pack: drop every wooden torch the birth kit
     * provides (it wields one and carries a spare). */
    for (;;) {
        struct object *t = NULL;
        for (struct object *obj = player->gear; obj; obj = obj->next)
            if (obj->tval == TV_LIGHT
                && obj->kind == lookup_kind(TV_LIGHT, lookup_sval(TV_LIGHT, "Wooden Torch"))
                && obj->number > 0) { t = obj; break; }
        if (!t) break;
        if (object_is_equipped(player->body, t))
            inven_takeoff(t);
        else
            inven_drop(t, t->number);
    }

    /* The pack contents per mode.  The light slot stays empty: that is
     * the recorded situation.  A torch in the pack always burns - the
     * engine destroys torches that reach zero turns. */
    struct object *torch = NULL, *lantern = NULL, *oil = NULL;
    if (streq(mode, "torchpref") || streq(mode, "fueledlantern")
        || streq(mode, "torchonly")) {
        torch = known_item(TV_LIGHT, "Wooden Torch", 1);
        torch->timeout = 4000;
    }
    if (!streq(mode, "torchonly")) {
        lantern = known_item(TV_LIGHT, "Lantern", 1);
        lantern->timeout = streq(mode, "fueledlantern") ? 3000 : 0; /* fueled | spent */
    }
    if (streq(mode, "spentlantern_oil"))
        oil = known_item(TV_FLASK, "Flask of Oil", 1);
    known_item(TV_FOOD, "Ration of Food", 5);

    player->upkeep->update |= PU_BONUS | PU_HP | PU_MANA | PU_TORCH | PU_INVEN | PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    player->chp = player->mhp;
    borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg.c = player->grid;
    borg.status.active = true;
    borg.ready_morgoth = 0;
    borg.time.level = borg.time.town = borg.time.now;
    borg_do_spell = true;
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    borg.avoidance = player->chp;
    /* The borg arrives on the level: the first production update takes
     * the new-level reset, which later state depends on. */
    borg_update();
    if (borg.status.old_depth != borg.trait[BI_CDEPTH]) { printf("SETUPFAIL arrival old_depth=%d depth=%d\n", borg.status.old_depth, borg.trait[BI_CDEPTH]); return 1; }
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY;
    }
    track_less.num = track_more.num = 0;
    borg_notice(true);
    printf("SETUP seed=657 mode=%s class=%s clevel=%d hp=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp, player->depth);
    for (int i = 0; i < z_info->pack_size; ++i) {
        borg_item *it = &borg_items[i];
        if (!it->iqty) continue;
        printf("PACK slot=%d letter=%c name=%s tval=%d sval=%d timeout=%d\n",
               i, all_letters_nohjkl[i], it->desc, it->tval, it->sval, it->timeout);
    }
    printf("PACK light_slot name=%s timeout=%d\n",
           borg_items[INVEN_LIGHT].desc, borg_items[INVEN_LIGHT].timeout);
    return 0;
}

static int drain_keys(keycode_t *keys, int max)
{
    int len = 0; keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < max) keys[len++] = key;
    return len;
}

static void print_keys(const char *tag, keycode_t *keys, int len)
{
    printf("%s keys=%d sequence=", tag, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Escape>");
        else if (keys[i] == '\n' || keys[i] == '\r') printf("<Enter>");
        else printf("%c", (char)keys[i]);
    }
    putchar('\n');
}

/* The pack slot whose selection letter the maintenance queued. */
static int slot_for_letter(char letter)
{
    for (int i = 0; i < z_info->pack_size; ++i)
        if (all_letters_nohjkl[i] == letter)
            return i;
    return -1;
}

static int scenario_light_maintenance(void *state)
{
    keycode_t keys[64];
    int spent = streq(mode, "torchpref") || streq(mode, "no_false_need")
        || streq(mode, "spentlantern_oil");

    /* The maintenance decision. */
    enum borg_need need = borg_maintain_light();
    int len = drain_keys(keys, 64);
    printf("MAINTAIN need=%d keys=%d sequence=", (int)need, len);
    for (int i = 0; i < len; ++i) printf("%c", (char)keys[i]);
    putchar('\n');

    /* Decode and perform the wield it queued, through the same gear
     * swap the game's wear command uses. */
    int worn_slot = -1, worn_torch = 0, worn_lantern = 0;
    if (len >= 2 && keys[0] == 'w') {
        worn_slot = slot_for_letter((char)keys[1]);
        if (worn_slot < 0) { printf("SETUPFAIL wear_letter\n"); return 1; }
        struct object *obj = player->upkeep->inven[worn_slot];
        printf("WEAR slot=%d name=%s timeout=%d\n",
               worn_slot, obj->kind->name, obj->timeout);
        printf("DECISION MAINTAIN need=%d keys=%d sequence=", (int)need, len);
        for (int i = 0; i < len; ++i) printf("%c", (char)keys[i]);
        putchar('\n');
        printf("DECISION WEAR slot=%d name=%s timeout=%d\n",
               worn_slot, obj->kind->name, obj->timeout);
        if (strstr(obj->kind->name, "Wooden Torch")) worn_torch = 1;
        if (strstr(obj->kind->name, "Lantern")) worn_lantern = 1;
        inven_wield(obj, wield_slot(obj));
        player->upkeep->update |= PU_BONUS | PU_TORCH | PU_INVEN;
        update_stuff(player);
        borg_equipment(); borg_inventory(); borg_notice(true);
    } else {
        printf("DECISION MAINTAIN need=%d keys=%d sequence=none\n", (int)need, len);
    }
    /* The borg's fuel-aware light radius after the action: a
     * fuel-dependent light with zero fuel credits nothing. */
    printf("LIGHT bi_light=%d\n", borg.trait[BI_LIGHT]);
    if (!streq(mode, "spentlantern_oil") && need == BORG_MET_NEED) {
        require(borg.trait[BI_LIGHT] >= 1);
        require(worn_slot >= 0);
    }
    if (spent && !streq(mode, "no_false_need")) {
        /* The wielded item: the lantern for spentlantern_oil (oil can
         * light it), never the lantern for torchpref. */
        struct object *worn = equipped_item_by_slot_name(player, "light");
        require(worn != NULL);
        printf("WORN name=%s timeout=%d\n", worn->kind->name, worn->timeout);
        if (streq(mode, "torchpref"))
            require(strstr(worn->kind->name, "Wooden Torch") != NULL);
        else
            require(strstr(worn->kind->name, "Lantern") != NULL);
    }
    if (streq(mode, "no_false_need"))
        /* Nothing can restore light here; the borg must not claim the
         * need is met. */
        require((int)need == (int)BORG_UNMET_NEED && len == 0);

    /* The wear-then-refuel sequence for the oil control: the next
     * maintenance call refuels the worn spent lantern. */
    if (streq(mode, "spentlantern_oil")) {
        enum borg_need need2 = borg_maintain_light();
        keycode_t keys2[64];
        int len2 = drain_keys(keys2, 64);
        printf("REFUEL need=%d keys=%d sequence=", (int)need2, len2);
        for (int i = 0; i < len2; ++i) printf("%c", (char)keys2[i]);
        putchar('\n');
        printf("DECISION REFUEL need=%d keys=%d sequence=", (int)need2, len2);
        for (int i = 0; i < len2; ++i) printf("%c", (char)keys2[i]);
        putchar('\n');
        require(len2 >= 1 && keys2[0] == 'F');
    }

    printf("STATE need=%d bi_light=%d worn_torch=%d worn_lantern=%d\n",
           (int)need, borg.trait[BI_LIGHT], worn_torch, worn_lantern);
    printf("OUTCOME need=%d bi_light=%d\n", (int)need, borg.trait[BI_LIGHT]);
    evidence_map("After the light-maintenance decision: @ = the Borg");
    ok;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/lantern-657";
struct test tests[] = {
    {"light_maintenance_wields_a_producing_light", scenario_light_maintenance},
    {NULL, NULL}};
