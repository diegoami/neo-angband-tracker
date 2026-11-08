# The shrunk stair range stopped the munchkin climb

This is **stairrange-245 / issue245**, a munchkin-mode decision defect. The
munchkin stair-range adjustment adds `(clevel - 10) + (10 - depth)` to the
base range of 8 whenever the depth is at most 10. Below character level 10
the first term is negative, so the "increase" shrinks the range instead: a
level-5 Borg at depth 10 gets range 3, a level-1 Borg range −1. With the
shrunk range no staircase counts as close, and the munchkin Borg stops
power-climbing toward stairs it can see — it falls back to the generic
any-stair flow. (The formula's design point is clevel ≥ 10, where the range
does grow.)

Per the owner's decision, the correction gates the adjustment on
`clevel > 10`: below level 10 the range returns to the base 8; at level 10
and above the previous growth is unchanged.

## What to watch

| Version | Climb decision (`borg.goal.less`) | Borg note trail |
| --- | --- | --- |
| Original | **0** — no staircase counts as close | "Munchkin Mode. Any Stair." (generic fallback) |
| Corrected | **1** — power-climb toward the visible staircase | "Munchkin Mode. Power Climb." → "Looking for stairs. Munchkin Mode." |

Fixture: Native Human Warrior, seed 245, level 5, depth 10, no monsters, a
known up staircase five grids north (outside the shrunk range of 3, inside
the base range of 8). The borg's tracked staircase list is seeded to match a
viewed staircase; the production munchkin decision runs once.

[replay.html](replay.html) is the offline page for the captured run.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairrange-245
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairrange-245-evidence
git show FETCH_HEAD:docs/audit/c-reports/stairrange-245/reproduce.py > /tmp/reproduce-stairrange-245.py
python3 /tmp/reproduce-stairrange-245.py --full-tests --fix-ref c16b3e9d14470dd19ef884f37f9eca2f7cc7d37f
```

The first fetch brings the fix commit; the second sets `FETCH_HEAD` to the
evidence revision. `--fix-ref` is required: the runner pins the corrected
source by that commit and asserts the correction is exactly the range gate in
the single touched file.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `c16b3e9d` (branch
  `task/stairrange-245`), fixtures `3c9a75e2` (branch
  `task/stairrange-245-evidence`)
- before: suite 0/1 — `goal_less=0` at clevel 5 / depth 10
- after: suite 1/1 — `goal_less=1`
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Chrome headless

## Scope and limits

The fixture isolates the range arithmetic and the climb decision for one
configuration (level 5, depth 10, stair at distance 5). It does not measure
autonomous play. Owner decision recorded on the issue: only apply the
adjustment when it increases; levels 10 and above keep the previous growth.
