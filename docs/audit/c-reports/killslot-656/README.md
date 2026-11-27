# The full monster tracker can allocate outside its active slot range

This is **killslot-656 / issue #656**, a C Borg defect
(diegoami/neo-angband-tracker#656).

When the Borg's monster table is full (borg_kills_nxt == 255, active slots 1..254), borg_new_kill() stole a victim with randint1(borg_kills_nxt - 1) + 1, which yields 2..255. Slot 255 is allocated storage that the tracking scans (i < borg_kills_nxt) never visit: a monster placed there is never refreshed or forgotten, and because the deletion of the still-empty slot returns before decrementing, the active count overstates by one. The same off-by-one made slot 1 unreachable as a victim.

## What to watch

The active monster count after the full-table steal (capacity 254; 255 means the count overstates); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `574b618c3`) | Corrected (fix `2939fb2d9`) |
| --- | --- | --- |
| The upper endpoint of the steal draw (pinned) | failed, count=255 | passed, count=254 |
| The lower endpoint of the steal draw (pinned) | failed, count=254 | passed, count=254 |
| Control: ordinary allocation on an unfilled table | passed, count=1 | byte-identical log |

- **The upper endpoint of the steal draw (pinned).** Slots 1..254 are filled with live records and a real, visible monster forces the steal with the upper-endpoint draw pinned (seed 551; both variants consume the identical draw). The original steals slot 255 - outside the active range, invisible to every scan, and the count inflates to 255. The corrected borg steals slot 254, the last active slot; the count stays at capacity and the map link resolves.

- **The lower endpoint of the steal draw (pinned).** The same full table with the lower-endpoint draw pinned (seed 305). The original maps it to slot 2 - slot 1 is never reachable as a victim. The corrected borg maps it to slot 1, the first active slot.

- **Control: ordinary allocation on an unfilled table.** A fresh table allocates the new monster at slot 1 in both variants; the complete logs are byte-identical.

## Conditions

Native Human Warrior, level 20 (experience forced), depth 5 in a synthetic open lit arena, a wooden torch as the only light. The borg arrives through the production borg_update() pass; then slots 1..254 are filled with live fake kill records (a real race, positions on the arena, seen this turn, grid links set), borg_kills_nxt = 255 and the count 254. A real visible floating eye is then added and the production borg_update() creates it, forcing the steal. The RNG is reseeded immediately before that update so the single steal draw is pinned per mode; the seed scan mode documents the choice. The tracked record is located by race and position, the production scan range by borg_kills_nxt, and the map link through borg_grids.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario to see the original and corrected runs side by side on the dungeon map, drawn with the game's own symbols (the legend under the maps explains them), press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/killslot-656
git fetch https://github.com/diegoami/neo-angband-tracker.git task/killslot-656-evidence
git show c0f33f9e5d523d0e2caca60a05cf38351f1050f8:docs/audit/c-reports/killslot-656/reproduce.py > /tmp/reproduce-killslot-656.py
python3 /tmp/reproduce-killslot-656.py --full-tests --fixtures-ref c0f33f9e5d523d0e2caca60a05cf38351f1050f8 --fix-ref 2939fb2d914cc31595a568fd52f9236982676081
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-kill.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix `2939fb2d914cc31595a568fd52f9236982676081` (branch `task/killslot-656`), fixtures
  `c0f33f9e5d523d0e2caca60a05cf38351f1050f8` (branch `task/killslot-656-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: ordinary
- full native gates: before 966/966 unit checks and 5/5 frontend tests;
  after 966/966 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

A full-table steal now picks its victim inside the active range 1..borg_kills_nxt-1: the new monster always lands in a slot the tracking scans visit, its map link resolves, the active count stays at capacity, and slot 1 is eligible as a victim. On the base commit the pinned upper-endpoint draw allocated slot 255 - a record no scan ever refreshes or forgets, with the count overstated at 255 - and the pinned lower-endpoint draw could never steal slot 1. Ordinary non-full allocation is unchanged (byte-identical control).

The fixture measures the allocation endpoint and the immediate table invariants (scan visibility, map link, count). The stale record the original leaves at slot 255 is never refreshed or forgotten by design of the bug; its downstream effects (phantom danger at a stale position, a goal flowing to a monster that is no longer there) follow from the source and are not replayed here. The pinned seeds are documented in the fixture (both variants consume the identical draw; only the mapping differs).

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
