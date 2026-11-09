# One unidentified item made a full pack look empty

This is **countsell-134 / issue134**, a C Borg defect
(diegoami/neo-angband-tracker#134).

A low-level Borg with a pack full of things to sell should stop going deeper and head back to town to sell them. It decides by counting the sellable items in its pack. When the count reached an unidentified item that was not part of a stack, it gave up and reported 0, instead of skipping that one item. A level-4 Warrior carrying thirteen spare swords and one unidentified mace therefore counted nothing to sell and took the down staircase. The correction skips the unidentified item and keeps counting; the Borg then declines to go down.

## What to watch

Took the down staircase (1 = flowed down); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `026a42d3f`) |
| --- | --- | --- |
| 13 spare swords plus one unidentified mace | failed, went_down=1 | passed, went_down=0 |
| Control: 13 spare swords, nothing unidentified | passed, went_down=0 | byte-identical log |
| Only the unidentified mace | passed, went_down=1 | passed, went_down=1 |

- **13 spare swords plus one unidentified mace.** Original: the count stops at the unidentified mace and returns 0, so the 'need to sell, don't go down' check passes and the Borg flows to the down staircase (key 54). Corrected: 15 sellable items counted; the Borg does not go down.

- **Control: 13 spare swords, nothing unidentified.** Both versions count 15 sellable items and decline to go down; the logs are identical.

- **Only the unidentified mace.** Little to sell in either version (0 before, 2 after); both go down. The count line differs, the decision does not.

## Conditions

Native Human Warrior, seed 134, level 4, depth 1, no gold, a torch and rations, in a lit room with a down staircase three steps east and an up staircase far west, both tracked by the Borg. 'full': thirteen different spare swords (fully known). 'full-unid': the same plus a Mace (Holy Avenger) whose runes are unknown. 'light': only the unidentified mace. The production borg_count_sell() runs, then the production borg_flow_stair_more() (the down-stair flow used when leaving a level) runs once and the first queued key is read back. This isolates the selling count and the stair decision; it does not play the trip to town.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/countsell-134
git fetch https://github.com/diegoami/neo-angband-tracker.git task/countsell-134-evidence
git show 68b23b2a1b2eeb94d70a86e98375aaddaadf0951:docs/audit/c-reports/countsell-134/reproduce.py > /tmp/reproduce-countsell-134.py
python3 /tmp/reproduce-countsell-134.py --full-tests --fixtures-ref 68b23b2a1b2eeb94d70a86e98375aaddaadf0951 --fix-ref 026a42d3fcdfcbedaaef12c6f39200e38ecbd2ed
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-sell.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `026a42d3fcdfcbedaaef12c6f39200e38ecbd2ed` (branch `task/countsell-134`), fixtures
  `68b23b2a1b2eeb94d70a86e98375aaddaadf0951` (branch `task/countsell-134-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: full
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_count_sell() returned 0 at the first unidentified non-stacked item. A level-4 Warrior with 13 spare swords and one unidentified mace took the down staircase (count 0) instead of going to sell (count 15). Skipping the item restores the count and the decision. The control without unidentified items is byte-identical; with only the unidentified mace both versions still go down.

Not measured: the trip back to town or the sale itself, and how often a pack holds an unidentified non-stacked item (common in early play). The correction also means the final borg_notice() in borg_count_sell() now always runs, so the Borg's traits are no longer left as computed for its last trial removal; that side effect was not separately measured.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
