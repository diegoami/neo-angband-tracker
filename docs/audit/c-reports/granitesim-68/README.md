# A fast caster behind passable rubble counted as no danger to the Borg

This is **granitesim-68 / issue #68**, a C Borg defect
(diegoami/neo-angband-tracker#68).

The Borg's danger estimate lets a fast monster take one step before it attacks. For ranged attacks it checks each grid the monster could step to and asks whether the Borg is in its line of fire from there. That check treated every feature numbered at or above granite as a wall, which also includes lava and passable rubble, so a monster could never step onto them unless it walks through walls. A blacklock mage (speed +10) in a dead-end pocket whose only exit is a pile of passable rubble can walk onto the rubble and cast in the same turn, but the Borg counted its danger as 0. A level-12 Warrior five steps away therefore took no precaution. With the check limited to granite, as in the melee step check, the danger is 208 (the same as with an open floor exit) and the Borg flees the level with Word of Recall, which the real read command then activates.

## What to watch

Borg danger at its own grid (borg_danger, one turn); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `f1b39e205`) |
| --- | --- | --- |
| Passable rubble between the mage and the Borg | failed, danger=0 | passed, danger=208 |
| Control: open floor between the mage and the Borg | passed, danger=208 | byte-identical log |
| Control: granite between the mage and the Borg | passed, danger=0 | byte-identical log |

- **Passable rubble between the mage and the Borg.** Original: the rubble counts as a wall the mage cannot step onto, danger 0, borg_caution() does nothing. Corrected: the mage can step onto the rubble and cast, danger 208; the Borg flees the level ('# Fleeing the level (recall)') and the real read activates Word of Recall.

- **Control: open floor between the mage and the Borg.** Floor is not a wall in either version: danger 208 and the same Word of Recall flight in both, byte-identical logs. This is what the corrected rubble case matches.

- **Control: granite between the mage and the Borg.** Granite is a wall in both versions; the mage cannot reach a firing grid, danger 0, no action, byte-identical logs.

## Conditions

Native Human Warrior, seed 68, level 12, depth 10, 159 hit points, starting kit plus 5 Phase Door, 5 Cure Light Wounds, temporary telepathy (so the walled-in monster is sensed), in a lit synthetic 30x30 arena. A blacklock mage (speed +10) sits in a pocket five steps south, walled with granite except the grid between it and the Borg, which holds the mode's terrain: passable rubble (main case), open floor (control) or granite (control). The Borg's map is copied from the game cave; the mage is tracked awake. The production borg_caution() runs once; a queued scroll read is executed with the real CMD_READ_SCROLL command.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/granitesim-68
git fetch https://github.com/diegoami/neo-angband-tracker.git task/granitesim-68-evidence
git show dc1b3433f763f2a207c7766f86be6d312fb9121d:docs/audit/c-reports/granitesim-68/reproduce.py > /tmp/reproduce-granitesim-68.py
python3 /tmp/reproduce-granitesim-68.py --full-tests --fixtures-ref dc1b3433f763f2a207c7766f86be6d312fb9121d --fix-ref f1b39e2050e2e9061da1f46dd546e80597d5ac45
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-danger.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `f1b39e2050e2e9061da1f46dd546e80597d5ac45` (branch `task/granitesim-68`), fixtures
  `dc1b3433f763f2a207c7766f86be6d312fb9121d` (branch `task/granitesim-68-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: floor, granite
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The ranged step simulation in borg_danger_one_kill() tested feat >= FEAT_GRANITE where the melee one tests feat == FEAT_GRANITE, so lava and passable rubble counted as walls only pass-wall and kill-wall monsters could enter. With ==, a blacklock mage one step behind passable rubble raises the danger at the Borg's grid from 0 to 208 (equal to the open-floor exit), and a level-12 Warrior flees with Word of Recall instead of taking no precaution; the open-floor and granite controls are byte-identical.

One synthetic pocket, one monster and one caution decision; the Borg's later turns are not played out. Measured: the danger value, the caution decision and the real scroll read. Not measured: how often fast casters sit behind passable rubble or lava in generated levels, or whole-game survival. Lava is affected the same way: after the change it counts as a grid any monster can step onto (as borg_cave_floor_grid() already treats it), although in the game only fire-immune monsters enter lava; no lava case was run. The Borg's own projection check treats passable rubble as transparent although the game's rubble blocks bolts; that is unchanged.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
