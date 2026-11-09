# The Borg did not see a Rune of Protection and read another one on top of it

This is **runeglyph-295 / issue #295**, a C Borg defect
(diegoami/neo-angband-tracker#295).

A Borg standing on a Rune of Protection that it did not lay itself (here, one the player read just before handing over to the Borg) does not know the rune is there. Its map update only records a rune inside the branch for visible, disarmable player traps, and a Rune of Protection is not a player trap (its kind is GLYPH | VISIBLE | FLOOR, without TRAP). With a summoner in view, the Borg's defence choice therefore still counts the full summoning danger (403 against 308 hit points) and reads another Rune of Protection onto the same grid: one scroll is spent and the game stacks a second rune where one already was. The correction also records a glyph that the player remembers on the grid. The Borg's map then shows the rune, its danger on that grid drops to the protected value (83), and it does not read a second rune; it drinks a Potion of Berserk Strength instead, its next-best defence in this kit.

## What to watch

Borg read a Rune of Protection (1) or not (0); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `50e64a37a`) |
| --- | --- | --- |
| Standing on a visible Rune of Protection | failed, read_rune=1 | passed, read_rune=0 |
| Control: plain floor, no rune | passed, read_rune=1 | byte-identical log |
| Control: revealed pit next to the Borg | passed, read_rune=1 | byte-identical log |

- **Standing on a visible Rune of Protection.** Original: the map has no glyph on the Borg's grid (glyph 0, tracked 0), the danger is 403 and the Borg reads a Rune of Protection (keys r d): scrolls 4 to 3, runes on the grid 1 to 2. Corrected: glyph 1, tracked 1, danger 83, no rune read (scrolls stay 4, one rune); the Borg drinks a Potion of Berserk Strength instead.

- **Control: plain floor, no rune.** No rune on the grid: no glyph is recorded in either version, the danger is 403 and the Borg reads a Rune of Protection (scrolls 5 to 4, runes 0 to 1). Byte-identical logs.

- **Control: revealed pit next to the Borg.** The revealed pit west of the Borg goes through the changed branch and is recorded as a trap in both versions (trap 1, glyph 0); the Borg reads a rune on its own floor grid. Byte-identical logs.

## Conditions

Native Human Warrior, seed 295, character level 30, depth 30, a lit 40x24 open arena, lantern, 5 known Scrolls of Rune of Protection and the class's starting kit. Fixture interventions: the arena, level and experience; the carried scrolls and lantern; an awake ranger (a summoner: S_ANIMAL) placed 4 grids east of the player; in rune mode the player reads one Rune of Protection through the game's own read command (do_cmd_read_scroll) before the Borg starts; in trap mode a pit is placed next to the player (place_trap) and revealed with the game's square_reveal_trap. Real code: the Borg's map update (borg_update, three passes), its danger evaluation (borg_danger) and its production defence choice (borg_defend, called with the danger at its grid as the caution path does). When the Borg queues a Rune of Protection read (r + the scroll's letter), the read is executed through the game's command and the scrolls and runes on the grid are counted.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/runeglyph-295
git fetch https://github.com/diegoami/neo-angband-tracker.git task/runeglyph-295-evidence
git show 3ce928e869641bd2d789dfca1566c890121922f9:docs/audit/c-reports/runeglyph-295/reproduce.py > /tmp/reproduce-runeglyph-295.py
python3 /tmp/reproduce-runeglyph-295.py --full-tests --fixtures-ref 3ce928e869641bd2d789dfca1566c890121922f9 --fix-ref 50e64a37a837c3ce4563e9ade452ce28db5cee82
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-update.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `50e64a37a837c3ce4563e9ade452ce28db5cee82` (branch `task/runeglyph-295`), fixtures
  `3ce928e869641bd2d789dfca1566c890121922f9` (branch `task/runeglyph-295-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: floor, trap
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_update_map() (borg-update.c) sets ag->glyph and adds the grid to track_glyph only inside the square_isdisarmabletrap() branch. That predicate requires TRF_TRAP, which the glyph of warding and decoy kinds do not carry, so a visible Rune of Protection is never recorded. borg_danger() then gives no rune reduction for summoning on that grid, and borg_defend_aux_glyph() reads another rune onto it. The correction also enters the branch when map_info's remembered trap for the grid has TRF_GLYPH, and sets ag->trap only for a disarmable trap; the existing glyph test and tracking then run unchanged.

The fixture creates the situation directly: a rune read by the player before the Borg starts, a single summoner in an open lit arena and a fixed kit. It shows one wasted scroll and turn per occurrence; it does not measure how often a live game reaches a known rune that the Borg did not record itself (runes laid before the Borg starts, runes forgotten when its rune list is cleared by a hit or a broken-rune message, or decoys), nor any survival or combat effect. Decoys carry the same GLYPH flag and are recorded as glyphs too, as the existing branch already intended for any TRF_GLYPH kind. Other glyph readers (Sea of Runes flow, happy-grid, escape and flow checks) now see the recorded rune but are not exercised here.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
