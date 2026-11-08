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


## Re-verification with a control (2026-11-08)

Why: the original run's phase logs (configure, builds, full gates) were not
committed (the repository's `*.log` ignore rule dropped them), the receipt had
no sha256 map, the replay render check was only planned, and the claim that the growth above level 10 is unchanged was not tested.

A control mode was added (`SR245_MODE=clevel11`). Munchkin mode only runs below maximum character level 12 (`borg_think_dungeon()`: `borg_cfg[BORG_MUNCHKIN_START] && borg.trait[BI_MAXCLEVEL] < 12`), so level 11 is the only level above the new gate where the adjustment can run. There the adjustment really increases the range: 8 + (11 − 10) + (10 − 10) = 9. The known up staircase is placed 8 grids away, and the munchkin stair test is `b_j < closeness`, so only the grown range counts it as close (8 < 9; the base 8 would not). Both versions power-climb with a byte-identical log, which shows the growth is still applied after the fix.

**New fixtures pin `4a93196b26631b1a36a6468226a26d2ccb3d25b0` supersedes `3c9a75e2023386863db8a21be16db5b8c12a840a`** for
every repeat command. It adds only the mode selection and the new case(s); the
main case's code path is unchanged, and the run at the new pin reproduced the
recorded main outcome exactly (the `Failed` line of the original's main log
now cites a later source line, the only difference in that log). Fix
`c16b3e9d14470dd19ef884f37f9eca2f7cc7d37f`, base `e473720ed`.

| Case | Original (base) | Corrected (fix) | Logs |
| --- | --- | --- | --- |
| `main`: level 5, depth 10, stair 5 grids away | climb_decision **0** (range 3, "Any Stair"), 0/1 | climb_decision **1** (range 8, "Power Climb"), 1/1 | `before-main.log`, `after-main.log` |
| `clevel11` (control): level 11, depth 10, stair 8 grids away | climb_decision 1 (range 9), 1/1 | climb_decision 1, 1/1 | **byte-identical** `before-clevel11.log` / `after-clevel11.log` |

Full native gates on both variants: 965/965 unit checks and 5/5 frontend tests
(parsed from `reverify-2026-11-08/before-alltests.log` and `after-alltests.log`,
no failed lines). Toolchain: GNU 13.3.0, cmake version 3.28.3.
Render check: `node tools/c-evidence/render-smoke.js` (Playwright headless) on
the committed `replay.html`: `RENDER-SMOKE actions=0 errors=0 OK` (no console errors, no
`undefined`/`NaN`); the page now has a section per extra case with both raw logs.

The top-level `before-*.log`, `after-*.log`, results files, `replay.html` and
`replay-data.json` are from this run. Configure, build, scenario-runner and
full-gate logs and `result.json` are in [reverify-2026-11-08/](reverify-2026-11-08/)
with their own [receipt](reverify-2026-11-08/receipt.json). The root
`receipt.json` now records this run (the earlier pin and outcomes are kept
under `superseded_run`) and a sha256 map of every file in this directory.

Repeat, with both shas pinned:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairrange-245
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairrange-245-evidence
git show 4a93196b26631b1a36a6468226a26d2ccb3d25b0:docs/audit/c-reports/stairrange-245/reproduce.py > /tmp/reproduce-stairrange-245.py
python3 /tmp/reproduce-stairrange-245.py --full-tests --fixtures-ref 4a93196b26631b1a36a6468226a26d2ccb3d25b0 --fix-ref c16b3e9d14470dd19ef884f37f9eca2f7cc7d37f
```
