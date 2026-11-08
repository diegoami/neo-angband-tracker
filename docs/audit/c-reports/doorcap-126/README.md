# Remove an unreachable door-count test from breeder-level door closing

This is **doorcap-126 / issue #126**, a C Borg defect
(diegoami/neo-angband-tracker#126).

Cleanup, no behaviour change; owner decision. On levels with breeding monsters the Borg closes doors to contain them: before each step, borg_play_step() looks for an open door next to it and closes it, and notes closed doors in a list (track_door). The scan skipped every door when the list held 255 or more entries, under the comment 'Skip repeatedly closed doors'. The list has room for 100 entries and only grows while it is below that size, so the test is never true (and it counts the whole list, not the door being looked at). The change removes the test and its comment. Neither 'skip doors already listed' nor a cap is implemented.

## What to watch

Borg_play_step() closed a door ('c' queued; 0 = stepped instead); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `f6f0d37cd`) |
| --- | --- | --- |
| Control: breeder level, one open door north | passed, close=1 | byte-identical log |
| Control: breeder level, open doors north and south | passed, close=1 | byte-identical log |
| Control: breeder level, door list full (100 of 100) | passed, close=1 | byte-identical log |
| Control: not a breeder level, one open door north | passed, close=0 | byte-identical log |

- **Control: breeder level, one open door north.** Both versions close the door north (goal (10,11), 'c' queued) instead of stepping east, and note it in the door list (0 to 1). Logs byte-identical.

- **Control: breeder level, open doors north and south.** Both versions close the last door scanned, the one south (10,13), and note it. Logs byte-identical.

- **Control: breeder level, door list full (100 of 100).** The list is at its real limit of 100, below the removed test's 255: both versions still close the door north, and neither can note it (the list stays at 100). Logs byte-identical.

- **Control: not a breeder level, one open door north.** Without the breeder flag the door scan is skipped and both versions step east (key 6). Logs byte-identical.

## Conditions

Native Dwarf Rogue, seed 126, clevel 25, depth 5, full HP, a lantern, in a lit, open 24x40 arena. The Borg's map is seeded from the real cave; the columns from x=30 eastward are unknown, so the production borg_flow_dark(false) commits a route east and takes its first step through borg_flow_old() and borg_play_step(). 'onedoor': an open door directly north of the player. 'twodoors': open doors north and south. 'full': one door north, with the door list already at its capacity of 100 entries (recorded at other grids). 'nobreeder': one door north, not a breeder level. The first queued key, the goal grid and the door list are read back. Fixture interventions: the arena, doors and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_MARK, borg.avoidance is set to current HP, the breeder flag (borg.near.breeder, set by borg_check_breeders() when three breeders are known) is set directly, and in 'full' the door list is filled directly.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/doorcap-126
git fetch https://github.com/diegoami/neo-angband-tracker.git task/doorcap-126-evidence
git show 65041720c259005b9ae1f88009377f94d5f82d7c:docs/audit/c-reports/doorcap-126/reproduce.py > /tmp/reproduce-doorcap-126.py
python3 /tmp/reproduce-doorcap-126.py --full-tests --fixtures-ref 65041720c259005b9ae1f88009377f94d5f82d7c --fix-ref f6f0d37cd6173f88b3132f5c038a222a2a449f1a
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `f6f0d37cd6173f88b3132f5c038a222a2a449f1a` (branch `task/doorcap-126`), fixtures
  `65041720c259005b9ae1f88009377f94d5f82d7c` (branch `task/doorcap-126-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: onedoor, twodoors, full, nobreeder
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The removed test in borg_play_step() is unreachable: track_door is created with 100 entries (borg_init_track(&track_door, 100)), the only place that raises track_door.num does so while it is below track_door.size, and every other write resets it to 0. Four native scenarios that run borg_play_step() through the production dark flow, three on a breeder level (one door, two doors, the door list full at 100) and one off it, give byte-identical logs before and after, and both full native gates pass on both builds.

This is a cleanup with no failing mode: the scenarios show unchanged behaviour, they cannot show a difference. Unreachability is argued from the source (the list size and the single guarded increment). Not measured: whole-game runs on breeder levels, or how often a level has more than 100 doors. What the comment intended (skip doors already listed, or a cap) is not implemented.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
