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
