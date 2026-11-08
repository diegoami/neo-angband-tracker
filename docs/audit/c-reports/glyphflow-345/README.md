# The Sea of Runes stalled after its centre rune

This is **glyphflow-345 / issue345**, an endgame-preparation defect in the
C Borg.

Before fighting Morgoth the Borg builds a *Sea of Runes*: a 5x5 block of
Runes of Protection with itself standing on the centre. Each time it picks
the next rune grid, it first measures how far every grid is from where it
stands (a "reverse flow" from its own grid), then skips grids it cannot
reach. That measurement was requested with *optimization*, and an optimized
spread stops as soon as the next distance (1) is larger than the starting
grid's distance (0). So only the Borg's own grid was ever measured.

While the centre is being chosen that does not matter — the Borg chooses the
grid it stands on. But once the centre rune is laid and the Borg stands on
it, the centre is excluded from the outlying-rune choice, and no other grid
looks reachable: the Borg finds nothing to do, and the Sea makes no further
progress while it stands there.

The correction measures the neighbourhood without optimization — what the
reverse-flow helper's own comment ("do NOT optimize") intends:

```c
borg_flow_reverse(250, false, false, false, -1, false);
```

## What to watch

| Scenario | Original | Corrected |
| --- | --- | --- |
| On the laid centre rune | two further calls return false, no key queued (progress **0**) | flows toward a ring grid at distance 1, queues a step (key 51) (progress **1**) |
| Control: on an unglyphed ring grid | chooses its own grid (distance 0), reads the next rune | byte-identical log |

The first two calls (choose the centre, arrive and read the centre rune)
behave identically in both versions; they are part of both logs.

Fixture: Native Human Warrior, seed 345, level 50, depth 99, a full-size
open level (198x66; the Sea scan only looks 15 rows / 50 columns inside the
edges), 40 known Rune of Protection scrolls. The production
`borg_flow_glyph()` runs as the Borg calls it; after the read on arrival the
fixture lays the rune on that grid (the result of the read) in the game map
and the Borg's map.

[replay.html](replay.html): choose a scenario, then **Show decision** and
**Show raw logs**.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glyphflow-345
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glyphflow-345-evidence
git show FETCH_HEAD:docs/audit/c-reports/glyphflow-345/reproduce.py > /tmp/reproduce-glyphflow-345.py
python3 /tmp/reproduce-glyphflow-345.py --full-tests --fix-ref f618381294dcf5bff1f0bc95f0fbe259412a53b9
```

The runner asserts the correction is exactly the unoptimized reverse flow
(plus its comment) in the single touched file, checks each expected result,
that the control log is byte-identical, runs both full native gates and
writes `replay.html`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `f61838129` (branch
  `task/glyphflow-345`), fixtures `fbe06401c` (branch
  `task/glyphflow-345-evidence`); the native run executed at the fixtures
  pin, the later commit only adds these captured artifacts
- before: centre case 0/1 (`progress=0`); control 1/1
- after: centre case 1/1 (`progress=1`, step key 51 toward distance 1);
  control byte-identical
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Playwright Chromium headless, no console errors

## Scope and limits

The fixture isolates the outlying-rune choice from the laid centre. It does
not build a whole Sea of Runes or fight Morgoth. Independent consumer
evidence (borg-rs, linked on the issue) reports that with this correction a
constructed level-50 character completes a 25-rune Sea autonomously where
the original did not within 500 turns; that run is not part of this
package. The same optimized reverse flow is used by the spastic
secret-door search (issue146), tracked and corrected separately.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`
(+ `profile/borg.txt`) are the generators. `before-*.log`, `after-*.log`,
`*-results.json`, `result.json`, `run-*.log`, `replay.html`,
`replay-data.json` and `receipt.json` (hashes of every file) are the
captured run.
