/* Synthetic native C vein-target eligibility fixture, excluded from the
 * source fix. Issue: diegoami/neo-angband-tracker#280 (veinmode-280).
 *
 * borg_flow_vein() checks whether the borg can dig its minimum vein kind
 * once (lowered to magma after twitching), then enqueues every tracked
 * vein without a per-target check. The reported defect: a borg whose
 * digging covers magma but not quartz commits a flow toward a quartz
 * vein it cannot dig, and borg_play_step() later refuses the dig, so the
 * committed flow makes no progress. The fixture stands such a borg
 * (mid digging skill, no swap digger, no Stone-to-Mud, twitching) with a
 * near quartz vein and a farther magma vein, and records which vein the
 * production borg_flow_vein() commits to. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
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
#include "player-timed.h"
#include "player-util.h"
#include "ui-prefs.h"
#include "ui-term.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-flow-misc.h"
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

static term screen;
#define H 24
#define W 40

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
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
    const char *profile = getenv("V280_PROFILE");
    const char *mode    = getenv("V280_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(280);
    if (!player_make_simple("Human", "Warrior", "Veinmode280")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    /* loc() takes x first: the Borg stands at (x=12, y=10). */
    player_place(cave, player, loc(12, 10));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 14;
    player_exp_gain(player, player_exp[21] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    known_item(TV_FOOD, "Ration of Food", 5);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    /* The capable control carries a swap digger and a strong digging
     * skill, so every vein is diggable on both variants. */
    if (streq(mode, "capable")) {
        struct object *shovel = known_item(TV_DIGGING, "Shovel", 1);
        inven_wield(shovel, wield_slot(shovel));
    }
    /* The reported mismatch: the near vein is quartz (undiggable for the
     * mid-skill borg), the far vein is magma (diggable). */
    square_set_feat(cave, loc(12, 7), FEAT_QUARTZ_K);
    square_set_feat(cave, loc(16, 10), FEAT_MAGMA_K);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    /* Fixture intervention: a mid digging skill for the defect and
     * magma-only modes (magma-capable, quartz not), a strong one for the
     * capable control. */
    player->state.skills[SKILL_DIGGING] = streq(mode, "capable") ? 60 : 14;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true); borg_notice_player();
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    /* The veins, tracked as seen. */
    track_vein.num = 0;
    if (!streq(mode, "magma-only")) {
        track_vein.x[track_vein.num] = 12; track_vein.y[track_vein.num] = 7;
        track_vein.num++;
    }
    track_vein.x[track_vein.num] = 16; track_vein.y[track_vein.num] = 10;
    track_vein.num++;
    borg.c = player->grid;
    borg.avoidance = player->chp;
    borg.time.level = borg.time.now;
    borg.time.town  = borg.time.now;
    /* Fixture intervention: the borg has been twitching, so the vein
     * flow's own gate drops to magma and admits this borg (its digging
     * 13 clears the twitch-relaxed magma threshold of 13 exactly), while
     * the quartz dig it would face still needs 23 and refuses. */
    borg.times_twitch = 27;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=280 mode=%s class=%s clevel=%d dig=%d quartz_diggable=%d "
           "magma_diggable=%d veins=%d twitch=%d gold=%d\n",
           mode, player->class->name, player->lev, borg.trait[BI_DIG],
           borg_can_dig(true, FEAT_QUARTZ_K) ? 1 : 0,
           borg_can_dig(true, FEAT_MAGMA_K) ? 1 : 0,
           track_vein.num, borg.times_twitch, borg.trait[BI_GOLD]);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_vein(void *state)
{
    const char *mode = getenv("V280_MODE");
    keycode_t keys[16];
    int len = 0;
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    uint16_t before = messages_num();
    /* The production vein decision with the dungeon brain's arguments. */
    bool rc = borg_flow_vein(true, 35);
    keycode_t key;
    while ((key = borg_inkey(true)) != 0 && len < 16) keys[len++] = key;
    char keybuf[17];
    for (int i = 0; i < len; ++i) keybuf[i] = (char)keys[i];
    keybuf[len] = 0;
    printf("DECISION rc=%d keys=\"%s\"\n", rc ? 1 : 0, keybuf);
    uint16_t after = messages_num();
    int vein_note = 0;
    for (int i = (int)(after - before) - 1; i >= 0; --i) {
        const char *m = message_str((uint16_t)i);
        printf("TRACK %s\n", m);
        if (prefix(m, "# Flowing toward vein")) vein_note = 1;
    }
    /* The dig the borg would face at the committed target grid. */
    printf("OUTCOME flowed=%d keys=\"%s\" quartz_diggable=%d vein_note=%d\n",
           rc ? 1 : 0, keybuf, borg_can_dig(false, FEAT_QUARTZ_K) ? 1 : 0,
           vein_note);
    if (streq(mode, "mixed")) {
        /* The contract: the committed target must be diggable; the near
         * quartz vein is not, so the flow may not step north toward it —
         * it must take the magma vein to the east instead. */
        require(rc);
        require(vein_note);
        require(streq(keybuf, "6"));
    } else if (streq(mode, "magma-only")) {
        /* Unchanged control: only the diggable magma vein exists. */
        require(rc);
        require(vein_note);
        require(streq(keybuf, "6"));
    } else {
        /* Unchanged control: every vein is diggable, quartz targeted. */
        require(rc);
        require(vein_note);
        require(streq(keybuf, "8"));
    }
    ok;
}

const char *suite_name = "native-c/veinmode-280";
struct test tests[] = {
    {"the_committed_vein_must_be_diggable", scenario_vein},
    {NULL, NULL}};
