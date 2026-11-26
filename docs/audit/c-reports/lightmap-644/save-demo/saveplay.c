/* Synthetic savefile player for diegoami/neo-angband-tracker#644
 * (lightmap-644).  Excluded from the source fix; evidence tooling only.
 *
 * Loads the savefile written by savegen.c through the production loader
 * (savefile_load + on_new_level + the view/monster updates), activates
 * the borg, and runs its real per-turn observation passes (borg_update():
 * the map scan through map_info, the view, the light map and the
 * missing-monster pass).
 *
 * Modes (the save carries the matching monster position):
 *  retreat  the ant holds the corridor's last floor, two grids east, in
 *           the dark.  The borg steps one grid east (the production
 *           view/light updates make the ant visible, and the scan notes
 *           its record), then steps back west; one turn passes.  On the
 *           base commit the borg's light map still marks the ant's grid
 *           lit from two grids away, the record is judged viewable, and
 *           the missing-monster pass deletes it as one that "did not
 *           really move" - the borg forgets a monster it just saw.  With
 *           the correction the grid is dark, the record is not viewable,
 *           and it is kept.
 *  worm     as retreat, with a white worm mass (the lingering hunter).
 *  lure     the level 1 Druid: after the same dance, borg_think() weighs
 *           the up staircase east - the way the save's walk says he came
 *           from, and the only thing on his map worth walking to -
 *           against the danger he knows about on the way.
 *  stay     control: the ant stands one grid east, inside the real
 *           light; the scan re-observes it every pass and both variants
 *           keep the record.
 *
 * Env: LM644S_PROFILE (user dir with borg.txt), LM644S_MODE
 *      (retreat|worm|lure|stay), LM644S_SAVE (savefile name inside
 *      $LM644S_PROFILE/save).
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
#include "borg/borg-caution.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-light.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-danger.h"
#include "borg/borg-flow.h"
#include "borg/borg-flow-kill.h"
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

#define CORR_Y 12

static term screen;
static char savepath[1024];
static const char *mode;

/* Move the player with the production updates, so the game's view, light
 * and monster visibility follow the move. */
static void step_player(int x)
{
    square_set_mon(cave, player->grid, 0);
    player_place(cave, player, loc(x, CORR_Y));
    player->upkeep->update |= PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
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
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    path_build(savepath, sizeof(savepath), ANGBAND_DIR_SAVE, savename);
    if (!savefile_load(savepath, false)) { printf("SETUPFAIL load\n"); return 1; }
    require(character_dungeon);
    /* Production post-load setup (the game's own sequence after loading). */
    on_new_level();
    player->upkeep->update |= PU_UPDATE_VIEW | PU_MONSTERS;
    update_stuff(player);
    /* Sanity: the loaded state. */
    if (!loc_eq(player->grid, loc(10, CORR_Y))) { printf("SETUPFAIL player=(%d,%d)\n", player->grid.x, player->grid.y); return 1; }
    if (player->depth != 5) { printf("SETUPFAIL depth=%d\n", player->depth); return 1; }
    printf("LOADED mode=%s save=%s class=%s clevel=%d hp=%d/%d depth=%d player=(%d,%d)\n",
           mode, savepath, player->class->name, player->lev, player->chp, player->mhp,
           player->depth, player->grid.x, player->grid.y);
    for (int i = 1; i < cave_monster_max(cave); i++) {
        struct monster *mon = cave_monster(cave, i);
        if (!mon->race) continue;
        printf("MON i=%d race=%s pos=(%d,%d) visible=%d aware=%d\n", i,
               mon->race->name, mon->grid.x, mon->grid.y,
               mflag_has(mon->mflag, MFLAG_VISIBLE) ? 1 : 0,
               mflag_has(mon->mflag, MFLAG_AWARE) ? 1 : 0);
    }
    /* The borg's real activation path: borg_init reads borg.txt from the
     * user dir, and the first borg_update() is the arrival pass (new
     * level reset, then the map scan through map_info, the view, the
     * light map, and the monster tracking). */
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg.c = player->grid;
    borg.status.active = true;
    borg.ready_morgoth = 0;
    borg.time.level = borg.time.town = borg.time.now;
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    borg.avoidance = player->chp;
    printf("PREPARE infra=%d light=%d\n", borg.trait[BI_INFRA], borg.trait[BI_LIGHT]);
    borg_update();
    borg_update_entrypoint(true);
    return 0;
}

static void print_kills(void)
{
    int kills = 0;
    for (int k = 1; k < borg_kills_nxt; ++k) {
        borg_kill *kill = &borg_kills[k];
        if (!kill->r_idx) continue;
        kills++;
        printf("KILL k=%d race=%s pos=(%d,%d) awake=%d\n", k,
               r_info[kill->r_idx].name, kill->pos.x, kill->pos.y, kill->awake);
    }
    printf("KILLS kills=%d\n", kills);
}

static void light_info(int y, int x)
{
    borg_grid *ag = &borg_grids[y][x];
    printf("LIGHTINFO grid=(%d,%d) light=%d glow=%d view=%d dark=%d\n", x, y,
           (ag->info & BORG_LIGHT) ? 1 : 0, (ag->info & BORG_GLOW) ? 1 : 0,
           (ag->info & BORG_VIEW) ? 1 : 0, (ag->info & BORG_DARK) ? 1 : 0);
}

static int scenario_light(void *state)
{
    struct monster *mon = square_monster(cave, streq(mode, "stay") ? loc(11, CORR_Y) : loc(12, CORR_Y));
    if (!mon) { printf("SETUPFAIL no monster\n"); return 1; }
    bool lure = streq(mode, "lure");

    if (streq(mode, "retreat") || lure || streq(mode, "worm")) {
        /* The borg steps one grid east: the torch lights the monster's
         * grid, the game sees it, and the scan notes the record. */
        step_player(11);
        printf("STEP1 player=(%d,%d) monster=(%d,%d) visible=%d\n",
               player->grid.x, player->grid.y, mon->grid.x, mon->grid.y,
               mflag_has(mon->mflag, MFLAG_VISIBLE) ? 1 : 0);
        borg_update();
        printf("OBSERVED the monster while lit\n");
        print_kills();
        /* The borg steps back west, out of the light; the monster's grid
         * goes dark again, and one turn passes. */
        step_player(10);
        printf("STEP2 player=(%d,%d) monster=(%d,%d) visible=%d\n",
               player->grid.x, player->grid.y, mon->grid.x, mon->grid.y,
               mflag_has(mon->mflag, MFLAG_VISIBLE) ? 1 : 0);
        borg.time.now++;
    } else {
        printf("OBSERVED the monster while lit\n");
        print_kills();
        borg.time.now++;
    }

    /* The next production per-turn pass: the scan (which cannot see the
     * monster in the dark), the view, the light map and the
     * missing-monster pass that may delete the record. */
    borg_update();

    print_kills();
    light_info(CORR_Y, mon->grid.x);
    if (lure) {
        /* The grid one step east of the monster: the corridor toward the
         * up staircase, the path the borg believes is clear. */
        light_info(CORR_Y, mon->grid.x + 1);
    }
    /* What the borg believes about the monster's grid after the pass: the
     * danger it expects on the grid itself and one grid west of it. */
    printf("DANGER monster_grid=%d west_of_monster=%d\n",
           borg_danger(CORR_Y, mon->grid.x, 1, true, false),
           borg_danger(CORR_Y, mon->grid.x - 1, 1, true, false));

    /* What the borg does next, in lure mode: the production per-turn
     * brain.  borg_think() weighs the up staircase east - the way its
     * map says it came from, and the only thing worth walking to -
     * against the danger it knows about on the way, and queues its
     * keys; the borg's own notes say what it decided. */
    bool flowed = false;
    int step_key = 0;
    if (lure) {
        borg_flush();
        memset(&borg.goal, 0, sizeof(borg.goal));
        borg_notice(true);
        flowed = borg_think();
        keycode_t keys[8];
        int nkeys = 0;
        keycode_t key;
        while ((key = borg_inkey(true)) != 0 && nkeys < 8)
            keys[nkeys++] = key;
        for (int i = 0; i < nkeys; i++)
            if (keys[i] >= '1' && keys[i] <= '9' && keys[i] != '5')
                { step_key = (int)keys[i]; break; }
        char keybuf[48];
        int kpos = 0;
        for (int i = 0; i < nkeys; i++)
            kpos += snprintf(keybuf + kpos, sizeof(keybuf) - kpos, "%s%d",
                i ? "," : "", (int)keys[i]);
        printf("DECISION flowed=%d keys=[%s] step=%d\n", flowed ? 1 : 0, keybuf, step_key);
        /* The borg's own notes for this decision (its reasoning). */
        printf("NOTES");
        for (int i = 7; i >= 0; i--) {
            const char *note = message_str(i);
            if (note && note[0]) printf(" | %s", note);
        }
        printf("\n");
    }

    printf("MAP After the pass: @ = the Borg, %c = the monster, ? = the borg's record%s\n",
           (char)mon->race->d_char,
           lure ? ", * = the borg's committed step" : "");
    {
        int kx = -1, ky = -1;
        for (int k = 1; k < borg_kills_nxt; ++k)
            if (borg_kills[k].r_idx) { kx = borg_kills[k].pos.x; ky = borg_kills[k].pos.y; }
        int sx = -1, sy = -1;
        if (step_key && step_key != '5') {
            sx = player->grid.x + ddgrid[step_key - '0'].x;
            sy = player->grid.y + ddgrid[step_key - '0'].y;
        }
        for (int y = 0; y < cave->height; ++y) {
            char row[256];
            int x;
            for (x = 0; x < cave->width && x < 255; ++x) {
                struct loc grid = loc(x, y);
                wchar_t c = f_info[square(cave, grid)->feat].d_char;
                struct monster *m = square_monster(cave, grid);
                if (m) c = m->race->d_char;
                if (loc_eq(grid, player->grid)) c = L'@';
                if (x == kx && y == ky) c = L'?';
                if (x == sx && y == sy) c = L'*';
                row[x] = (c > 31 && c < 127) ? (char)c : '?';
            }
            row[x] = 0;
            printf("MAPROW %s\n", row);
        }
        printf("MAPEND\n");
    }
    int kills = 0, kx = -1;
    for (int k = 1; k < borg_kills_nxt; ++k)
        if (borg_kills[k].r_idx) { kills++; kx = borg_kills[k].pos.x; }
    printf("STATE kills=%d record_x=%d monster_x=%d flowed=%d\n", kills, kx, mon->grid.x, flowed ? 1 : 0);
    int kept = kills > 0 ? 1 : 0;
    printf("OUTCOME kept=%d\n", kept);
    /* Corrected contract: a monster that was seen while lit and now
     * stands in the dark just outside the light is remembered, and one
     * that stays in the light is re-observed and kept. */
    require(kept == 1);
    ok;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/lightmap-644-saveplay";
struct test tests[] = {{"record_outside_the_light", scenario_light}, {NULL, NULL}};
