# A tall or wide terminal made the Borg read past its own map

This is **panelrows-348 / issue #348**, a C Borg defect
(diegoami/neo-angband-tracker#348).

The Borg keeps its own copy of the level map, 66 rows of 198 grids (the largest level). Three of its searches look only at the part of the map the game is showing: looking for a lit area to walk to when it has no light, looking for a vault wall it can dig, and looking for a vault wall it can turn to mud. The size of that visible part grows with the terminal window. With the standard sidebar, a terminal of 69 or more rows, or 213 or more columns, shows more rows or columns than any level has, and the game puts the view at the level's edge. The searches then read past the end of the Borg's map: in a 70-row terminal the game crashed (segmentation fault) as soon as one of these searches ran; in a 230-column terminal it read neighbouring memory without crashing. The correction stops each search at the edge of the Borg's map; on standard terminals nothing changes.

## What to watch

The production call returned and the scenario finished (1); n/a means the run stopped on a crash or sanitizer report; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `82702575b`) |
| --- | --- | --- |
| Glow scan, 80x70 terminal (panel 68 rows on a 66-row level) | failed, completed=n/a | passed, completed=1 |
| Glow scan, 230x24 terminal (panel 216 columns on a 198-column level) | passed, completed=1 | passed, completed=1 |
| Control: glow scan, standard 80x24 terminal | passed, completed=1 | byte-identical log |
| Control: glow scan, 212x68 terminal (panel exactly 198x66, the largest that fits) | passed, completed=1 | byte-identical log |
| Vault-wall scan, 80x70 terminal | failed, completed=n/a | passed, completed=1 |
| Vault-wall scan, 230x24 terminal | passed, completed=1 | passed, completed=1 |
| Control: vault-wall scan, standard 80x24 terminal | passed, completed=1 | byte-identical log |
| Stone to Mud vault scan (Druid), 80x70 terminal | failed, completed=n/a | passed, completed=1 |
| Stone to Mud vault scan (Druid), 230x24 terminal | passed, completed=1 | passed, completed=1 |
| Control: Stone to Mud vault scan, standard 80x24 terminal | passed, completed=1 | byte-identical log |

- **Glow scan, 80x70 terminal (panel 68 rows on a 66-row level).** Original: the glow scan reads row pointers 66 and 67 past the end of the Borg's 66-row map; the normal build crashes (segmentation fault) and AddressSanitizer reports a global-buffer-overflow READ in borg_flow_light (borg-flow-misc.c:836). Corrected: the scan stops at row 65, finds the 88 lit grids of the room and the Borg steps toward them (key 49).

- **Glow scan, 230x24 terminal (panel 216 columns on a 198-column level).** Original: the glow scan reads 18 grids past the end of every map row; the normal build does not notice (it reads neighbouring heap memory and happens to find no stray glow), AddressSanitizer reports a heap-buffer-overflow READ in borg_flow_light (borg-flow-misc.c:839). Corrected: no out-of-map read; the same 66 lit grids and the same step (key 49).

- **Control: glow scan, standard 80x24 terminal.** The panel (66x22) lies inside the map; both versions find the same 66 lit grids and step toward them (key 49); byte-identical logs.

- **Control: glow scan, 212x68 terminal (panel exactly 198x66, the largest that fits).** The panel is exactly the size of the map; the clamp never cuts the loop. Both versions find all 88 lit grids; byte-identical logs.

- **Vault-wall scan, 80x70 terminal.** Original: borg_flow_vault(35) reads the missing rows 66 and 67 for grids within 35 steps; the normal build crashes, AddressSanitizer reports a global-buffer-overflow READ (borg-flow-misc.c:909). Corrected: the magma vein next to the permanent wall is found and the Borg flows toward it (key 50).

- **Vault-wall scan, 230x24 terminal.** Original: reads past the end of rows near the east edge; silent in the normal build, AddressSanitizer heap-buffer-overflow READ (borg-flow-misc.c:909). Corrected: same target and step (key 50), no out-of-map read.

- **Control: vault-wall scan, standard 80x24 terminal.** Panel inside the map; both versions flow to the same vein (key 50); byte-identical logs.

- **Stone to Mud vault scan (Druid), 80x70 terminal.** Original: borg_excavate_vault() reads the missing rows; the normal build crashes, AddressSanitizer global-buffer-overflow READ (borg-think-dungeon-util.c:1016). Corrected: the Borg targets the vein and casts Turn Stone to Mud (first key 42, the targeting key).

- **Stone to Mud vault scan (Druid), 230x24 terminal.** Original: silent read past the row ends in the normal build, AddressSanitizer heap-buffer-overflow READ (borg-think-dungeon-util.c:1016). Corrected: same cast, no out-of-map read.

- **Control: Stone to Mud vault scan, standard 80x24 terminal.** Panel inside the map; both versions cast Turn Stone to Mud at the same vein; byte-identical logs.

## Conditions

Native Human Warrior (light and vault scans) or Human Druid (Stone to Mud scan), seed 348, character level 21, depth 10, on a full-size open level (198x66). Terminal sizes are set with term_init (standard sidebar layout: map starts at column 13, one status row above and below). The game's own center_panel() places the view around the player, and the Borg's panel origin is copied from the terminal exactly as borg_update() does. Fixture interventions: the terminal size, the arena, the Borg's map entries (features copied from the level; the 11x8 lit room is marked as lit in the Borg's map, as its call-light code records it), the vault flag, a wielded Mattock (Warrior vault cases), the Druid's two town books with every spell up to its level learned. Each case runs one production call: borg_flow_light(GOAL_FLEE), borg_flow_vault(35) or borg_excavate_vault(z_info->max_range - 2), as borg_think_dungeon() calls them, and reads back the first queued key. The replay page shows the AddressSanitizer build; the normal-build results are in the README table.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/panelrows-348
git fetch https://github.com/diegoami/neo-angband-tracker.git task/panelrows-348-evidence
git show aac11f0e0f96354a2c425213396d19c82f93ed22:docs/audit/c-reports/panelrows-348/reproduce.py > /tmp/reproduce-panelrows-348.py
python3 /tmp/reproduce-panelrows-348.py --full-tests --fixtures-ref aac11f0e0f96354a2c425213396d19c82f93ed22 --fix-ref 82702575b2179cb27cb7eac24272515dfea0830e
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-misc.c, src/borg/borg-think-dungeon-util.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `82702575b2179cb27cb7eac24272515dfea0830e` (branch `task/panelrows-348`), fixtures
  `aac11f0e0f96354a2c425213396d19c82f93ed22` (branch `task/panelrows-348-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: light-normal, light-fit, vault-normal, dig-normal
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_flow_light(), borg_flow_vault() and borg_excavate_vault() loop over the visible panel, w_y .. w_y + SCREEN_HGT and w_x .. w_x + SCREEN_WID, and index borg_grids[y][x], which has AUTO_MAX_Y (66) rows of AUTO_MAX_X (198) grids. SCREEN_HGT and SCREEN_WID follow the terminal size; when the panel is larger than the level, modify_panel() clamps the origin to 0 and the loops run past the map. On a 70-row terminal the original reads borg_grids[66] and [67] (AddressSanitizer: global-buffer-overflow READ of size 8) and the normal build crashes; on a 230-column terminal it reads 18 grids past each row (heap-buffer-overflow READ) silently. Bounding both loops by AUTO_MAX_Y / AUTO_MAX_X removes every report; standard-terminal and exact-fit controls are byte-identical.

The trigger is a terminal larger than the dungeon: with the default left sidebar, 69 or more rows or 213 or more columns (with the top-bar or no-sidebar layouts, 199 or more columns, or 71 or 68 or more rows; larger tiles make the panel smaller, so the threshold grows). The cases here were measured only with the default layout. An 80x24 to 212x68 terminal never reaches it. The three scans run only in particular situations: borg_flow_light() when the Borg has run out of light below the town, could not recall or take stairs, and still counts a Word of Recall; borg_flow_vault() and borg_excavate_vault() only on a level with a vault and the means to dig or cast Stone to Mud. The fixture calls each scan directly; it does not measure how often a full game on a large terminal reaches them, and no live-game crash has been observed. Smaller levels (town, caverns, labyrinths) do not read outside the Borg's map, because it is always allocated at full size. The Borg's debug line-of-sight display (borg-commands.c) has the same unbounded loops but only runs on a debug command; it is not changed here. The map update scan (borg-update.c) already checks bounds and was never affected.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
