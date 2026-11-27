/* Synthetic native C monster-tracker fixture for issue #656 (killslot-656).
 * Excluded from the source fix.  When the Borg's monster table is full
 * (borg_kills_nxt == 255, active slots 1..254), borg_new_kill() stole a
 * victim with randint1(borg_kills_nxt - 1) + 1, which yields 2..255.
 * Valid active indices are 1..254: slot 255 is allocated storage that the
 * normal tracking scans (i < borg_kills_nxt) never visit, so a monster
 * placed there is never refreshed or forgotten - a stale record the map
 * can reference forever - and because the deletion of the empty slot
 * returns before decrementing, the active count overstates by one.
 * The same off-by-one also made slot 1 unreachable as a victim
 * (randint1 + 1 starts at 2).
 *
 * The table is pre-filled: slots 1..254 carry live fake records, the
 * count is 254 and the next index 255.  A real, visible floating eye is
 * then added and the production borg_update() pass creates it, forcing
 * the steal path.  The RNG is seeded immediately before the update so
 * the single steal draw is pinned per mode:
 *   upper_edge  draw % 254 == 253: the corrected borg steals slot 254
 *               (last active slot); the original steals 255 - outside
 *               the active range, unvisited by scans, count 255.
 *   slot_one    draw % 254 == 0: the corrected borg steals slot 1 (first
 *               active slot); the original steals 2 - slot 1 is never
 *               eligible as a victim.
 *   ordinary    control: an unfilled table allocates normally
 *               (byte-identical before/after).
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
#include "ui-input.h"
#include "ui-term.h"
#include "ui-prefs.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-util.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow-stairs.h"
#include "borg/borg-flow-kill.h"
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
            struct monster *mon = square_monster(cave, grid);
            if (mon)
                c = mon->race->d_char;
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
#define EYE_X 23
#define EYE_Y 12

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
    return streq(mode, "upper_edge") || streq(mode, "slot_one")
        || streq(mode, "ordinary") || streq(mode, "scan");
}

/* The race index the fake records carry is chosen at runtime. */

int setup_tests(void **state)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    const char *profile = getenv("KS656_PROFILE");
    mode = getenv("KS656_MODE");
    if (!profile || !mode || !mode_valid()) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(656);
    if (!player_make_simple("Human", "Warrior", "KillSlot656")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    /* An open, lit arena floor; no monsters of its own. */
    cave = t_build_arena(H, W);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    player->cave = cave_new(H, W);
    player_place(cave, player, loc(20, EYE_Y));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 5;
    player_exp_gain(player, player_exp[18] * player->expfact / 100);
    character_dungeon = true;
    struct object *torch = known_item(TV_LIGHT, "Wooden Torch", 1);
    torch->timeout = 4000;
    inven_wield(torch, wield_slot(torch));
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
    printf("SETUP seed=656 mode=%s class=%s clevel=%d hp=%d depth=%d\n",
           mode, player->class->name, player->lev, player->chp, player->depth);
    return 0;
}

/* Fill the active slots 1..254 with live fake records. */
static void fill_table(unsigned int race)
{
    int placed = 0;
    /* Clear everything the previous iteration may have left behind,
     * including slot 255 (which the original can occupy) and the grid
     * links. */
    memset(&borg_kills[255], 0, sizeof(borg_kill));
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            borg_grids[y][x].kill = 0;
    for (int i = 1; i < 255; ++i) {
        borg_kill *kill = &borg_kills[i];
        memset(kill, 0, sizeof(*kill));
        kill->r_idx  = race;
        kill->awake  = true;
        kill->when   = borg.time.now; /* "seen this turn" */
        /* Spread the fakes over the arena, away from the eye. */
        kill->pos.x  = (i * 7) % (W - 2) + 1;
        kill->pos.y  = (i * 5) % (H - 2) + 1;
        if (kill->pos.y == EYE_Y && (kill->pos.x == EYE_X
            || kill->pos.x == player->grid.x))
            kill->pos.x = (kill->pos.x + 5) % (W - 2) + 1;
        kill->m_idx  = 0;
        kill->power  = 10;
        borg_grids[kill->pos.y][kill->pos.x].kill = i;
        ++placed;
    }
    borg_kills_nxt = 255;
    borg_kills_cnt = (int16_t)placed;
}

/* A real, harmless race for the filler records. */
static unsigned int filler_race(unsigned int not_this)
{
    for (unsigned int r = 1; r < (unsigned int)z_info->r_max - 1; ++r)
        if (r_info[r].name && r_info[r].rarity != 0 && r != not_this)
            return r;
    return 1;
}

static int scenario_full_table_steal(void *state)
{
    unsigned int eye_race;
    int eye_slot = -1, scan_visited = 0;
    struct monster *mon;
    const char *seedenv = getenv("KS656_SEED");
    /* Pinned steal draws (found by scanning seeds through the real
     * mode path on the original build; both variants consume the
     * identical draw): 551 sends the original to the out-of-range slot
     * 255 (the corrected borg steals 254, the last active slot); 305
     * sends the original to slot 2 (the corrected borg steals 1, the
     * first active slot). */
    long seed = seedenv ? strtol(seedenv, NULL, 10)
        : (streq(mode, "upper_edge") ? 551
            : (streq(mode, "slot_one") ? 305 : 656));
    bool full = !streq(mode, "ordinary");
    mon = t_add_monster(cave, loc(EYE_X, EYE_Y), "floating eye");
    if (!mon) { printf("SETUPFAIL monster\n"); return 1; }
    mflag_on(mon->mflag, MFLAG_AWARE);
    update_mon(mon, cave, true);
    if (!mflag_has(mon->mflag, MFLAG_VISIBLE)) { printf("SETUPFAIL visible\n"); return 1; }
    eye_race = (unsigned int)mon->race->ridx;

    /* Seed scan: repeat the full-table steal for many seeds in one
     * process (development helper that documents the choice of the
     * pinned seeds for the upper_edge and slot_one modes). */
    if (streq(mode, "scan")) {
        fill_table(filler_race(eye_race));
        for (long s = 1; s <= 800; ++s) {
            fill_table(filler_race(eye_race));
            Rand_state_init((unsigned int)s);
            borg_update();
            int slot = -1;
            for (int i = 1; i < 256; ++i)
                if (borg_kills[i].r_idx == eye_race
                    && borg_kills[i].pos.x == EYE_X
                    && borg_kills[i].pos.y == EYE_Y) { slot = i; break; }
            if (slot == 255 || slot == 254 || slot == 2 || slot == 1)
                printf("SCAN seed=%ld slot=%d count=%d\n", s, slot, borg_kills_cnt);
        }
        ok;
    }

    if (full) fill_table(filler_race(eye_race));
    printf("TABLE full=%d nxt=%d cnt=%d\n", full ? 1 : 0,
           borg_kills_nxt, borg_kills_cnt);

    /* Pin the single steal draw: the update below performs exactly one
     * borg_new_kill() for the eye. */
    Rand_state_init((unsigned int)seed);
    borg_update();

    /* Where did the eye land? */
    for (int i = 1; i < 256; ++i) {
        if (borg_kills[i].r_idx == eye_race && borg_kills[i].pos.x == EYE_X
            && borg_kills[i].pos.y == EYE_Y) {
            eye_slot = i;
            break;
        }
    }
    /* The production scan range: how many live records can it see? */
    for (int i = 1; i < borg_kills_nxt; ++i)
        if (borg_kills[i].r_idx)
            ++scan_visited;

    printf("NEWSLOT slot=%d next=%d count=%d visited=%d max_active=254\n",
           eye_slot, borg_kills_nxt, borg_kills_cnt, scan_visited);
    printf("STATE slot=%d count=%d in_scan=%d\n", eye_slot,
           borg_kills_cnt,
           (eye_slot > 0 && eye_slot < borg_kills_nxt) ? 1 : 0);
    /* The map link of the eye's grid must resolve to the tracked record
     * through the production link. */
    int link = borg_grids[EYE_Y][EYE_X].kill;
    printf("LINK grid_kill=%d resolves=%d\n", link,
           (link > 0 && link < 256 && borg_kills[link].r_idx == eye_race
            && borg_kills[link].pos.x == EYE_X
            && borg_kills[link].pos.y == EYE_Y) ? 1 : 0);

    /* Corrected contract: the new record lives in an ACTIVE slot
     * (1..254), the scans can visit it, the grid link resolves, and the
     * active count stays at capacity on a full table.  The pinned modes
     * also pin the endpoint the draw maps to: 551 maps the upper
     * endpoint draw to the LAST active slot (254) - the original sent
     * it to the out-of-range slot 255 - and 305 maps the lower endpoint
     * draw to the FIRST active slot (1) - the original started its
     * range at 2, making slot 1 unreachable as a victim. */
    require(eye_slot >= 1 && eye_slot <= 254);
    require(eye_slot < borg_kills_nxt);
    require(link > 0 && link == eye_slot);
    if (full) require(borg_kills_cnt == 254);
    else require(borg_kills_cnt >= 1);
    if (streq(mode, "upper_edge")) require(eye_slot == 254);
    if (streq(mode, "slot_one")) require(eye_slot == 1);

    evidence_map("After the full-table steal: @ = the Borg, e = the new monster");
    ok;
}

int teardown_tests(void *state)
{
    borg_update_entrypoint(false);
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

const char *suite_name = "native-c/killslot-656";
struct test tests[] = {
    {"full_table_steal_stays_in_active_range", scenario_full_table_steal},
    {NULL, NULL}};
