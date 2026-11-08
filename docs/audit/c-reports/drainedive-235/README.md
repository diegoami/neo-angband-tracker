# The drained Borg power-dived anyway

This is **drainedive-235 / issue235**, a leave-level decision defect. The
"Do not dive when drained" guard read `if (g && borg.trait[BI_ISFIXEXP])`,
but `g` is only non-zero at that point when an earlier branch has already
decided to head up. For a drained Borg that had not yet chosen a direction
the guard was dead: the power-dive branch ("playing too shallow") fired
anyway. The sibling "do not dive when full of items" guard was translated
from the original dive-cancellation to an unconditional
`try_not_to_descend`; this one kept the dead `g` gate.

The correction applies the drained guard unconditionally:

```c
if (borg.trait[BI_ISFIXEXP])
    try_not_to_descend = true;
```

matching the sibling guard and the original 2011 dive-cancellation shape
(`if (g && ...) g = 0`).

## What to watch

| Version | Power-dive notes (`dive_notes`) | Borg note trail |
| --- | --- | --- |
| Original | **1** — the drained Borg takes the dive | "# power dive, playing too shallow." |
| Corrected | **0** — the dive is suppressed while drained | (no dive note) |

Fixture: Native Human Warrior, seed 235, clevel 49, depth 15, no monsters.
Restock and preparedness are satisfied through depth 20 (fueled worn
lantern, rations, phase and teleportation scrolls, worn See Invisible and
Free Action rings — the aux bands for levels 10–20 require BI_SINV and
BI_FRACT), the static preparedness path is forced
(`BORG_USES_DYNAMIC_CALCS = FALSE`), and the drain is real (`exp = 0`
against `max_exp = 10000`, clevel ≠ 50, so `BI_ISFIXEXP` derives true and
stays true through the call). `borg_leave_level(false)` runs once; the
power-dive check is `borg_prepared(CDEPTH + 5)` = `borg_prepared(20)` ==
NULL, which the rings satisfy without touching the resist bands.

[replay.html](replay.html) is the offline page for the captured run.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/drainedive-235
git fetch https://github.com/diegoami/neo-angband-tracker.git task/drainedive-235-evidence
git show FETCH_HEAD:docs/audit/c-reports/drainedive-235/reproduce.py > /tmp/reproduce-drainedive-235.py
python3 /tmp/reproduce-drainedive-235.py --full-tests --fix-ref 6d909bf5216c1e6c5ed199595d9ed3e5baf4b0d8
```

The first fetch brings the fix commit; the second sets `FETCH_HEAD` to the
evidence revision. `--fix-ref` is required: the runner pins the corrected
source by that commit and asserts the correction is exactly the drained
guard in the single touched file.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `6d909bf5` (branch
  `task/drainedive-235`), fixtures on branch `task/drainedive-235-evidence`
- before: suite 0/1 — `dive_notes=1 drained=1` ("# power dive, playing too
  shallow.")
- after: suite 1/1 — `dive_notes=0 drained=1`
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Chrome headless

## Scope and limits

The fixture isolates the drained guard and the leave-level dive decision
for one configuration (clevel 49, depth 15, prepared through depth 20). It
does not measure autonomous play. The guard's purpose — keep a drained
Borg from diving — now applies regardless of any earlier direction choice.


## Re-verification with a control (2026-11-08)

Why: the original run's phase logs (configure, builds, full gates) were not
committed (the repository's `*.log` ignore rule dropped them), the receipt had
no sha256 map, the before/after scenario logs themselves were missing (only the results JSONs were committed), the replay render check was only planned, and there was no unchanged-situation control.

A control mode was added (`DD235_MODE=not-drained`): the same fully prepared Borg at depth 15 with full experience (`exp == max_exp`, `BI_ISFIXEXP` 0). Both versions take the power dive ("# power dive, playing too shallow.") with a byte-identical log, so the correction suppresses only the drained dive. The scenario logs of both cases are now committed at the top level.

**New fixtures pin `d948ef62041a1b26780bc8e08cf24a8f328f4f33` supersedes `557c85260f8b836ab2c8725fe8baa0910be38261`** for
every repeat command. It adds only the mode selection and the new case(s); the
main case's code path is unchanged, and the run at the new pin reproduced the
recorded main outcome exactly (the `Failed` line of the original's main log
now cites a later source line, the only difference in that log). Fix
`6d909bf5216c1e6c5ed199595d9ed3e5baf4b0d8`, base `e473720ed`.

| Case | Original (base) | Corrected (fix) | Logs |
| --- | --- | --- | --- |
| `main`: drained (exp 0 of max 10000), prepared, depth 15 | dive_notes **1** ("power dive, playing too shallow"), 0/1 | dive_notes **0**, 1/1 | `before-main.log`, `after-main.log` |
| `not-drained` (control): exp = max_exp | dive_notes 1, 1/1 | dive_notes 1, 1/1 | **byte-identical** `before-not-drained.log` / `after-not-drained.log` |

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
git fetch https://github.com/diegoami/neo-angband-tracker.git task/drainedive-235
git fetch https://github.com/diegoami/neo-angband-tracker.git task/drainedive-235-evidence
git show d948ef62041a1b26780bc8e08cf24a8f328f4f33:docs/audit/c-reports/drainedive-235/reproduce.py > /tmp/reproduce-drainedive-235.py
python3 /tmp/reproduce-drainedive-235.py --full-tests --fixtures-ref d948ef62041a1b26780bc8e08cf24a8f328f4f33 --fix-ref 6d909bf5216c1e6c5ed199595d9ed3e5baf4b0d8
```
