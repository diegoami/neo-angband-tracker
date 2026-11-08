/* Synthetic native C full-pack drop fixture, excluded from the
 * source fix.  Issue: diegoami/neo-angband-tracker#136 (drophole-136). */
#include "unit-test.h"
#include "test-utils.h"
#include "cave.h"
#include "game-world.h"
#include "init.h"
#include "message.h"
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
#include "ui-term.h"
#include "ui-prefs.h"
#include "ui-menu.h"
#include "borg/borg-init.h"
#include "borg/borg-inventory.h"
#include "borg/borg-item.h"
#include "borg/borg-prepared.h"
#include "borg/borg-trait.h"
#include "borg/borg-junk.h"
#include "borg/borg-io.h"
#include "borg/borg-power.h"
#include "borg/borg-cave.h"
#include "borg/borg-cave-view.h"
#include "borg/borg-flow.h"
#include "borg/borg-update.h"
#include "borg/borg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static term screen;

/* Make the object fully known to the player, as after normal play with
 * every rune learned. */
static struct object *carry_known(struct object *obj)
{
    obj->known = object_new();
    object_set_base_known(player, obj);
    object_flavor_aware(player, obj);
    inven_carry(player, obj, false, false);
    object_touch(player, obj);
    player_know_object(player, obj);
    return obj;
}

static struct object *known_item(int tval, const char *name, int number)
{
    struct object *obj       = object_new();
    struct object_kind *kind = lookup_kind(tval, lookup_sval(tval, name));
    if (!kind) { fprintf(stderr, "MISSING KIND %s\n", name); quit("fixture item kind missing"); }
    object_prep(obj, kind, 50, MAXIMISE);
    obj->number = number;
    return carry_known(obj);
}

int setup_tests(void **state)
{
    const char *profile = getenv("DH136_PROFILE");
    const char *mode    = getenv("DH136_MODE");
    if (!profile || !mode) return 1;
    set_file_paths();
    string_free(ANGBAND_DIR_USER); ANGBAND_DIR_USER = string_make(profile);
    string_free(ANGBAND_DIR_SAVE); ANGBAND_DIR_SAVE = string_make(format("%s/save", profile));
    string_free(ANGBAND_DIR_PANIC); ANGBAND_DIR_PANIC = string_make(format("%s/panic", profile));
    string_free(ANGBAND_DIR_SCORES); ANGBAND_DIR_SCORES = string_make(format("%s/scores", profile));
    string_free(ANGBAND_DIR_ARCHIVE); ANGBAND_DIR_ARCHIVE = string_make(format("%s/archive", profile));
    if (!init_angband()) { printf("SETUPFAIL init\n"); return 1; }
    create_needed_dirs();
    Rand_quick = false; Rand_state_init(136);
    if (!player_make_simple("Human", "Warrior", "DropHole136")) { printf("SETUPFAIL birth\n"); return 1; }
    term_init(&screen, 198, 66, 256); Term_activate(&screen); angband_term[0] = &screen;
    textui_prefs_init();
    cave = t_build_arena(24, 24);
    player->cave = cave_new(24, 24);
    player_place(cave, player, loc(12, 12));
    player->depth = player->max_depth = cave->depth = player->cave->depth = 20;
    player_exp_gain(player, player_exp[streq(mode, "level30") ? 28 : 38] * player->expfact / 100);
    character_dungeon = true;
    for (int y = 1; y < 23; ++y) for (int x = 1; x < 23; ++x)
        sqinfo_on(square(cave, loc(x, y))->info, SQUARE_GLOW);
    /* Start from an empty pack: remove the starting kit's carried items. */
    {
        struct object *obj = player->gear, *next;
        while (obj) {
            next = obj->next;
            if (!object_is_equipped(player->body, obj)) {
                bool none_left = false;
                obj = gear_object_for_use(player, obj, obj->number, false, &none_left);
                if (obj->known) object_delete(player->cave, NULL, &obj->known);
                object_delete(cave, player->cave, &obj);
            }
            obj = next;
        }
    }
    /* The starting kit's wielded torch stays as the light; nothing else
     * is carried apart from what follows. */
    known_item(TV_FOOD, "Ration of Food", 3);
    /* A spare amulet the Borg is not wearing.  "spare-esp": the Borg
     * wears an Amulet of ESP and carries a second one. */
    if (streq(mode, "spare-esp")) {
        { struct object *worn = known_item(TV_AMULET, "ESP", 1); inven_wield(worn, wield_slot(worn)); }
        known_item(TV_AMULET, "ESP", 1);
    } else
        known_item(TV_AMULET, "Slow Digestion", 1);
    /* "salt-water" and "level30": a worthless potion.  "restore-mana": a
     * potion the drop code is meant to protect at high level. */
    known_item(TV_POTION, streq(mode, "restore-mana") ? "Restore Mana" : "Salt Water", 1);
    /* Fill the rest of the pack with artifacts, which the full-pack drop
     * never considers, so the choice is between the amulet and the potion. */
    for (int a = 1; a < z_info->a_max; ++a) {
        const struct artifact *art = &a_info[a];
        if (!art->name || (art->tval != TV_SWORD && art->tval != TV_HAFTED
                && art->tval != TV_POLEARM))
            continue;
        struct object *obj = object_new();
        object_prep(obj, lookup_kind(art->tval, art->sval), 50, MAXIMISE);
        obj->artifact = art;
        copy_artifact_data(obj, art);
        if (!inven_carry_okay(obj)) { object_delete(player->cave, NULL, &obj); break; }
        carry_known(obj);
    }
    /* Every rune known, as for a character this far into the game. */
    player_learn_all_runes(player);
    calc_inventory(player);
    player->upkeep->update |= PU_BONUS | PU_HP | PU_TORCH | PU_INVEN;
    update_stuff(player);
    player->chp = player->mhp;
    borg_rand_local = 1000; borg_init();
    if (borg_init_failure || borg_cfg[BORG_CHEAT_DEATH] || OPT(player, cheat_live)) { printf("SETUPFAIL borg\n"); return 1; }
    borg_equipment(); borg_inventory(); borg_notice(true);
    borg_alloc_detection();
    for (int y = 0; y < 24; ++y) for (int x = 0; x < 24; ++x) {
        borg_grids[y][x].feat = square(cave, loc(x, y))->feat;
        borg_grids[y][x].info |= BORG_OKAY | BORG_VIEW;
    }
    borg.c = player->grid; borg.avoidance = player->chp;
    borg.status.active = true;
    printf("SETUP seed=136 mode=%s class=%s clevel=%d depth=%d\n",
           mode, player->class->name, player->lev, player->depth);
    return 0;
}

int teardown_tests(void *state)
{
    textui_prefs_free();
    cleanup_angband(); term_nuke(&screen); angband_term[0] = NULL; return 0;
}

static int scenario_drop(void *state)
{
    const char *mode = getenv("DH136_MODE");
    char keys[256];
    int nkeys = 0;
    borg_notice(true);
    borg.power = borg_power();
    for (int i = 0; i < z_info->pack_size; ++i) {
        borg_item *it = &borg_items[i];
        if (!it->iqty || it->art_idx) continue;
        printf("PACK %c %s value=%d weight=%d\n", all_letters_nohjkl[i],
               it->desc, (int)it->value, (int)borg_item_weight(it));
    }
    memcpy(safe_items, borg_items, QUIVER_END * sizeof(borg_item));
    borg_flush();
    uint16_t before = messages_num();
    /* The production full-pack drop, non-desperate first pass. */
    bool rc = borg_drop_hole(false);
    uint16_t mid = messages_num();
    for (int i = (int)(mid - before) - 1; i >= 0; --i)
        printf("TRACK %s\n", message_str((uint16_t)i));
    /* Read back the keys the Borg queued. */
    for (keycode_t k; nkeys < 255 && (k = borg_inkey(true)) != 0; )
        keys[nkeys++] = (k == KC_ENTER) ? '$' : (char)k;
    keys[nkeys] = '\0';
    printf("KEYS %s\n", keys);
    /* Expected shape: '{' letter "borg ignore" ENTER 'd' letter. */
    int slot = -1;
    if (nkeys >= 16 && keys[0] == '{' && keys[14] == 'd' && keys[15] == keys[1]
        && !strncmp(keys + 2, "borg ignore$", 12))
        for (int i = 0; i < z_info->pack_size; ++i)
            if (all_letters_nohjkl[i] == keys[1]) slot = i;
    char name[120] = "none";
    int amulet = 0, potion = 0;
    if (slot >= 0 && player->upkeep->inven[slot]) {
        struct object *obj = player->upkeep->inven[slot];
        object_desc(name, sizeof(name), obj, ODESC_PREFIX | ODESC_FULL, player);
        amulet = obj->tval == TV_AMULET;
        potion = obj->tval == TV_POTION;
        /* Execute the drop with the game's own inventory drop. */
        uint16_t pre = messages_num();
        inven_drop(obj, obj->number);
        uint16_t post = messages_num();
        for (int i = (int)(post - pre) - 1; i >= 0; --i)
            printf("ENGINE %s\n", message_str((uint16_t)i));
    }
    int floor = 0;
    for (struct object *o = square_object(cave, player->grid); o; o = o->next) ++floor;
    printf("DECISION drop=%d slot=%c item=%s\n", rc ? 1 : 0,
           slot >= 0 ? all_letters_nohjkl[slot] : '-', name);
    printf("OUTCOME dropped_amulet=%d dropped_potion=%d floor_items=%d\n",
           amulet, potion, floor);
    require(slot >= 0);
    if (!streq(mode, "restore-mana"))
        require(potion && !amulet); /* fails before at level 40: the amulet goes, the salt water stays */
    else
        require(amulet && !potion); /* protected potion: the amulet is the cheapest loss */
    ok;
}

const char *suite_name = "native-c/drophole-136";
struct test tests[] = {
    {"full_pack_drops_the_junk", scenario_drop},
    {NULL, NULL}};
