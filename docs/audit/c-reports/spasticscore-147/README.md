# The secret-door search judged a grid by its north-west neighbour's search count

This is **spasticscore-147 / issue147**, a C Borg defect
(diegoami/neo-angband-tracker#147).

When a bored Borg has nothing left to explore it searches grids next to walls for secret doors. It remembers how often each grid has been searched (every search counts for the nine grids around the Borg) and gives up on a grid once it has been searched enough. When scoring a candidate grid, the count it subtracted was not the candidate's own: it was the count of the candidate's north-west neighbour, left over from the neighbour checks just above. So a grid that had never been searched was refused because its north-west neighbour had been, and a grid already searched three times was searched again because its north-west neighbour had not been. The correction uses the candidate grid's own count.

## What to watch

Searches recorded at P (grid under test); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `6e54def84`) |
| --- | --- | --- |
| P never searched; its north-west neighbour searched three times | failed, searches_at_P=0 | passed, searches_at_P=3 |
| P already searched three times; its north-west neighbour never | failed, searches_at_P=1 | passed, searches_at_P=0 |
| Control: the Borg searches on P from the start | passed, searches_at_P=3 | byte-identical log |

- **P never searched; its north-west neighbour searched three times.** Searching two grids west raised the count of P's north-west neighbour to 15 but left P at 0. Original: P is scored with the neighbour's 15, comes out negative and is never searched (0). Corrected: P is scored with its own 0 and searched three times, exactly like the control.

- **P already searched three times; its north-west neighbour never.** Searching one grid east raised P's count to 15 but not its north-west neighbour's. Original: P is scored with the neighbour's 0 and searched a fourth time (count 20). Corrected: P's own 15 makes the score negative; no further search.

- **Control: the Borg searches on P from the start.** P and its north-west neighbour are always searched together, so the two counts are equal: both versions search P three times and the logs are identical.

## Conditions

Native Human Warrior, seed 147, level 20, depth 5, alone in a closed 22x22 room with a granite north wall, no stairs or monsters, just arrived on the level (level timer 0). The grid under test P is (10,2), against the north wall. 'fresh': the Borg first searches at (8,2), two grids west, until that grid is used up (three searches), then is moved to P. 'searched': the same from (11,2), one grid east. Control 'plain': the Borg starts on P. At each position the production borg_flow_spastic(true) (the bored call) runs eight times in a row and each call reports whether it recorded a search at the Borg's grid. Fixture interventions: the granite north wall (the arena helper builds permanent rock) and the move to P (monster_swap; the walk is not under test). On the pristine source the search can only choose the Borg's own grid (issue #146), which is why the Borg is moved between positions.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/spasticscore-147
git fetch https://github.com/diegoami/neo-angband-tracker.git task/spasticscore-147-evidence
git show 5b38022eb2697a8f8ea02732386a59919d8dae26:docs/audit/c-reports/spasticscore-147/reproduce.py > /tmp/reproduce-spasticscore-147.py
python3 /tmp/reproduce-spasticscore-147.py --full-tests --fixtures-ref 5b38022eb2697a8f8ea02732386a59919d8dae26 --fix-ref 6e54def846e04ee7a6a203bfcb1623128fc7d696
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-misc.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `6e54def846e04ee7a6a203bfcb1623128fc7d696` (branch `task/spasticscore-147`), fixtures
  `5b38022eb2697a8f8ea02732386a59919d8dae26` (branch `task/spasticscore-147-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: plain
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_flow_spastic() subtracts ag->xtra * 40 from a candidate's score after ag was reused by the neighbour loops, so it reads the north-west neighbour's search count. A never-searched grid next to a searched one was refused (0 searches instead of 3); a grid searched three times was searched a fourth time. Scoring with borg_grids[y][x].xtra fixes both; the control, where the counts coincide, is byte-identical.

On the pristine source the spastic search cannot choose any grid but the Borg's own (issue #146, draft fix PR #426), and choosing its own grid queues no key: in this source the slip changes which grids the Borg records as searched and how many bored calls it spends there, not a game action. Once #146 is corrected the same score ranks the grids the Borg walks to, so the slip would steer its walks; that combination was not measured here. In 4.2 secret doors are found by walking next to them, so no door-finding outcome is claimed.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.

## Stacked with #146 (2026-11-08)

Owner decision on #147: keep the standalone pair above, and add a run with
both corrections applied to measure which grid the Borg actually walks to.
On the pristine base the search can only choose the Borg's own grid (#146),
so #147's score has no visible move there. **#147's visible effect depends
on #146** (source PR #426); it should be filed after, or together with, #146.
The standalone run above and its pins stay valid; this section adds a
separate run in [`stacked/`](stacked/) and supersedes nothing.

**Setup.** Same Warrior (seed 147, level 20, depth 5) in a closed 22x70
room with a granite north wall (wide, so the corners, which score higher,
stay beyond the search's reach at level 20). One disposable worktree builds
three variants of `src/borg/borg-flow-misc.c`: the pristine base
(reference), base + #146's hunk, and base + #146's + #147's hunks. The runner
asserts that `task/spastic-146` (`f97311e5d`) and `task/spasticscore-147`
(`6e54def84`) are each exactly their expected hunk in that one file, and
neither source branch was changed. In each build the production
`borg_flow_spastic(true)` runs repeatedly; the fixture carries out each step
the call queues (`monster_swap`) and, before each call, blanks the committed
flow so that the grid at flow cost 0 afterwards is the grid the call chose.

| Mode | Pristine base | #146 only | #146 + #147 |
| --- | --- | --- | --- |
| `wall`: starts at (35,2) against the wall, 12 bored calls | searches its own grid 3 times, never moves (0/1) | after the first search picks **(34,2)**, already covered (count 5); creeps west one grid per search: 6 searches, **5 on covered grids**, ends at (30,2) (0/1) | picks **(37,2)**, not covered (count 0); searches (35,2), (37,2), (39,2), (41,2): 4 searches, **none repeated**, ends at (42,2) (1/1) |
| `approach` (control): starts at (35,5), stops at the first search | finds no grid, never moves (0/1) | walks to (38,2), searches once (1/1) | byte-identical to #146 only |

Why: after the search at (35,2) the counts of (34..36, 1..3) are 5. With
#146 alone each candidate is scored with its north-west neighbour's count, so
(34,2) is scored with (33,1)'s 0 and wins at distance 1. With #147 too,
(34,2) and (36,2) carry their own 5 and the nearest uncovered grid wins. In
the control no grid has been searched, so both counts are 0 everywhere.

Gates: 965/965 unit checks and 5/5 frontend tests for each of the three
builds. Replay [`stacked/replay.html`](stacked/replay.html): generated script
passes `node --check`; Playwright Chromium headless `render-check.js` (12
checks) and `render-smoke.js` (6 actions), no console errors.
[`stacked/receipt.json`](stacked/receipt.json) holds the sha256 of every file
in `stacked/`; all phase logs are committed there.

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/spastic-146
git fetch https://github.com/diegoami/neo-angband-tracker.git task/spasticscore-147
git fetch https://github.com/diegoami/neo-angband-tracker.git task/spasticscore-147-evidence
git show 81fe464dc50445b5eb1c6758c3bc4773d9972d5d:docs/audit/c-reports/spasticscore-147/stacked/reproduce.py > /tmp/reproduce-spasticscore-147-stacked.py
python3 /tmp/reproduce-spasticscore-147-stacked.py --full-tests --fixtures-ref 81fe464dc50445b5eb1c6758c3bc4773d9972d5d --fix146-ref f97311e5d52de4dda8d75058114c52acbdac2674 --fix147-ref 6e54def846e04ee7a6a203bfcb1623128fc7d696
```

**Proves:** with #146 applied, #147's slip decides where the Borg walks: it
steps onto grids its last search already covered and searches each of them
again; the corrected score sends it to uncovered grids. **Does not prove:**
anything about a real dungeon level, other classes or levels, how often a
Borg is bored next to a wall in play, or that a secret door is found (in
4.2 doors are found by walking next to them). The fixture carries out the
queued steps itself; the game loop is not under test.

The appended section changes this README's bytes; `receipt.json`'s
`README.md` hash is updated to follow it (old and new values recorded in
`stacked/receipt.json`), and no other file of the standalone run changed.
