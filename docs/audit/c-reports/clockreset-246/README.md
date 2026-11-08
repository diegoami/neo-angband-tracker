# The clock-overflow abort fired before the graceful reset

This is **clockreset-246 / issue246**, an unreachable-handler defect. The
graceful clock reset in `borg_think()` fires at `BORG_TIME_MAX - 100`, but
`borg_think_dungeon()` aborts the Borg with `borg_oops("clock overflow")` at
`BORG_TIME_MAX - 3000` — the abort always won about 2,900 turns earlier, so
the graceful reset was unreachable in dungeon play: an aborted Borg never
thinks again, the clock never advances, and the only handler the Borg ever
exercised was the panic.

Per the owner's decision (reset first, abort as failsafe), the correction
resets the clock in place at the threshold — the same `borg.time.now = 1`
assignment the `borg_think()` overflow check performs — and keeps playing.
The `borg_think()` overflow check remains as a failsafe in case the reset is
somehow missed.

## What to watch

| Version | Clock after the threshold pass | Borg still active |
| --- | --- | --- |
| Original | frozen at the threshold (2147480647) | **no — aborted** ("Aborting (clock overflow).") |
| Corrected | **reset to 1** | **yes** — keeps playing |

Fixture: Native Human Warrior, seed 246, depth 9, a glowing arena with no
monsters. The borg clock is set to the exact abort threshold and one
production `borg_think_dungeon()` pass runs; the clock value, the active
flag and the borg notes are read back directly.

[replay.html](replay.html) is the offline page for the captured run.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/clockreset-246
git fetch https://github.com/diegoami/neo-angband-tracker.git task/clockreset-246-evidence
git show FETCH_HEAD:docs/audit/c-reports/clockreset-246/reproduce.py > /tmp/reproduce-clockreset-246.py
python3 /tmp/reproduce-clockreset-246.py --full-tests --fix-ref 7cc0ebbd2016afeaaf57c5e976251b67f7c0e147
```

The first fetch brings the fix commit; the second sets `FETCH_HEAD` to the
evidence revision. `--fix-ref` is required: the runner pins the corrected
source by that commit and asserts the correction is exactly the graceful
reset replacing the panic in the single touched file.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `7cc0ebbd` (branch
  `task/clockreset-246`), fixtures `85ff2254` (branch
  `task/clockreset-246-evidence`)
- before: suite 0/1 — clock frozen at 2147480647, borg aborted
  (`active=0`), note "Aborting (clock overflow)."
- after: suite 1/1 — clock reset to 1, borg still active, overflow warning
  recorded
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Chrome headless

## Scope and limits

The fixture isolates the overflow handling at the threshold for one think
pass. It does not measure autonomous play across the following thousands of
turns. The reset semantics (`borg.time.now = 1`) are identical to the
original `borg_think()` overflow handler, moved to the point where they can
actually run.


## Re-verification with a control (2026-11-08)

Why: the original run's phase logs (configure, builds, full gates) were not
committed (the repository's `*.log` ignore rule dropped them), the receipt had
no sha256 map, the replay render check was only planned, and there was no unchanged-situation control.

A control mode was added (`CR246_MODE=below-threshold`): the clock one turn below the threshold (`BORG_TIME_MAX - 3001`). Neither the original abort nor the corrected reset applies; both versions keep the clock (2147480646), stay active and take the same ordinary decision, with a byte-identical log. This bounds the change to the threshold itself.

**New fixtures pin `526c82044ee91bbbb531c25edd07f63bed7cd88c` supersedes `85ff225404d99acfdf22568b4bc5ec64afd4218b`** for
every repeat command. It adds only the mode selection and the new case(s); the
main case's code path is unchanged, and the run at the new pin reproduced the
recorded main outcome exactly (the `Failed` line of the original's main log
now cites a later source line, the only difference in that log). Fix
`7cc0ebbd2016afeaaf57c5e976251b67f7c0e147`, base `e473720ed`.

| Case | Original (base) | Corrected (fix) | Logs |
| --- | --- | --- | --- |
| `main`: clock at `BORG_TIME_MAX - 3000` | clock frozen at 2147480647, **aborted** ("Aborting (clock overflow)."), 0/1 | clock **reset to 1**, still active, 1/1 | `before-main.log`, `after-main.log` |
| `below-threshold` (control): `BORG_TIME_MAX - 3001` | clock 2147480646, active, 1/1 | identical, 1/1 | **byte-identical** `before-below-threshold.log` / `after-below-threshold.log` |

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
git fetch https://github.com/diegoami/neo-angband-tracker.git task/clockreset-246
git fetch https://github.com/diegoami/neo-angband-tracker.git task/clockreset-246-evidence
git show 526c82044ee91bbbb531c25edd07f63bed7cd88c:docs/audit/c-reports/clockreset-246/reproduce.py > /tmp/reproduce-clockreset-246.py
python3 /tmp/reproduce-clockreset-246.py --full-tests --fixtures-ref 526c82044ee91bbbb531c25edd07f63bed7cd88c --fix-ref 7cc0ebbd2016afeaaf57c5e976251b67f7c0e147
```

## Revision (2026-11-08): owner-decided shape

**Owner decision: trim the source comment now** (recorded on issue #246).
The reset's source comment used to narrate history ("The previous version
aborted the borg here…"). History belongs in the commit message, not in code
proposed upstream. The source branch has one new commit,
`045f7acc75f64b462d48116a91fce7a15781bd71`, which replaces that comment with
a plain description of what the code does now. **No code changed.** Measured
against base `e473720ed`, the whole branch is now:

```diff
-    /* Prevent clock wrapping Step 2 */
+    /* Prevent clock wrapping Step 2: reset the clock and keep playing.
+     * The overflow check in borg_think() remains as a failsafe. */
     if (borg.time.now >= BORG_TIME_MAX - 3000) {
-        /* Panic */
-        borg_oops("clock overflow");
-
-        /* Oops */
-        return true;
+        borg.time.now = 1;
+        borg_warning("*****WARNING***** Borg Clock Overflow");
     }
```

The fixtures commit `12d555c66c0274df03711e5e906ee24c011f28fb` changes only
the expected hunk in `reproduce.py`. The scenario, the runner and the replay
generator are unchanged.

**New pins supersede the earlier ones** for every repeat command: fixtures
`12d555c66c0274df03711e5e906ee24c011f28fb` (was `526c82044`) and fix
`045f7acc75f64b462d48116a91fce7a15781bd71` (was `7cc0ebbd2`).

The outcomes are identical to the previous run, as expected for a
comment-only change. Every scenario log (`before-main.log`, `after-main.log`
and both `below-threshold` logs), both results files, `replay.html` and
`replay-data.json` came out byte-identical to the files already committed
here.

| Case | Original (base) | Corrected (fix `045f7acc7`) | Logs |
| --- | --- | --- | --- |
| `main`: clock at `BORG_TIME_MAX - 3000` | clock frozen at 2147480647, **aborted** ("Aborting (clock overflow)."), 0/1 | clock **reset to 1**, still active, 1/1 | unchanged from the previous run |
| `below-threshold` (control): `BORG_TIME_MAX - 3001` | clock 2147480646, active, 1/1 | identical, 1/1 | **byte-identical** |

Both full native gates pass on each variant: 965/965 unit checks and 5/5
frontend tests, with no failed lines. The figures are parsed from
`revision-2026-11-08/before-alltests.log` and `after-alltests.log`. The full
build of the corrected tree reports no warnings. Toolchain: GNU 13.3.0, cmake
version 3.28.3. Render check: `node tools/c-evidence/render-smoke.js`
(Playwright headless) on `replay.html` printed
`RENDER-SMOKE actions=0 errors=0 OK`.

The configure, build, scenario-runner and full-gate logs and `result.json`
for this run are in [revision-2026-11-08/](revision-2026-11-08/), with their
own [receipt](revision-2026-11-08/receipt.json). The root `receipt.json`
records this run and keeps earlier runs under `superseded_runs`. It also
holds a sha256 map of every file in this directory.

Repeat, with both shas pinned:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/clockreset-246
git fetch https://github.com/diegoami/neo-angband-tracker.git task/clockreset-246-evidence
git show 12d555c66c0274df03711e5e906ee24c011f28fb:docs/audit/c-reports/clockreset-246/reproduce.py > /tmp/reproduce-clockreset-246.py
python3 /tmp/reproduce-clockreset-246.py --full-tests --fixtures-ref 12d555c66c0274df03711e5e906ee24c011f28fb --fix-ref 045f7acc75f64b462d48116a91fce7a15781bd71
```
