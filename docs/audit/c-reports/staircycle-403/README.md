# The poisoned, hallucinating Borg stair-cycled without advancing the world clock

This is **staircycle-403 / issue403**, a munchkin-start liveness defect. When
the Borg is poisoned and hallucinating, the munchkin branch keeps directing it
at known stairs. Stair travel does not age the world clock — entering a level
grants minimum move energy and the world tick is skipped during level
generation — so the timed ailments can never wear off while the Borg
shuttles between stairs. The munchkin branch also runs before the ordinary
dungeon think, so the ordinary poison/hallucination recovery handling is
never reached.

The correction adds one bail to the munchkin branch's existing early-bail
sequence: while `BI_ISPOISONED` or `BI_ISIMAGE` is active, hand the turn back
to the ordinary recovery path, which ages turns normally (walking and resting
clear both ailments). A clean control confirms munchkin play is unchanged.

## What to watch

| Version | Ailed (poisoned + hallucinating) | Clean control |
| --- | --- | --- |
| Original | **stair_directed=1** (Power Climb, flows to the stair) | munchkin_handled=1 |
| Corrected | **stair_directed=0** (turn handed to recovery) | munchkin_handled=1 |

Fixture: Human Warrior, seed 1045, depth 9 (past the munchkin depth target,
engaging the Power Climb branch), known up stair underfoot, down stair three
tiles east, no monsters. The ailments enter through the real timed-effect API;
traits derive through the production notice path.

[replay.html](replay.html) is the offline page for the captured run
(regenerate it with the commands below).

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staircycle-403
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staircycle-403-evidence
git show FETCH_HEAD:docs/audit/c-reports/staircycle-403/reproduce.py > /tmp/reproduce-staircycle-403.py
python3 /tmp/reproduce-staircycle-403.py --full-tests --fix-ref d92e06c3f964e4bba19642288ded1b331a327109
```

The first fetch brings the fix commit; the second sets `FETCH_HEAD` to the
evidence revision the script is extracted from. `--fix-ref` is required: the
runner pins the corrected source by that commit and asserts the correction is
exactly the bail insertion in the single touched file. The runner builds the
native C game before and after, runs both scenarios per version, checks the
recorded flip, runs both full native regression gates and generates
`replay.html`. Heavy commands wait on the shared validation lock and print
their log paths.

Prerequisites: Git, Python3, CMake, a C toolchain, `flock`, and native
platform dependencies (X11 development headers/libraries on this Linux
configuration).

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `d92e06c3f` (branch
  `task/staircycle-403`), fixtures `0bd2123be` (branch
  `task/staircycle-403-evidence`)
- before: fails (`stair_directed=1` — the ailed Borg stair-cycles), clean
  control passes (`munchkin_handled=1`)
- after: passes both (`stair_directed=0`, `munchkin_handled=1`)
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Chrome headless
- the runner asserts the correction is exactly the bail insertion and that
  the source scope is the single touched file

## Scope and limits

The fixture isolates the munchkin decision stage, not the consumer zoo run
that first stalled there (a poisoned, hallucinating munchkin at depths 1-2,
reported by borg-zoo run42416417481). The zoo report's end-to-end liveness
proof and negative control (munchkin off: the Borg walks twice, poison ends,
then rests) cover the ordinary path's progress from the real stalled state;
this fixture covers the decision that has to change. No survival,
encounter-frequency or player-death claim is made. The world-clock mechanics
(level entry grants minimum move energy; the world tick is skipped during
level generation) are upstream engine behaviour, quoted from the issue's
native observations, not modified by this correction.


## Re-verification with a control (2026-11-08)

Why: the original run's phase logs (configure, builds, full gates) were not
committed (the repository's `*.log` ignore rule dropped them), the receipt had
no sha256 map, the replay render check was only planned, and the healthy control ran inside the same process as the ailing case, so its log carried that case's messages and could not be compared byte for byte.

The healthy control now also runs on its own (`STAIR403_MODE=clean`): the same position and depth without poison or hallucination, in a fresh process. The runner asserts it passes in both versions with a byte-identical complete log. The `main` mode still runs both scenarios as before.

**New fixtures pin `43dee40ed05284848f3df25aa5abf19a7f776c7f` supersedes `0bd2123be5b7f0830be26949549ea1a4b59e75df`** for
every repeat command. It adds only the mode selection and the new case(s); the
main case's code path is unchanged, and the run at the new pin reproduced the
recorded main outcome exactly (the `Failed` line of the original's main log
now cites a later source line, the only difference in that log). Fix
`d92e06c3f964e4bba19642288ded1b331a327109`, base `e473720ed`.

| Case | Original (base) | Corrected (fix) | Logs |
| --- | --- | --- | --- |
| `main`: poisoned + hallucinating (then the healthy case) | **stair_directed=1** (Power Climb), suite 1/2 | **stair_directed=0** (turn handed to recovery), suite 2/2 | `before-main.log`, `after-main.log` |
| `clean` (control, run alone) | munchkin_handled=1, `<` key; 1/1 | munchkin_handled=1; 1/1 | **byte-identical** `before-clean.log` / `after-clean.log` |

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
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staircycle-403
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staircycle-403-evidence
git show 43dee40ed05284848f3df25aa5abf19a7f776c7f:docs/audit/c-reports/staircycle-403/reproduce.py > /tmp/reproduce-staircycle-403.py
python3 /tmp/reproduce-staircycle-403.py --full-tests --fixtures-ref 43dee40ed05284848f3df25aa5abf19a7f776c7f --fix-ref d92e06c3f964e4bba19642288ded1b331a327109
```
