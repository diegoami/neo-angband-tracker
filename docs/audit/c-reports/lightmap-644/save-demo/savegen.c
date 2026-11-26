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

/* The corridor: floor on row 12 from x=1 to x=12 (dead end), rock
 * elsewhere.  The player walks in from the west. */
#define CORR_Y 12
#define CORR_X0 1
#define CORR_X1 12

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
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(644);
    if (!player_make_simple("Human", "Warrior", "LightMap644")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(H, W);
    /* A dark dead-end corridor: no SQUARE_GLOW anywhere. */
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (y != CORR_Y || x < CORR_X0 || x > CORR_X1)
                square_set_feat(cave, loc(x, y), FEAT_GRANITE);
    player->cave = cave_new(H, W);
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 4000;
    inven_wield(torch, wield_slot(torch));
    known_item(TV_FOOD, "Ration of Food", 5);
    /* The player walked the corridor: stand one grid from the end (the
     * torch lights the last floor), then walk back west two grids. */
    player_place(cave, player, loc(11, CORR_Y));
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    memorize_view();
    square_set_mon(cave, player->grid, 0);
    player_place(cave, player, loc(10, CORR_Y));
    player->upkeep->update |= PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    memorize_view();
    player->chp = player->mhp;
    /* The soldier ant, awake: on the corridor's last floor (retreat) or
     * one grid east of the player (stay control). */
    ant_x = streq(mode, "stay") ? 11 : 12;
    struct monster *mon = t_add_monster(cave, loc(ant_x, CORR_Y), "Soldier ant");
    if (!mon) { printf("SETUPFAIL monster\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE);
    update_mon(mon, cave, true);
    if (mon->m_timed[MON_TMD_SLEEP] != 0) { printf("SETUPFAIL ant asleep\n"); return 1; }
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
    if (!savefile_save(savepath)) { printf("SETUPFAIL save\n"); return 1; }
    printf("SAVED %s mode=%s class=%s clevel=%d hp=%d depth=%d torch=1 corridor=%d..%d,%d player=(%d,%d) ant=(%d,%d)\n",
           savepath, mode, player->class->name, player->lev, player->chp, player->depth,
           CORR_X0, CORR_X1, CORR_Y, player->grid.x, player->grid.y,
           ant_x, CORR_Y);
    printf("MAP The saved level: @ = the player, a = the soldier ant\n");
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
