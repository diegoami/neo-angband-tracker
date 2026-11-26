/* Synthetic savefile generator for diegoami/neo-angband-tracker#644
 * (lightmap-644).  Excluded from the source fix; evidence tooling only.
 *
 * Writes the issue's situations as real savefiles: a dark one-grid-wide
 * dead-end corridor (no glow anywhere, so the torch is the only light),
 * a Human Warrior who walked the corridor to its end and back (so the
 * game's memory of the level covers the corridor's last floor), wielding
 * a wooden torch, with one awake soldier ant:
 *
 *  retreat  the ant holds the corridor's last floor, two grids east of
 *           the player, in the dark outside the torch's real light.
 *           saveplay.c steps the player one grid east (the ant becomes
 *           visible and is noted), then back west: on the next
 *           borg_update() the base commit's light map still marks the
 *           ant's grid lit, the record is judged viewable and is
 *           deleted as one that "did not really move" - the borg
 *           forgets a monster waiting just outside its light.  With the
 *           correction the grid is dark, the record is not viewable,
 *           and it is kept.
 *  stay     control: the ant stands one grid east, inside the real
 *           light, visible in the save itself.
 *
 * Env: LM644S_PROFILE (user dir), LM644S_MODE (retreat|stay),
 *      LM644S_SAVE (savefile name, written into $LM644S_PROFILE/save).
 */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
#include "mon-make.h"
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
#include "savefile.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "z-file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;
#define H 24
#define W 40

/* The level by mode.  retreat/stay: floor on row 12 from x=1 to x=12
 * (dead end).  lure: the corridor turns south at x=12 and ends in a down
 * staircase, so something the borg wants lies beyond the monster. */
#define CORR_Y 12
#define CORR_X0 1
#define CORR_X1 12
#define LURE_STAIR_Y 15
#define LURE_STAIR_X 12

static char savepath[1024];
static const char *mode;
static int ant_x;

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

/* Memorize the level the way drawing it does: map_info() copies the
 * features of every currently-seen grid into the player's known cave,
 * and the savefile keeps them. */
static void memorize_view(void)
{
    struct grid_data g;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            map_info(loc(x, y), &g);
}

int setup_tests(void **state)
{
    const char *profile  = getenv("LM644S_PROFILE");
    const char *savename = getenv("LM644S_SAVE");
    mode = getenv("LM644S_MODE");
    if (!profile || !savename || !mode) return 1;
    bool weak = streq(mode, "lure");
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(644);
    /* The lure save is played by a level 1 Human Druid with a whip: too
     * weak to out-damage a breeding white worm mass.  The other saves
     * keep the level 20 Warrior of the in-memory fixture. */
    if (!player_make_simple("Human", weak ? "Druid" : "Warrior", "LightMap644")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    /* A dark corridor (no SQUARE_GLOW anywhere); lure adds the south leg
     * and the down staircase at its end. */
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            bool floor_grid =
                (y == CORR_Y && x >= CORR_X0 && x <= CORR_X1)
                || (streq(mode, "lure") && x == LURE_STAIR_X
                    && y >= CORR_Y + 1 && y <= LURE_STAIR_Y - 1);
            square_set_feat(cave, loc(x, y), floor_grid ? FEAT_FLOOR : FEAT_GRANITE);
        }
    if (streq(mode, "lure"))
        square_set_feat(cave, loc(LURE_STAIR_X, LURE_STAIR_Y), FEAT_MORE);
    player->cave = cave_new(H, W);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    if (!weak)
        player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 4000;
    inven_wield(torch, wield_slot(torch));
    if (weak) {
        struct object *whip = known_item(TV_HAFTED, "Whip", 1);
        inven_wield(whip, wield_slot(whip));
    }
    known_item(TV_FOOD, "Ration of Food", 5);
    /* Compute the torch's radius once (and the HPs), then walk. */
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    /* The player walked the corridor: stand on every lit step of it (so
     * the whole floor and, in lure mode, the staircase are memorized),
     * then walk back west. */
    {
        /* loc() takes (x, y): the corner is (12, 12), the south stand is
         * (12, 14) - one grid west of the staircase at (12, 15). */
        int walk_lure[][2] = {{12, CORR_Y}, {12, CORR_Y + 2}, {12, CORR_Y}, {10, CORR_Y}};
        int walk_flat[][2] = {{11, CORR_Y}, {10, CORR_Y}};
        int (*walk)[2] = streq(mode, "lure") ? walk_lure : walk_flat;
        size_t nwalk = streq(mode, "lure")
            ? sizeof(walk_lure) / sizeof(walk_lure[0])
            : sizeof(walk_flat) / sizeof(walk_flat[0]);
        for (size_t i = 0; i < nwalk; i++) {
            square_set_mon(cave, player->grid, 0);
            player_place(cave, player, loc(walk[i][0], walk[i][1]));
            player->upkeep->update |= PU_UPDATE_VIEW | PU_MONSTERS;
            update_stuff(player);
            memorize_view();
        }
    }
    player->chp = player->mhp;
    /* The monster, awake: retreat/stay a soldier ant (on the corridor's
     * last floor, or one grid east of the player for the control); lure
     * and worm a white worm mass holding the dark corner / the last
     * floor - slow, random-moving, breeding, poison: the lingering
     * hunter, and a real threat to the level 1 Druid with a whip that
     * plays the lure save. */
    ant_x = streq(mode, "stay") ? 11 : 12;
    const char *race;
    if (streq(mode, "lure") || streq(mode, "worm"))
        race = "White worm mass"; /* slow, random-mover: lingers in the dark */
    else
        race = "Soldier ant";
    struct monster *mon = t_add_monster(cave, loc(ant_x, CORR_Y), race);
    if (!mon) { printf("SETUPFAIL monster\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE);
    update_mon(mon, cave, true);
    if (rf_has(mon->race->flags, RF_NEVER_MOVE)) { printf("SETUPFAIL %s never moves\n", race); return 1; }
    if (mon->m_timed[MON_TMD_SLEEP] != 0) { printf("SETUPFAIL monster asleep\n"); return 1; }
    if (mflag_has(mon->mflag, MFLAG_VISIBLE) != (ant_x == 11)) {
        printf("SETUPFAIL ant visible=%d expected=%d\n",
               mflag_has(mon->mflag, MFLAG_VISIBLE) ? 1 : 0, ant_x == 11 ? 1 : 0);
        return 1;
    }
    path_build(savepath, sizeof(savepath), ANGBAND_DIR_SAVE, savename);
    return 0;
}

static int scenario_save(void *state)
{
    struct monster *mon = square_monster(cave, loc(ant_x, CORR_Y));
    const char *race = mon ? mon->race->name : "?";
    if (!savefile_save(savepath)) { printf("SETUPFAIL save\n"); return 1; }
    printf("SAVED %s mode=%s class=%s clevel=%d hp=%d depth=%d torch=1 corridor=%d..%d,%d player=(%d,%d) monster=%s(%d,%d)\n",
           savepath, mode, player->class->name, player->lev, player->chp, player->depth,
           CORR_X0, CORR_X1, CORR_Y, player->grid.x, player->grid.y,
           race, ant_x, CORR_Y);
    printf("MAP The saved level: @ = the player, %c = the monster\n", mon ? (char)mon->race->d_char : '?');
    for (int y = 0; y < cave->height; ++y) {
        char row[256];
        int x;
        for (x = 0; x < cave->width && x < 255; ++x) {
            struct loc grid = loc(x, y);
            wchar_t c = f_info[square(cave, grid)->feat].d_char;
            struct monster *m = square_monster(cave, grid);
            if (m) c = m->race->d_char;
            if (loc_eq(grid, player->grid)) c = L'@';
            row[x] = (c > 31 && c < 127) ? (char)c : '?';
        }
        row[x] = 0;
        printf("MAPROW %s\n", row);
    }
    printf("MAPEND\n");
    require(file_exists(savepath));
    ok;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/lightmap-644-savegen";
struct test tests[] = {{"save_written", scenario_save}, {NULL, NULL}};
