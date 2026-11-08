/* Synthetic native C Druid fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#516 (regensim-516). */
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
#include "obj-tval.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-spell.h"
#include "player-timed.h"
#include "player-util.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-think.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-misc.h"
#include "borg/borg-fight-perm.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-magic.h"
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
    const char *profile = getenv("RS516_PROFILE");
    const char *mode    = getenv("RS516_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(516);
    if (!player_make_simple("Human", "Druid", "RegenSim516")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(10, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 20;
    player_exp_gain(player, player_exp[48] * player->expfact / 100);
    /* Fixture intervention: a Druid with the wisdom to pay for the spells. */
    for (int s = 0; s < STAT_MAX; s++) { player->stat_max[s] = 18 + 220; player->stat_cur[s] = 18 + 220; }
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 2000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    for (int b = 0; b < player->class->magic.num_books; b++) {
        struct class_book *book = &player->class->magic.books[b];
        struct object_kind *k = lookup_kind(book->tval, book->sval);
        if (!k) continue;
        struct object *obj = object_new();
        object_prep(obj, k, 10, MAXIMISE);
        obj->number = 1;
        obj->known = object_new();
        object_set_base_known(player, obj);
        object_flavor_aware(player, obj);
        inven_carry(player, obj, false, false);
        object_touch(player, obj);
        player_know_object(player, obj);
    }
    /* The Druid has learned and cast Haste Self and Rapid Regeneration before. */
    for (int i = 0; i < player->class->magic.total_spells; i++) {
        const struct class_spell *s = spell_by_index(player, i);
        if (s && (streq(s->name, "Haste Self") || streq(s->name, "Rapid Regeneration")))
            player->spell_flags[i] |= PY_SPELL_LEARNED | PY_SPELL_WORKED;
    }
    if (streq(mode, "regen_active")) player->timed[TMD_HEAL] = 50;
    if (streq(mode, "haste_active")) player->timed[TMD_FAST] = 50;
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_MANA | PU_SPELLS;
    update_stuff(player);
    player->chp = player->mhp;
    player->csp = player->msp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory();
    borg_do_spell = true;
    borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    update_view(cave, player);
    track_less.num = track_more.num = 0;
    borg.c = player->grid; borg.avoidance = player->chp; borg.ready_morgoth = 0;
    borg.time.level = borg.time.now;
    borg.time.town = borg.time.now;
    borg.status.active = true;
    borg_update_entrypoint(true);
    printf("SETUP seed=516 mode=%s class=%s clevel=%d hp=%d/%d sp=%d/%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp, player->mhp,
           player->csp, player->msp, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_perma(void *state)
{
    uint16_t before = messages_num();
    borg_flush();
    memset(&borg.goal, 0, sizeof(borg.goal));
    borg_notice(true);
    borg.no_rest_prep = 0;
    printf("STATE clevel=%d cursp=%d maxsp=%d cdepth=%d haste_okay=%d haste_fail=%d regen_okay=%d regen_fail=%d haste_on=%d regen_on=%d\n",
           borg.trait[BI_CLEVEL], borg.trait[BI_CURSP], borg.trait[BI_MAXSP], borg.trait[BI_CDEPTH],
           borg_spell_okay(HASTE_SELF) ? 1 : 0, borg_spell_fail_rate(HASTE_SELF),
           borg_spell_okay(RAPID_REGENERATION) ? 1 : 0, borg_spell_fail_rate(RAPID_REGENERATION),
           borg.temp.fast ? 1 : 0, borg.temp.regen ? 1 : 0);
    bool acted = borg_perma_spell();
    /* A cast is three queued keys: m, the book letter and the spell letter. */
    int keys = 0;
    char seq[256] = "";
    keycode_t k;
    while ((k = borg_inkey(true)) != 0) {
        size_t n = strlen(seq);
        if (n < sizeof(seq) - 3) snprintf(seq + n, 3, "%s%c", keys ? " " : "", (char)k);
        keys++;
    }
    printf("DECISION acted=%d keys=%d key_sequence=\"%s\"\n", acted ? 1 : 0, keys, seq);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    printf("OUTCOME casts_queued=%d\n", keys / 3);
    require(acted);
    require(keys == 3); /* fails before in the two scoring modes: Rapid Regeneration is queued while only scoring */
    ok;
}

const char *suite_name = "native-c/regensim-516";
struct test tests[] = {
    {"perma_spell_queue", scenario_perma},
    {NULL, NULL}};
