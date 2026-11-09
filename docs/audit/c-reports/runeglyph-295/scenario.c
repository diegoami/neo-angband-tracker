/* Synthetic native C rune-recognition fixture, excluded from the source fix.
 * Issue: diegoami/neo-angband-tracker#295 (runeglyph-295).
 *
 * The player reads a Rune of Protection through the game's own read command
 * (rune mode) and then hands over to the Borg.  The Borg's own map update
 * (borg_update) runs, then the production defence choice (borg_defend) is
 * asked what to do about an awake summoner in view.  If it reads a Rune of
 * Protection, the read is executed through the game's command. */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "cmds.h"
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
#include "obj-tval.h"
#include "obj-util.h"
#include "player.h"
#include "player-birth.h"
#include "player-calcs.h"
#include "player-util.h"
#include "trap.h"
#include "ui-menu.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-fight-defend.h"
#include "borg/borg-flow-glyph.h"
#include "borg/borg-flow-kill.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-io.h"
#include "borg/borg-item.h"
#include "borg/borg-trait.h"
#include "borg/borg-update.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40
#define PX 12
#define PY 12
#define LEV 30
#define DEPTH 30
#define SCROLLS 5

static const char *mode;
static struct monster *mon;

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

static struct object *rune_scrolls(void)
{
    int sval = lookup_sval(TV_SCROLL, "Rune of Protection");
    for (struct object *o = player->gear; o; o = o->next)
        if (o->tval == TV_SCROLL && o->sval == sval) return o;
    return NULL;
}

static int scroll_count(void)
{
    struct object *o = rune_scrolls();
    return o ? o->number : 0;
}

/* Runes of Protection in the game's trap list of a grid. */
static int runes_at(struct loc g)
{
    int n = 0;
    for (struct trap *t = square_trap(cave, g); t; t = t->next)
        if (trf_has(t->flags, TRF_GLYPH)) ++n;
    return n;
}

/* The game's own read command on the carried Rune of Protection scrolls. */
static void read_rune(const char *tag)
{
    struct object *obj = rune_scrolls();
    uint16_t before = messages_num();
    struct command cmd;
    memset(&cmd, 0, sizeof cmd);
    cmd.code = CMD_READ_SCROLL;
    cmd_set_arg_item(&cmd, "item", obj);
    do_cmd_read_scroll(&cmd);
    update_stuff(player);
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK %s game-message <%s>\n", tag, message_str((uint16_t)i));
}

int setup_tests(void **state)
{
    const char *profile = getenv("RG295_PROFILE");
    mode = getenv("RG295_MODE");
    if (!profile || !mode) return 1;
    if (!streq(mode, "rune") && !streq(mode, "floor") && !streq(mode, "trap")) {
        printf("SETUPFAIL mode\n"); return 1;
    }
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(295);
    if (!player_make_simple("Human", "Warrior", "RuneGlyph295")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(PX, PY));
    player->depth = player->max_depth = cave->depth = player->cave->depth = DEPTH;
    player_exp_gain(player, player_exp[LEV - 2] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < H - 1; ++y) for (int x = 1; x < W - 1; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    struct object *lantern = known_item(TV_LIGHT, "Lantern", 1);
    lantern->timeout = 2000;
    inven_wield(lantern, wield_slot(lantern));
    known_item(TV_SCROLL, "Rune of Protection", SCROLLS);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    wiz_light(cave, player, true);
    update_view(cave, player);

    /* The summoner, placed before any trap so the monster roll is the same
     * in every case. */
    const char *race = getenv("RG295_RACE");
    if (!race) race = "ranger";
    mon = t_add_monster(cave, loc(PX + 4, PY), race);
    if (!mon) { printf("SETUPFAIL mon\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE); update_mon(mon, cave, true);
    if (!monster_is_visible(mon)) { printf("SETUPFAIL mon invisible\n"); return 1; }

    /* rune: the player reads a Rune of Protection on the current grid with
     * the game's own command, before handing over to the Borg.  trap: a
     * revealed pit (a disarmable player trap) next to the player. */
    if (streq(mode, "rune")) read_rune("player");
    if (streq(mode, "trap")) {
        place_trap(cave, loc(PX - 1, PY), lookup_trap("pit")->tidx, DEPTH);
        square_reveal_trap(cave, loc(PX - 1, PY), true, false);
    }
    update_view(cave, player);

    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg.c = player->grid; borg.goal.g = player->grid;
    borg.time.level = borg.time.now;
    borg.status.active = true;
    /* The Borg's own map update reads the screen and the level. */
    for (int pass = 0; pass < 3; ++pass) borg_update();
    borg_notice(true);
    borg.avoidance = borg.trait[BI_CURHP];
    printf("SETUP seed=295 mode=%s class=%s clevel=%d depth=%d hp=%d monster=%s at=(%d,%d) scrolls=%d\n",
           mode, player->class->name, player->lev, player->depth, borg.trait[BI_CURHP],
           mon->race->name, mon->grid.x, mon->grid.y, scroll_count());
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario(void *state)
{
    keycode_t keys[32];
    int len = 0;
    keycode_t key;
    struct loc here = player->grid;
    borg_grid *ag = &borg_grids[here.y][here.x];
    borg_grid *west = &borg_grids[here.y][here.x - 1];
    int k = borg_grids[mon->grid.y][mon->grid.x].kill;

    printf("STATE game_runes_here=%d borg_glyph_here=%d borg_trap_here=%d tracked_glyphs=%d "
           "borg_trap_west=%d game_trap_west=%d monster_tracked=%d\n",
           runes_at(here), ag->glyph ? 1 : 0, ag->trap ? 1 : 0, track_glyph.num,
           west->trap ? 1 : 0, square_isdisarmabletrap(cave, loc(here.x - 1, here.y)) ? 1 : 0,
           k > 0 ? 1 : 0);

    int p1 = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    borg_on_glyph = true;
    int p2 = borg_danger(borg.c.y, borg.c.x, 1, true, false);
    borg_on_glyph = false;
    printf("DANGER here=%d on_rune=%d avoidance=%d\n", p1, p2, borg.avoidance);

    /* The production defence choice, called with the danger at the Borg's
     * grid as borg_think's caution path does. */
    borg_flush();
    uint16_t before = messages_num();
    bool rc = borg_defend(p1);
    while ((key = borg_inkey(true)) != 0 && len < 32) keys[len++] = key;
    printf("DECISION defend=%d keys=%d sequence=", rc ? 1 : 0, len);
    for (int i = 0; i < len; ++i) {
        if (keys[i] == ESCAPE) printf("<Escape>"); else printf("%c", (char)keys[i]);
    }
    putchar('\n');
    uint16_t after = messages_num();
    for (int i = (int)(after - before) - 1; i >= 0; --i)
        printf("TRACK borg-note <%s>\n", message_str((uint16_t)i));

    /* Decode: 'r' then the inventory letter of the Rune of Protection. */
    struct object *runes = rune_scrolls();
    int read = 0;
    for (int i = 0; i + 1 < len; ++i) {
        if (keys[i] != 'r' || !runes) continue;
        int slot = -1;
        for (int s = 0; s < z_info->pack_size; ++s)
            if (player->upkeep->inven[s] == runes) slot = s;
        if (slot >= 0 && keys[i + 1] == all_letters_nohjkl[slot]) read = 1;
    }
    int runes_before = runes_at(here);
    int scrolls_before = scroll_count();
    if (read) read_rune("borg");
    printf("OUTCOME read_rune=%d scrolls=%d->%d game_runes_here=%d->%d borg_glyph_here=%d\n",
           read, scrolls_before, scroll_count(), runes_before, runes_at(here),
           ag->glyph ? 1 : 0);

    if (streq(mode, "rune")) {
        /* Corrected contract: the visible rune is on the Borg's map and it
         * does not read another one on top of it. */
        require(ag->glyph);
        require(!read);
    } else if (streq(mode, "trap")) {
        require(west->trap);
        require(read);
    } else {
        require(read);
    }
    ok;
}

const char *suite_name = "native-c/runeglyph-295";
struct test tests[] = {
    {"rune_recognition_defence", scenario},
    {NULL, NULL}};
