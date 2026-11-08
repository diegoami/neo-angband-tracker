# Glowing floors were never flagged to the borg

This is **glowtrace-213 / issue213**, a map-update bookkeeping defect found by
code reading (borg-rs conversion review; no native measurement existed). The
map update's Floors branch tested `g.f_idx == FEAT_NONE` — the same condition
as the Darkness branch before it in the same `else if` chain — so it was never
taken: the game's permanent lighting never reached the borg's grid flags
(`BORG_GLOW`/`BORG_DARK`/`BORG_LIGHT`), and the flow-toward-light consumer
always found an empty target list.

The condition is a transcription slip from the original borg's `switch`,
which cased `FEAT_FLOOR` here (verified against the 2011 import at
angband commit `a50705002`; the slip entered with the 2023 "Borgsource"
reorg). Upstream master still carries the dead branch today, so this fix has
no upstream counterpart to adopt.

The correction restores the `FEAT_FLOOR` condition and reads the glow state
from `square_islit()` — the grid's permanent-glow flag, matching the
`CAVE_GLOW` test the original branch performed. The `grid_data` lighting enum
cannot express glow: `LIGHTING_LIT` is `map_info`'s default for grids whose
lighting is not otherwise established, so the transcribed
`g.lighting == LIGHTING_LIT` test would have marked remembered dark floors as
glowing. Since the branch never executed at the base, every behaviour the
corrected branch adds is new and limited to floor grids' lighting flags;
walls, doors, traps and unknown grids keep their existing handling.

## What to watch

| Version | Distant glowing floor | Adjacent glowing floor | Perm wall (control) | Flow-toward-light targets |
| --- | --- | --- | --- | --- |
| Original | not flagged | not flagged | not flagged | **0 (empty list)** |
| Corrected | **flagged BORG_GLOW** | **flagged BORG_GLOW** | not flagged | **484** |

Fixture: Human Warrior, seed 213, depth 9, a uniformly permanently-glowing
arena with perm walls, no monsters, no ailments. The Borg views the whole
arena; the production `borg_update()` (including the map update) runs once,
and grid flags plus the consumer state are read back directly.

[replay.html](replay.html) is the offline page for the captured run.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowtrace-213
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowtrace-213-evidence
git show FETCH_HEAD:docs/audit/c-reports/glowtrace-213/reproduce.py > /tmp/reproduce-glowtrace-213.py
python3 /tmp/reproduce-glowtrace-213.py --full-tests --fix-ref da9934216eb15d421220050b374c72b65b9cd680
```

The first fetch brings the fix commit; the second sets `FETCH_HEAD` to the
evidence revision. `--fix-ref` is required: the runner pins the corrected
source by that commit and asserts the correction is exactly the Floors branch
restoration in the single touched file.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `da9934216` (branch
  `task/glowtrace-213`), fixtures `5edc188e` (branch
  `task/glowtrace-213-evidence`)
- before: suite 0/1 — `distant_glow=0 near_glow=0 wall_glow=0` (lit=1 in the
  game, unmarked in the borg), flow-light list empty
- after: suite 1/1 — `distant_glow=1 near_glow=1 wall_glow=0`, flow-light
  list 484 targets
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Chrome headless

## Scope and limits

The fixture isolates the map-update flag stage and one consumer
(`borg_flow_light`). It does not measure autonomous play, encounter outcomes
or the other consumers (`borg-fight-defend.c` unlit-Borg checks,
`borg-commands.c` rest-site ladder), which read the same flags and gain the
same corrected data. No survival or encounter-frequency claim.


## Re-verification with a control (2026-11-08)

Why: the original run's phase logs (configure, builds, full gates) were not
committed (the repository's `*.log` ignore rule dropped them), the receipt had
no sha256 map, the replay render check was only planned, and there was no separate unchanged-situation control.

Two modes were added. `GLOW213_MODE=door` is the control: the same glowing arena with an open door beside the Borg; only the door and permanent-wall grids are printed, and the runner asserts the log is byte-identical (the correction touches only the Floors branch). `GLOW213_MODE=torchlit` is a **diagnostic, not a control**: an arena with no permanent glow, where the floor beside the Borg is lit only by its own light source.

### Owner decision needed: `square_islit()` is not the permanent-glow flag

The correction's comment says `square_islit()` is "the grid's permanent-glow flag, matching the CAVE_GLOW test". At the base it is not: `square_islit()` returns `square_light(c, grid) > 0` (src/cave-square.c:721-724), and `square_light()` returns the grid's current light level `square(c, grid)->light` (:974-978), which includes the player's own light source. The permanent-glow test is `square_isglow()`, `sqinfo_has(..., SQUARE_GLOW)` (:431-434). The torch-lit diagnostic measures the consequence: the floor beside the Borg has `square_isglow=0`, `square_light=1`; the original leaves it unflagged (`BORG_LIGHT` is already set by the Borg's own light code, `near_light_flag=1` in both), while the correction marks it `BORG_GLOW` and the flow-toward-light target list goes from 0 to 9 grids, all inside the Borg's own light radius. Because a lit grid always takes the glow branch, the removed `else` (torch-lit, `BORG_LIGHT`) branch could not run under `square_islit()` anyway. The scope therefore cannot be justified from the code as written; candidate shapes for the owner: (a) keep `square_islit()` (glow = currently lit); (b) `square_isglow()` for `BORG_GLOW`/`BORG_DARK` plus a torch-lit branch setting `BORG_LIGHT` when `square_islit() && !square_isglow()`, the closest to the original borg's three cases; (c) `square_isglow()` only, leaving `BORG_LIGHT` to the light code. The source branch is unchanged here; the main and door results hold for any of the three (the arena glows permanently).

**New fixtures pin `50d4911e0bdfdcc980c5de287b9e90247aed2d0f` supersedes `5edc188e4e37c4ccf351dd5657b83bc5f1167244`** for
every repeat command. It adds only the mode selection and the new case(s); the
main case's code path is unchanged, and the run at the new pin reproduced the
recorded main outcome exactly (the `Failed` line of the original's main log
now cites a later source line, the only difference in that log). Fix
`da9934216eb15d421220050b374c72b65b9cd680`, base `e473720ed`.

| Case | Original (base) | Corrected (fix) | Logs |
| --- | --- | --- | --- |
| `main`: permanently glowing arena | distant/near glow **0/0**, flow-light targets **0**, 0/1 | distant/near glow **1/1**, targets **484**, 1/1 | `before-main.log`, `after-main.log` |
| `door` (control) | door_info=57, no glow; wall unflagged; 1/1 | identical | **byte-identical** `before-door.log` / `after-door.log` |
| `torchlit` (diagnostic) | torch-lit floor: isglow 0, light 1, BORG_GLOW 0; far floor no flags; targets 0 | torch-lit floor **BORG_GLOW 1**; far floor BORG_DARK 1; targets **9** | `before-torchlit.log`, `after-torchlit.log` (differ by design) |

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
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowtrace-213
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowtrace-213-evidence
git show 50d4911e0bdfdcc980c5de287b9e90247aed2d0f:docs/audit/c-reports/glowtrace-213/reproduce.py > /tmp/reproduce-glowtrace-213.py
python3 /tmp/reproduce-glowtrace-213.py --full-tests --fixtures-ref 50d4911e0bdfdcc980c5de287b9e90247aed2d0f --fix-ref da9934216eb15d421220050b374c72b65b9cd680
```

## Revision (2026-11-08): owner-decided shape

**Owner decision: use `square_isglow()`** (option (b) above; recorded on
issue #213). The previous shape read "lit" from `square_islit()`, the grid's
*current* light level, which includes the Borg's own torch. It also dropped
the torch-lit `BORG_LIGHT` branch. The source branch now has one more commit,
`c316a65b248c4372ff3b686eafaa0477c3117eeb`. Measured against base
`e473720ed`, the whole branch is a three-line change to the dead Floors
branch in `src/borg/borg-update.c`:

```diff
-            else if (g.f_idx == FEAT_NONE) {
+            else if (g.f_idx == FEAT_FLOOR) {
...
-                if (g.lighting == LIGHTING_DARK) {
+                if (!square_isglow(cave, l) && !square_islit(cave, l)) {
...
-                else if (g.lighting == LIGHTING_LIT) {
+                else if (square_isglow(cave, l)) {
```

A floor is marked `BORG_GLOW` only when the game flags it `SQUARE_GLOW`. It
is marked `BORG_DARK` when it is neither glowing nor lit. The existing
torch-lit branch still marks a floor that is lit but not glowing as
`BORG_LIGHT`. The comments are unchanged from the base.

The fixtures changed in commit `8934edd4e26dfa0b73aec5d9f4ab89ecdc5aee36`:

- `reproduce.py` asserts the hunk above. It also checks that the source scope
  is that single file.
- **`torchlit` is now an unchanged control** (it was a diagnostic). It prints
  only the floor next to the Borg and the flow-toward-light list, and it
  requires that this torch-lit floor is not `BORG_GLOW` and that the list is
  empty. The runner checks that its logs are byte-identical.
- The torch-lit case's far unlit floor had been on the same log. The
  corrected branch does mark that floor `BORG_DARK`, so it moved to its own
  mode, `unlit`. That mode has an asserted change: `far_dark` must be 0
  before and 1 after.

**New pins supersede the earlier ones** for every repeat command: fixtures
`8934edd4e26dfa0b73aec5d9f4ab89ecdc5aee36` (was `50d4911e0`) and fix
`c316a65b248c4372ff3b686eafaa0477c3117eeb` (was `da9934216`).

| Case | Original (base) | Corrected (fix `c316a65b2`) | Logs |
| --- | --- | --- | --- |
| `main`: permanently glowing arena | distant/near glow **0/0**, flow-light targets **0**, 0/1 | distant/near glow **1/1**, targets **484**, wall unflagged, 1/1 | `before-main.log`, `after-main.log` |
| `door` (control) | door_info=57, no glow; wall unflagged; 1/1 | identical | **byte-identical** |
| `torchlit` (control): floor lit only by the Borg's light | isglow 0, light 1, BORG_GLOW 0, BORG_LIGHT 1; targets 0; 1/1 | identical | **byte-identical** |
| `unlit`: known floor outside the light radius, no glow | far_dark **0**, 0/1 | far_dark **1**, far_glow 0, 1/1 | `before-unlit.log`, `after-unlit.log` |

The main case is unchanged from the previous run: the after log is
byte-identical (including `info33=43 info1211=59`). The before log differs
only in the source line cited by `Failed`, because `scenario.c` grew. Under
the previous shape the torch-lit floor became `BORG_GLOW` and the list had 9
targets. Under this shape both match the original.

Both full native gates pass on each variant: 965/965 unit checks and 5/5
frontend tests, with no failed lines. The figures are parsed from
`revision-2026-11-08/before-alltests.log` and `after-alltests.log`. The full
build of the corrected tree reports no warnings. Toolchain: GNU 13.3.0, cmake
version 3.28.3. Render check: `node tools/c-evidence/render-smoke.js`
(Playwright headless) on the committed `replay.html` printed
`RENDER-SMOKE actions=0 errors=0 OK`.

These files are from this run: the top-level `before-*.log`, `after-*.log`,
results files, `replay.html` and `replay-data.json`. The configure, build,
scenario-runner and full-gate logs and `result.json` are in
[revision-2026-11-08/](revision-2026-11-08/), with their own
[receipt](revision-2026-11-08/receipt.json). The root `receipt.json` records
this run and keeps earlier runs under `superseded_runs`. It also holds a
sha256 map of every file in this directory. The `reverify-2026-11-08/`
directory still holds the previous shape's run.

What this proves: restoring the dead Floors branch lets the game's permanent
lighting reach the Borg's map and its walk-toward-light targets. Unlit floors
are marked dark. Torch-lit floors, doors and walls are unchanged. What it
does not prove: any effect on autonomous play or survival. The other readers
of these flags (the unlit-Borg defence checks and the rest-site choice) were
not exercised.

Repeat, with both shas pinned:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowtrace-213
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowtrace-213-evidence
git show 8934edd4e26dfa0b73aec5d9f4ab89ecdc5aee36:docs/audit/c-reports/glowtrace-213/reproduce.py > /tmp/reproduce-glowtrace-213.py
python3 /tmp/reproduce-glowtrace-213.py --full-tests --fixtures-ref 8934edd4e26dfa0b73aec5d9f4ab89ecdc5aee36 --fix-ref c316a65b248c4372ff3b686eafaa0477c3117eeb
```
