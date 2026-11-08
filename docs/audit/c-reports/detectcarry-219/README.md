# Object detection carried over to the new level

The Borg holds an object-detection stamp for panel 0 — the current
panel — and takes stairs from depth 30 to depth 31. On a depth change
the new-level pass in `borg_update()` clears the per-panel flags that
remember where walls, traps, doors and evil were already detected, so
the Borg re-checks everything on the new level.

**The clearing loop covered `borg_detect_wall`, `borg_detect_trap`,
`borg_detect_door` and `borg_detect_evil` — but not
`borg_detect_obj`.** Panel flags are positions: the same panel on the
new level therefore still counted as detected, and the Borg skipped
casting detect objects there — on a level it had never scanned.
Measured on the exact situation (seed 1000, one production update pass
across the depth change): the original leaves the object flag set
while the wall flag next to it is cleared (the witness that the pass
ran); the corrected loop clears both. A no-depth-change control keeps
both flags in both variants, byte-identical. This is a
missed-detection fix: the Borg under-scans new levels for objects. No
damage or survival claim beyond the tracked flags is made.

Canonical report: [issue219](https://github.com/diegoami/neo-angband-tracker/issues/219).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/detectcarry-219-evidence
git show 55e00023674b9e7634713f2117b5c8c47802ca5f:docs/audit/c-reports/detectcarry-219/reproduce.py > /tmp/reproduce-detectcarry-219.py
python3 /tmp/reproduce-detectcarry-219.py --fixtures-ref 55e00023674b9e7634713f2117b5c8c47802ca5f --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the added `borg_detect_obj` clear, runs two recorded
situations on each, compares controls and gates, and writes
`replay.html` for offline review. Your checkout and character/profile
are not used or edited. The disposable source and binary finish on the
correction.

The native run recorded in this directory's logs executed at fixtures
pin `55e00023674b9e7634713f2117b5c8c47802ca5f`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact
commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **Taking stairs to depth 31**, then **The planted flags** and
**After the pass**: the original keeps the object stamp while clearing
the wall flag next to it; the corrected clears both. Choose
**Control: no depth change**: the logs match byte for byte between
versions.

Actual recorded output, original (`before-new-level.log`; the
scenario asserts the corrected clearing, so the original fails it —
that is the fail-before contract):

```text
SETUP olddepth=30 curdepth=31 panels=1,1
STATE obj=1 wall=0
native-c/detectcarry-219 finished: 0/1 passed
```

Corrected (`after-new-level.log`):

```text
STATE obj=0 wall=0
native-c/detectcarry-219 finished: 1/1 passed
```

Control (`before-same-level.log` / `after-same-level.log` —
byte-identical pair):

```text
STATE obj=1 wall=1
```

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Warrior, seed 1000, glowing constructed arena, declared
depth change 30 to 31 (`borg.status.old_depth` vs `borg.trait[BI_CDEPTH]`,
the same comparison the pass uses). The per-panel object and wall flags
are planted before the pass in the state a detect-objects cast on the
previous level would have left — recorded as a fixture intervention.
The production update pass runs directly; the tracked flags are read
back, nothing is cast in the game.

### File inventory

- `scenario.c` — the native observer (two modes via `DC219_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-new-level.log`, `after-new-level.log`,
  `before-same-level.log`, `after-same-level.log` — captured scenario
  logs (the control pair is byte-identical).
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
`55e00023674b9e7634713f2117b5c8c47802ca5f`, base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `3d104a8b82aafeb421869c62736c8046b6272d82`) with `--full-tests`.
The earlier text above is left unchanged.

The run reproduced every recorded case: per-case pass/fail and every
parsed outcome value in its `result.json` equal the `cases` in the
original `receipt.json` (`matches_original_receipt_cases: true`).
Runner verdict: `VERIFIED: the original keeps the previous level's object-detection stamp, the correction clears it; control log identical.`

| Mode | Original (base) | Corrected (fix) |
| --- | --- | --- |
| `new-level` | FAIL: obj=1 wall=0 | pass: obj=0 wall=0 |
| `same-level` (control, byte-identical) | pass: obj=1 wall=1 | pass: obj=1 wall=1 |

Full native gates, parsed from the committed alltests logs: original
965/965 unit checks and 5/5 frontend tests; corrected
965/965 and 5/5. Toolchain for this run: `cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`cmake version 3.28.3` (the original run recorded `gcc 15.2.0`, `cmake 4.2.3`). Replay render
check on the committed `replay.html`: `RENDER-SMOKE actions=6 errors=0 OK`
(`tools/c-evidence/render-smoke.js`, Playwright Chromium headless).

Logs, `result.json`, per-phase results and a `receipt.json` with the
sha256 of every file are in [`reverify-2026-11-08/`](reverify-2026-11-08/).
Repeat (fixtures pinned by sha; this runner has no `--fix-ref` flag: the
fix sha `3d104a8b82aafeb421869c62736c8046b6272d82` is a constant in the pinned script, which
asserts the source diff against it):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/detectcarry-219-evidence
git fetch https://github.com/diegoami/neo-angband-tracker.git task/detectcarry-219
git show 55e00023674b9e7634713f2117b5c8c47802ca5f:docs/audit/c-reports/detectcarry-219/reproduce.py > /tmp/reproduce-detectcarry-219.py
python3 /tmp/reproduce-detectcarry-219.py --full-tests --fixtures-ref 55e00023674b9e7634713f2117b5c8c47802ca5f
```

## Errata (2026-11-08)

"Source proposal: PR (see the issue)" above refers to
diegoami/neo-angband-tracker#391; this evidence package is PR #392.
The pins and repeat commands above are correct as printed.
