# The Borg's Lunal mode walked to items it had marked as not worth taking

This is **lunaltake-73 / issue73**, a C Borg defect
(diegoami/neo-angband-tracker#73).

In Lunal mode the Borg is leaving the level and only wants a few things on the way: gold, artifacts, certain potions and, at the start of the game, valuable items. The take routine works out for each visible item whether it is bad and notes the ones it wants, but the result was never used, so every visible item within range became a target. The correction skips the items marked bad.

## What to watch

The Borg walks toward the item (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `45423c1c8`) |
| --- | --- | --- |
| A dagger lies four grids away | failed, walks_to_item=1 | passed, walks_to_item=0 |
| Control: a pile of gold lies four grids away | passed, walks_to_item=1 | byte-identical log |

- **A dagger lies four grids away.** A level-12 Warrior with a free pack slot sees a Dagger four grids east. The production Lunal take routine decides whether to go for it. Original: the routine marks the dagger bad (it is not gold, an artifact, a stat potion or a valuable start-of-game item) but never skips it, so the Borg flows to it. Corrected: the dagger is skipped and the routine does nothing.

- **Control: a pile of gold lies four grids away.** The same Warrior with a pile of gold in the same place. Gold is marked good and the Borg walks to it before and after.

## Conditions

Native Human Warrior, seed 73, character level 12, depth 5, a lit open arena, a free pack slot. The item is placed by the fixture in the Borg's object list four grids east, in view. Fixture interventions: the object list entry is placed by the fixture (kind, position, value 100); the Borg's map update is not run, so the detection arrays are sized for a two-by-two panel map as it would. The production borg_flow_take_lunal() decides; its keypress is read, not played.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lunaltake-73
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lunaltake-73-evidence
git show a2184b5d56d7a92cb8754335932367e2ce4b0f3a:docs/audit/c-reports/lunaltake-73/reproduce.py > /tmp/reproduce-lunaltake-73.py
python3 /tmp/reproduce-lunaltake-73.py --full-tests --fixtures-ref a2184b5d56d7a92cb8754335932367e2ce4b0f3a --fix-ref 45423c1c8d99bc7846dc8503fa0a2be0e5c7e149
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-take.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `45423c1c8d99bc7846dc8503fa0a2be0e5c7e149` (branch `task/lunaltake-73`), fixtures
  `a2184b5d56d7a92cb8754335932367e2ce4b0f3a` (branch `task/lunaltake-73-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: gold
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With a dagger four grids away the original Lunal take routine flowed to it and queued a step; with the items marked bad skipped it does nothing. A pile of gold in the same place is taken before and after, and that control is byte-identical.

Priority low to medium: this measures the take routine's answer in one scene, not what Lunal mode does over a level. Which items count as wanted is the routine's existing rule (gold, artifacts, a range of potions, valuable items at the start); the fix does not change it. The item is placed in the Borg's list by the fixture.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
