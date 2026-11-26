# The Borg's light map lights one grid more than the game

This is **lightmap-644 / issue #644**, a C Borg defect
(diegoami/neo-angband-tracker#644).

The Borg keeps its own map of the grids its light source lights, and takes the light's strength (BI_LIGHT) as that map's radius. The engine lights one grid less: calc_lighting() builds radius = ABS(light) - 1 and add_light() lights every grid with distance() no greater than the radius, through line of sight. With a wooden torch the Borg therefore believes the ring two grids out is torch-lit when it is dark. A monster standing there is expected to be visible, is not, and the missing-monster pass deletes its record as one that did not really move - the Borg forgets a monster waiting just outside its light. The old radius-2 and radius-3 shapes are not the engine's either: they light (2,2), which is distance() 3, and skip (2,1). The correction collects the lit grids the way the engine does, at radius BI_LIGHT - 1 with line of sight; a Necromancer standing in unlight keeps the radius the Borg always used for that case.

## What to watch

Kill record kept; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `574b618c3`) | Corrected (fix `d8d31359e`) |
| --- | --- | --- |
| Monster two grids out in the dark ring: the record must be kept | failed, kept=0 | passed, kept=1 |
| Monster one grid out, seen: unchanged control | passed, kept=1 | byte-identical log |

- **Monster two grids out in the dark ring: the record must be kept.** A dead-end corridor, one grid wide, in the dark. The Borg, a level-20 Human Warrior with a wooden torch (BI_LIGHT 2, so the game lights distance 1), walked the corridor and stands two grids from its end. An awake soldier ant holds the last floor; the game cannot see it (no light, no infravision). The original marks the ant's grid BORG_LIGHT, judges the record viewable, and borg_follow_kill() deletes it in the missing-monster pass as a monster that did not really move. With the engine's radius the grid is dark, the record is not viewable, and it is kept.

- **Monster one grid out, seen: unchanged control.** The same corridor with the ant one grid east of the Borg, inside the real light. Both radii light the grid, the game sees the ant, the screen scan re-observes it, and the record is kept unchanged by both variants with byte-identical logs.

## Conditions

Native corridor fixture (24x40 arena sculpted into a one-grid-wide dead-end corridor, no SQUARE_GLOW), Human Warrior at character level 20, depth 5, wielding a Wooden Torch (BI_LIGHT 2, fuel 4000) as the only light, infravision 0; an awake soldier ant two grids east on the corridor's last floor (forget) or one grid east (adjacent control), noted via observe_kill_diff and aged one turn; the borg arrives on the level with one production borg_update() (new-level reset), then the measured borg_update() runs the screen scan, the view, the light map and the missing-monster pass with borg_do_update_view and borg_do_update_lite forced. Seed 644, borg_rand_local 1000. Measured: the kill record's survival and position, the light flags on the ant's grid, and the suite line. Not measured: the follow-on decisions the kept record would drive, further turns, and Necromancer unlight, which keeps the old radius by design.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario to see the original and corrected runs side by side on the dungeon map, drawn with the game's own symbols (the legend under the maps explains them), press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lightmap-644
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lightmap-644-evidence
git show 3df4d5cc99fdf705e48cfb70ba3dfab7b9c78ad3:docs/audit/c-reports/lightmap-644/reproduce.py > /tmp/reproduce-lightmap-644.py
python3 /tmp/reproduce-lightmap-644.py --full-tests --fixtures-ref 3df4d5cc99fdf705e48cfb70ba3dfab7b9c78ad3 --fix-ref d8d31359ec2ef9c73c9d9aa4b92579dc73a3b5fb
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-cave-light.c, src/borg/borg-light.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix `d8d31359ec2ef9c73c9d9aa4b92579dc73a3b5fb` (branch `task/lightmap-644`), fixtures
  `3df4d5cc99fdf705e48cfb70ba3dfab7b9c78ad3` (branch `task/lightmap-644-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: adjacent
- full native gates: before 966/966 unit checks and 5/5 frontend tests;
  after 966/966 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

A torch-carrying Human Warrior in a dark dead-end corridor with an awake soldier ant two grids east, invisible to the player, loses the ant's kill record on the original (deleted by the missing-monster pass after the light map marks the dark grid BORG_LIGHT) and keeps it with the engine's radius (BI_LIGHT - 1). The adjacent control, an ant one grid east the player sees, is kept by both variants with byte-identical logs, and both full native gates pass at 966/966 and 5/5 in each variant.



## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
