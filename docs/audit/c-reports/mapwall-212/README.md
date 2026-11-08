# New walls and doors never left the Borg's flow

A Warrior holds a routing flow through a floor grid two grids east —
flow value 50 on the grid, knowledge recorded, the stone-to-mud
detection stamps set. Between two map passes, the door on that grid
closes (a monster closes it, the player hears the click). The next map
pass is supposed to notice the grid turned into a wall: drop it from
the flow (value 255), clear its knowledge flags, refresh the view and
light, and reset the wizard-light and detect-walls stamps so the Borg
re-detects.

**The map pass read the grid's "old" wall state after it had already
overwritten the grid with the game's new feature
(`ag->feat = g.f_idx` at borg-update.c:299, `old_wall` read at 313) —
so old and new always matched and the entire wall-change block
(borg-update.c:594) was dead.** Measured on the exact situation (seed
1000, two production map passes): the original keeps flow=50 with
knowledge and stamps intact through a freshly closed door; the
corrected pass invalidates it — flow 255, knowledge and icky flags
cleared, both stamps reset. The reverse change (a closed door opening)
behaves symmetrically: knowledge cleared and stamps reset while the
flow value is kept (only new walls get 255). A no-change control keeps
everything in both variants, byte-identical. This is a routing-truth
fix: stale flows through fresh walls are how the Borg walks into doors
that no longer exist. No damage or survival claim beyond the tracked
state is made.

Canonical report: [issue212](https://github.com/diegoami/neo-angband-tracker/issues/212).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/mapwall-212-evidence
git show 9fe10e42709c480e7b09c8472f9205b9281d6a1c:docs/audit/c-reports/mapwall-212/reproduce.py > /tmp/reproduce-mapwall-212.py
python3 /tmp/reproduce-mapwall-212.py --fixtures-ref 9fe10e42709c480e7b09c8472f9205b9281d6a1c --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the early old_wall capture, runs three recorded situations on
each, compares controls and gates, and writes `replay.html` for
offline review. Your checkout and character/profile are not used or
edited. The disposable source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `9fe10e42709c480e7b09c8472f9205b9281d6a1c`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact commit;
an earlier pin `5c1d75ac12…` carried a reproduce script whose source
comparison mis-reconstructed the moved lines and was amended before
any capture).

Open [replay.html](replay.html) (download raw and open locally).
Choose **A floor grid turns into a closed door**, then **The grid**
and **After the second pass**: the original keeps the stale state, the
corrected invalidates it. Choose **Control: the grid is unchanged**:
the logs match byte for byte between versions.

Actual recorded output, original (`before-door-close.log`; the
scenario asserts the corrected invalidation, so the original fails it
— that is the fail-before contract):

```text
SETUP pass1 feat=1 flow=50 know=1 icky=1
STATE flow=50 know=1 icky=1 wizard_light=1 detect_walls=1
native-c/mapwall-212 finished: 0/1 passed
```

Corrected (`after-door-close.log`):

```text
STATE flow=255 know=0 icky=0 wizard_light=0 detect_walls=0
native-c/mapwall-212 finished: 1/1 passed
```

Control (`before-no-change.log` / `after-no-change.log` —
byte-identical pair): `STATE flow=50 know=1 icky=1 wizard_light=1
detect_walls=1`.

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Warrior, depth 30, glowing constructed arena, one watched
grid two grids east of the Borg. Between the two production map passes
(`borg_update()` called directly) the fixture sets the game feature
with `square_set_feat` — the same state change a closing door or a
dug-and-resealed tunnel produces. Flow, knowledge, icky flags and the
stamps are planted before the second pass in the state a flow routine
and a stone-to-mud detection would have left — recorded as a fixture
intervention. The tracked state after the pass is read back; the Borg
does not walk anywhere in this fixture.

### File inventory

- `scenario.c` — the native observer (three modes via `MW212_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-door-close.log`, `after-door-close.log`,
  `before-door-open.log`, `after-door-open.log`,
  `before-no-change.log`, `after-no-change.log` — captured scenario
  logs (the control pairs are byte-identical).
- `before-results.json`, `after-results.json` — parsed per-mode rows.
- `configure.log`, `before-build.log`, `after-build.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, identical controls, gates,
  fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.

## Re-verification (2026-11-08)

Why: the logs of the original run were listed in `receipt.json` (with
sha256 values) but were never committed — the repository's `*.log`
ignore rule dropped them. To give reviewers the actual logs, the
pinned runner was executed again at the same pins (fixtures
`9fe10e42709c480e7b09c8472f9205b9281d6a1c`, base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `5bd789ee0ff0a471162d7e3ebefb1ac4d501803c`) with `--full-tests`.
The earlier text above is left unchanged.

The run reproduced every recorded case: per-case pass/fail and every
parsed outcome value in its `result.json` equal the `cases` in the
original `receipt.json` (`matches_original_receipt_cases: true`).
Runner verdict: `VERIFIED: the original keeps stale flows through changed grids, the correction invalidates them; control log identical.`

| Mode | Original (base) | Corrected (fix) |
| --- | --- | --- |
| `door-close` | FAIL: flow=50 know=1 icky=1 wizard_light=1 detect_walls=1 | pass: flow=255 know=0 icky=0 wizard_light=0 detect_walls=0 |
| `door-open` | FAIL: flow=50 know=1 icky=1 wizard_light=1 detect_walls=1 | pass: flow=50 know=0 icky=0 wizard_light=0 detect_walls=0 |
| `no-change` (control, byte-identical) | pass: flow=50 know=1 icky=1 wizard_light=1 detect_walls=1 | pass: flow=50 know=1 icky=1 wizard_light=1 detect_walls=1 |

Full native gates, parsed from the committed alltests logs: original
965/965 unit checks and 5/5 frontend tests; corrected
965/965 and 5/5. Toolchain for this run: `cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`cmake version 3.28.3` (the original run recorded `gcc 15.2.0`, `cmake 4.2.3`). Replay render
check on the committed `replay.html`: `RENDER-SMOKE actions=9 errors=0 OK`
(`tools/c-evidence/render-smoke.js`, Playwright Chromium headless).

Logs, `result.json`, per-phase results and a `receipt.json` with the
sha256 of every file are in [`reverify-2026-11-08/`](reverify-2026-11-08/).
Repeat (fixtures pinned by sha; this runner has no `--fix-ref` flag: the
fix sha `5bd789ee0ff0a471162d7e3ebefb1ac4d501803c` is a constant in the pinned script, which
asserts the source diff against it):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/mapwall-212-evidence
git fetch https://github.com/diegoami/neo-angband-tracker.git task/mapwall-212
git show 9fe10e42709c480e7b09c8472f9205b9281d6a1c:docs/audit/c-reports/mapwall-212/reproduce.py > /tmp/reproduce-mapwall-212.py
python3 /tmp/reproduce-mapwall-212.py --full-tests --fixtures-ref 9fe10e42709c480e7b09c8472f9205b9281d6a1c
```

## Errata (2026-11-08)

"Source proposal: PR (see the issue)" above refers to
diegoami/neo-angband-tracker#387; this evidence package is PR #388.
The pins and repeat commands above are correct as printed.
