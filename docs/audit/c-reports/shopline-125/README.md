# The Borg walked onto the door of the wrong shop on its way to another

This is **shopline-125 / issue125**, a C Borg defect
(diegoami/neo-angband-tracker#125).

Stepping onto a shop's entrance enters that shop, so when the Borg flows to a shop it is meant to keep off every other shop's entrance. The flow skipped such an entrance only when it was in neither the Borg's row nor its column (the test joined the two comparisons with 'and' where 'or' was meant), so an entrance in line with the Borg stayed on the path. The correction skips every other shop entrance except the grid the Borg stands on.

## What to watch

The first step is onto the other shop's door (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `9d35babe8`) |
| --- | --- | --- |
| Another shop's door lies next to the Borg, in its row | failed, steps_onto_other_shop=1 | passed, steps_onto_other_shop=0 |
| Control: the other shop's door is off the Borg's row and column | passed, steps_onto_other_shop=0 | byte-identical log |

- **Another shop's door lies next to the Borg, in its row.** A level-12 Warrior in town heads for the General Store 20 grids east in its row. The Armoury door is the next grid east, also in the row. The production shop-entry flow takes the first step. Original: the flow keeps the Armoury door on its path and the first step is east, onto it, entering the Armoury. Corrected: the door is avoided and the first step is south-east, around it, at the same total cost.

- **Control: the other shop's door is off the Borg's row and column.** The same Warrior with the Armoury door two rows above the line. The old test already skipped it, so the first step is east, before and after.

## Conditions

Native Human Warrior, seed 125, character level 12, town (depth 0), a lit open arena. The General Store entrance is the goal, 20 grids east of the Borg in its row; the Armoury entrance is next to the Borg in the same row ('inline') or two rows above ('offrow'). Fixture interventions: the two shop entrances are placed by the fixture in the game map and in the Borg's map entries and shop-location tables; the Borg's map update is not run. The production borg_flow_shop_entry() takes the step; the keypress is read, not played.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/shopline-125
git fetch https://github.com/diegoami/neo-angband-tracker.git task/shopline-125-evidence
git show 330deefd6caa6e7fe194d5a00c8d05fc2dd0afeb:docs/audit/c-reports/shopline-125/reproduce.py > /tmp/reproduce-shopline-125.py
python3 /tmp/reproduce-shopline-125.py --full-tests --fixtures-ref 330deefd6caa6e7fe194d5a00c8d05fc2dd0afeb --fix-ref 9d35babe8bd8e38b2d522e00ce159d740919718f
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `9d35babe8bd8e38b2d522e00ce159d740919718f` (branch `task/shopline-125`), fixtures
  `330deefd6caa6e7fe194d5a00c8d05fc2dd0afeb` (branch `task/shopline-125-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: offrow
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With another shop's door next to the Borg in its row, the original flow stepped east onto it and entered the wrong shop; with every other door avoided the first step goes round it (south-east) at the same path cost. A control with the door off the Borg's row is byte-identical.

Priority low to medium: this measures the first step in one constructed town scene, not a shopping trip. The town layout (a door in line with the Borg on the way) is common in the real town, but how often the Borg is in that position is not measured.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
