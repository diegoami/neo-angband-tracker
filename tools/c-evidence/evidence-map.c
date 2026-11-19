/* Dungeon map for the evidence replay (tools/c-evidence/evidence-map.c).
 *
 * Copy this block into a package's scenario.c, after its includes (it needs
 * cave.h, monster.h and player.h, which every fixture already includes).
 * The evidence runner copies only scenario.c, so the helper travels inline.
 *
 * evidence_map() prints the current level with the game's own symbols: each
 * grid's feature symbol from f_info, each monster's race symbol, '@' for the
 * player, plus up to a few marks the scenario chooses (for example '*' on the
 * grid the Borg's chosen step leads to, '-' along a bolt's path).  It reads
 * the game's cave, not the Borg's memory, and changes no game state.
 *
 * Output, parsed by the generated make_replay.py:
 *   MAP <caption>
 *   MAPROW <text>      (one per row, top to bottom)
 *   MAPEND
 *
 * Print the same maps in every phase; a control's map is then byte-identical
 * before and after like the rest of its log. */
struct evidence_mark {
    struct loc grid;
    char glyph;
};

static void evidence_map(const char *caption, const struct evidence_mark *marks,
    int n)
{
    printf("MAP %s\n", caption);
    for (int y = 0; y < cave->height; ++y) {
        char row[256];
        int x;
        for (x = 0; x < cave->width && x < 255; ++x) {
            struct loc grid = loc(x, y);
            wchar_t c = f_info[square(cave, grid)->feat].d_char;
            struct monster *mon = square_monster(cave, grid);
            for (int i = 0; i < n; ++i)
                if (loc_eq(grid, marks[i].grid))
                    c = (wchar_t)marks[i].glyph;
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
