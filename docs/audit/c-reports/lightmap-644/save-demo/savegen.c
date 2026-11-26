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
 *  worm     as retreat, but the corridor's last floor is held by a
 *           white worm mass (slow, random-mover: the lingering hunter).
 *  lure     a level 1 Human Druid with a whip, played by saveplay.c
 *           through borg_think().  She came down the up staircase east
 *           of the corridor and walked west into the room; the level
 *           memory covers the corridor, the room and the staircase,
 *           and west of that room a second room continues the level.
 *           A kobold lurks in the dark of the first west room - a
 *           scary guy for a character of her level, and the reason she
 *           is going back east to the stairs - and a second kobold
 *           sleeps at the far end of the second west room, exempt
 *           from the forget by being asleep.  A white worm mass
 *           spawned on the corridor's middle grid, two grids east, in
 *           the dark, and the druid carries no escape kit at all: on
 *           foot, the only options are through the worm or away from
 *           it.  After the dances (glimpse the kobold, retreat,
 *           glimpse the worms, retreat) the base borg forgets the
 *           worm, keeps only the kobold's fear, and walks east toward
 *           the stairs straight past the breeding mass - and in live
 *           play it ping-pongs: east to the reveal, west from the
 *           fear, forget, east again, while the mass breeds; the
 *           corrected borg keeps the record, refuses the east route,
 *           and retreats west without ever going back.
 *  stay     control: the ant stands one grid east, inside the real
 *           light, visible in the save itself.
 *
 * Env: LM644S_PROFILE (user dir), LM644S_MODE
 *      (retreat|worm|lure|stay), LM644S_SAVE (savefile name, written
 *      into $LM644S_PROFILE/save).
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
#include "obj-tval.h"
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

/* The level by mode.  retreat/stay/worm: floor on row 12 from x=1 to
 * x=12 (dead end).  lure: the corridor runs east from the player (x=10)
 * to an up staircase (x=14), and west of the player a small room holds
 * a kobold - so what the borg wants (the way back up, away from the
 * scary guy) lies beyond the monster. */
#define CORR_Y 12
#define CORR_X0 1
#define CORR_X1 12
#define LURE_X0 10
#define LURE_X1 13
#define LURE_STAIR_Y 12
#define LURE_STAIR_X 14
#define LURE_ROOM_X0 5
#define LURE_ROOM_X1 9
#define LURE_ROOM_Y0 11
#define LURE_ROOM_Y1 13
#define LURE_KOBOLD_X 7
#define LURE_KOBOLD_Y 12
/* West of the first room a second room continues the level (a doorway
 * pinches between them), and at its far end a second kobold sleeps:
 * never seen, never woken, it never enters the borg's memory - but the
 * level does not end at the druid's back. */
#define LURE_ROOM2_X0 1
#define LURE_ROOM2_X1 4
#define LURE_SLEEPER_X 2
#define LURE_SLEEPER_Y 12

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
    /* A dark corridor (no SQUARE_GLOW anywhere); the lure corridor is
     * three grids wide (rows 11-13) from the player east to the up
     * staircase, so the far corner holds three worm masses side by
     * side - all revealed at once when the borg steps east, far too
     * much for the whip to be worth fighting.  West of the player a
     * small room opens onto the corridor and a second room continues
     * west past a doorway pinch. */
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            bool floor_grid;
            if (streq(mode, "lure")) {
                floor_grid =
                    (y >= LURE_STAIR_Y - 1 && y <= LURE_STAIR_Y + 1
                     && x >= LURE_X0 && x <= LURE_X1)
                    || (y >= LURE_ROOM_Y0 && y <= LURE_ROOM_Y1
                        && x >= LURE_ROOM_X0 && x <= LURE_ROOM_X1)
                    || (y >= LURE_ROOM_Y0 && y <= LURE_ROOM_Y1
                        && x >= LURE_ROOM2_X0 && x <= LURE_ROOM2_X1);
            } else {
                floor_grid =
                    (y == CORR_Y && x >= CORR_X0 && x <= CORR_X1);
            }
            square_set_feat(cave, loc(x, y), floor_grid ? FEAT_FLOOR : FEAT_GRANITE);
        }
    if (streq(mode, "lure")) {
        /* The doorway pinch between the two west rooms. */
        square_set_feat(cave, loc(4, LURE_ROOM_Y0), FEAT_GRANITE);
        square_set_feat(cave, loc(5, LURE_ROOM_Y0), FEAT_GRANITE);
        square_set_feat(cave, loc(4, LURE_ROOM_Y1), FEAT_GRANITE);
        square_set_feat(cave, loc(5, LURE_ROOM_Y1), FEAT_GRANITE);
        square_set_feat(cave, loc(LURE_STAIR_X, LURE_STAIR_Y), FEAT_LESS);
    }
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
    if (weak) {
        /* The druid carries no spellbook: nothing to study, nothing to
         * cast, so the borg's attention goes straight to what its level
         * memory offers - the way back up, east of the corridor. */
        struct object *books[16];
        int nbooks = 0;
        for (struct object *obj = player->gear; obj; obj = obj->next) {
            if (tval_is_book_k(obj->kind) && nbooks < 16)
                books[nbooks++] = obj;
        }
        for (int i = 0; i < nbooks; i++) {
            bool none_left = false;
            struct object *book = gear_object_for_use(
                player, books[i], books[i]->number, false, &none_left);
            object_delete(NULL, NULL, &book);
        }
        /* ... and no escape kit either - no Word of Recall, no Phase
         * Door: whatever happens, happens on foot.  With nothing to
         * read, the escape ladder fails and the borg must back away,
         * which is where the two builds part ways: the original forgets
         * the worm once it is two grids away in the dark and walks back
         * east into it, over and over, while the mass breeds; the fix
         * keeps the record and keeps its distance. */
        {
            struct object *scrolls[8];
            int nscrolls = 0;
            for (struct object *obj = player->gear; obj; obj = obj->next) {
                struct object_kind *wor =
                    lookup_kind(TV_SCROLL, lookup_sval(TV_SCROLL, "Word of Recall"));
                if (wor && obj->kind == wor && nscrolls < 8)
                    scrolls[nscrolls++] = obj;
            }
            for (int i = 0; i < nscrolls; i++) {
                bool none_left = false;
                struct object *scroll = gear_object_for_use(
                    player, scrolls[i], scrolls[i]->number, false, &none_left);
                object_delete(NULL, NULL, &scroll);
            }
        }
    }
    /* Compute the torch's radius once (and the HPs), then walk. */
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN | PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    /* The player walked the corridor: stand on every lit step of it (so
     * the whole floor and, in lure mode, the staircase are memorized),
     * then walk back west. */
    {
        /* loc() takes (x, y).  The player came down the up staircase in
         * the east (14, 12), walked the corridor west into the room to
         * its west edge, and turned back east to (10, 12): every stand
         * memorizes its lit ring, so the corridor, the room and the
         * staircase are all in the level memory.  The kobold and the
         * worm mass are placed only after this walk - in the meantime,
         * as it were. */
        int walk_lure[][2] = {
            {14, CORR_Y}, {13, CORR_Y}, {12, CORR_Y}, {11, CORR_Y},
            {10, CORR_Y}, {9, CORR_Y}, {8, CORR_Y}, {7, CORR_Y},
            {6, CORR_Y}, {7, CORR_Y}, {8, CORR_Y}, {9, CORR_Y},
            {10, CORR_Y}
        };
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
    /* The monsters, awake: retreat/stay a soldier ant (on the corridor's
     * last floor, or one grid east of the player for the control); lure
     * and worm white worm masses - slow, random-moving, breeding,
     * poison: the lingering hunters, spawned after the player walked
     * past, and a real threat to the level 1 Druid with a whip that
     * plays the lure save.  The lure holds TWO of them side by side at
     * the dark corner (12,12) and (12,11), both two grids from the
     * player, both forgotten together on the base commit and kept
     * together on the fix. */
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
    if (streq(mode, "lure")) {
        /* The second and third worm masses beside the first: three
         * abreast at the dark corner (12,11), (12,12), (12,13) - all
         * two grids east of the player, all in the same forget ring. */
        for (int dy = -1; dy <= 1; dy++) {
            struct monster *worm2 =
                t_add_monster(cave, loc(12, CORR_Y + dy), race);
            if (!worm2) { printf("SETUPFAIL worm2\n"); return 1; }
            mflag_on(worm2->mflag, MFLAG_AWARE);
            update_mon(worm2, cave, true);
            if (worm2->m_timed[MON_TMD_SLEEP] != 0) { printf("SETUPFAIL worm2 asleep\n"); return 1; }
            if (mflag_has(worm2->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL worm2 visible\n"); return 1; }
        }
    }
    if (streq(mode, "lure")) {
        /* The kobold in the west room: awake, mobile, three grids west
         * of the player and dark.  It is in the borg's scary-guy list
         * (BI_CLEVEL <= 8), so once the borg has seen it the borg
         * flees the level - back east, to the up staircase. */
        struct monster *kob =
            t_add_monster(cave, loc(LURE_KOBOLD_X, LURE_KOBOLD_Y), "Kobold");
        if (!kob) { printf("SETUPFAIL kobold\n"); return 1; }
        mflag_on(kob->mflag, MFLAG_AWARE);
        update_mon(kob, cave, true);
        if (rf_has(kob->race->flags, RF_NEVER_MOVE)) { printf("SETUPFAIL kobold never moves\n"); return 1; }
        if (kob->m_timed[MON_TMD_SLEEP] != 0) { printf("SETUPFAIL kobold asleep\n"); return 1; }
        if (mflag_has(kob->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL kobold visible\n"); return 1; }
        /* And at the far west end of the second room a second kobold,
         * sleeping: asleep and unaware, so it is exempt from the
         * missing-monster pass ("Don't 'forget' certain ones") and
         * prices as no danger at all until something wakes it. */
        struct monster *sleeper =
            t_add_monster(cave, loc(LURE_SLEEPER_X, LURE_SLEEPER_Y), "Kobold");
        if (!sleeper) { printf("SETUPFAIL sleeper\n"); return 1; }
        sleeper->m_timed[MON_TMD_SLEEP] = 1000;
        update_mon(sleeper, cave, true);
        if (sleeper->m_timed[MON_TMD_SLEEP] == 0) { printf("SETUPFAIL sleeper awake\n"); return 1; }
        if (mflag_has(sleeper->mflag, MFLAG_AWARE)) { printf("SETUPFAIL sleeper aware\n"); return 1; }
        if (mflag_has(sleeper->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL sleeper visible\n"); return 1; }
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
