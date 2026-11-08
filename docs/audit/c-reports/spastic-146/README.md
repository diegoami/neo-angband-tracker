# The secret-door search never left its own grid

This is **spastic-146 / issue146 (CBUG-0130)**, an exploration defect in the
C Borg.

When a bored Borg has nothing left to explore it runs a "spastic" search: it
looks for grids next to walls where a secret door might be found and walks
to the best one. To compare candidates it first measures how far every grid
is from where it stands (a "reverse flow" from its own grid). That
measurement was requested with *optimization*, and an optimized spread stops
as soon as the next distance (1) is larger than the starting grid's distance
(0). So only the Borg's own grid was ever measured, and the search could
only choose the grid it already stood on:

- from the middle of a room it found no candidate and never moved;
- standing next to a wall, it searched that grid and then kept choosing the
  same grid instead of moving on.

The correction measures the neighbourhood without optimization — what the
reverse-flow helper's own comment ("do NOT optimize") intends:

```c
borg_flow_reverse(250, false, false, false, -1, false);
```

## What to watch

Three consecutive bored calls `borg_flow_spastic(true)` from the same grid:

| Scenario | Original | Corrected |
| --- | --- | --- |
| Middle of a 22x22 room | all three return false, no key (moved **0**) | flows toward a wall grid at distance 10, step key 51 (moved **1**) |
| North-west corner | call 1 picks the corner, call 2 searches it, call 3 picks it again (moved **0**) | calls 1–2 identical, call 3 walks toward the next wall grid at distance 21 (moved **1**) |
| Control: full-size room, every wall beyond the level-20 search range (25) | nothing found | byte-identical log |

Fixture: Native Human Warrior, seed 146, level 20, depth 5, alone in a
closed granite room with no stairs or monsters, just arrived on the level
(level timer 0). Lantern and food carried.

[replay.html](replay.html): choose a scenario, then **Show decision** and
**Show raw logs**.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/spastic-146
git fetch https://github.com/diegoami/neo-angband-tracker.git task/spastic-146-evidence
git show FETCH_HEAD:docs/audit/c-reports/spastic-146/reproduce.py > /tmp/reproduce-spastic-146.py
python3 /tmp/reproduce-spastic-146.py --full-tests --fix-ref f97311e5d52de4dda8d75058114c52acbdac2674
```

The runner asserts the correction is exactly the unoptimized reverse flow
(plus its comment) in the single touched file, checks each expected result
and that the control log is byte-identical, runs both full native gates and
writes `replay.html`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `f97311e5d` (branch
  `task/spastic-146`), fixtures `da9a39f4d` (branch
  `task/spastic-146-evidence`); the native run executed at the fixtures
  pin, the later commit only adds these captured artifacts
- before: middle 0/1, corner 0/1 (`moved=0`); control 1/1
- after: middle 1/1, corner 1/1 (`moved=1`); control byte-identical
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Playwright Chromium headless, no console errors

## Scope and limits

The fixture isolates the search choice over three calls. It does not run
until a secret door is found, and does not measure how often a real Borg
falls back to this search (it is the bored/stuck fallback in the dungeon
brain). In 4.2 secret doors are found by walking next to them, so a search
that never moves cannot find one. The same optimized reverse flow is used by
the Sea of Runes builder (issue345), tracked and corrected separately.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`
(+ `profile/borg.txt`) are the generators. `before-*.log`, `after-*.log`,
`*-results.json`, `result.json`, `run-*.log`, `replay.html`,
`replay-data.json` and `receipt.json` (hashes of every file) are the
captured run.
