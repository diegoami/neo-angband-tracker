# The reload that forgot its array

Reinitializing the Borg frees the dynamic calculations and parses them
again — but the free emptied the list while keeping its backing pointer
array.

**`calculations_free()` reset the calculation count to zero but
retained the backing pointer array; `borg_array_add()` allocates a
fresh array whenever the count is zero, so the next parse after a
reload overwrote the retained pointer and lost the previous
allocation.** Measured natively: the original still holds the array
after the free (`retained=1`) and LeakSanitizer finds the overwritten
80-byte allocation lost (320 bytes in 4 allocations on the reload
path); the correction releases the array on free (`retained=0`), the
overwritten-array leak is gone (240 bytes in 3), and reloading still
works — the reparsed formula evaluates to 7 in both variants.

Control: a single parse without a reload never touches the free path —
both variants evaluate the formula to 3 with one calculation and
byte-identical logs.

Consequence: every borg reinitialize leaked the previous calculation
backing array — 80 bytes per reload, a slow leak on a manual command,
not a crash; no death or survival claim is made. The two further leaks
visible in the same report (temporary operator arrays, token pointer
arrays) are the separate reported leads in issues 324 and 323, present
in both variants and untouched here.

Canonical report: [issue325](https://github.com/diegoami/neo-angband-tracker/issues/325).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [#337](https://github.com/diegoami/neo-angband-tracker/pull/337).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/calcreload-325-evidence
git show 47d5600af81a1880be98a0ae4a80391e7dd43bd5:docs/audit/c-reports/calcreload-325/reproduce.py > /tmp/reproduce-calcreload-325.py
python3 /tmp/reproduce-calcreload-325.py --fixtures-ref 47d5600af81a1880be98a0ae4a80391e7dd43bd5 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` (a normal build and an AddressSanitizer build each), verifies
the source diff is exactly the array release, runs the reload and
control situations per variant, runs the reload path under
LeakSanitizer, checks the fail-before and pass-after matrix, the
disappearing backing-array leak, the surviving sibling leaks and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The reload that forgot its array**: the original retains the array
across the free and loses the previous allocation; the correction
releases it.

Actual recorded output, original (`before-reload.log`):

```text
GEOMETRY freed retained=1 count=0
RATING retained=1 reload_value=7 reload_count=1
```

Corrected (`after-reload.log`): `RATING retained=0 reload_value=7
reload_count=1`. LeakSanitizer on the reload path
(`before-asan-reload.log` / `after-asan-reload.log`): `320 byte(s)
leaked in 4 allocation(s)` — including the overwritten backing array,
allocated in `borg_array_add` directly from `parse_calculation_line` —
down to `240 byte(s) leaked in 3 allocation(s)` with that block gone;
the remaining operator-block and token-array leaks (issues 324 and 323)
appear in both. Control (`control`): `RATING control_value=3
control_count=1` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`253ab11468bd91f47f2a458ecc3f333a00eaa677`. One block in
`src/borg/borg-formulas-calc.c`. `calculations_free()` freed each
calculation and zeroed the count but kept `calculations.items`;
`borg_array_add()` treats a zero count as unallocated and allocates a
new array, overwriting the retained pointer. The correction frees the
array and clears the pointer and bound so the reload starts from a
clean empty array.

```sh
git fetch --no-tags origin task/c-borg-calc-array-reload
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-formulas-calc.c | sed -n '700,712p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 253ab11468bd91f47f2a458ecc3f333a00eaa677 -- src/borg/borg-formulas-calc.c
```

### Conditions and limits

Native calculation machinery only (no game state needed): parse the
constant formula "1 + 2", free the calculations as a reinitialize does,
parse "3 + 4" and evaluate it. The retained-pointer check reads the
global array state directly; the leak proof is the same reload path
built with AddressSanitizer and run under LeakSanitizer. The lost
allocation is 80 bytes per reload — a slow leak on a manual
reinitialize, not a crash; no death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).


## Re-verification (2026-11-08)

**Why.** The original run's phase logs were never committed: the repository's
`*.log` ignore rule dropped them although `receipt.json` above lists and hashes
them. This section records a second native run at the same pins; the earlier
text of this guide is unchanged.

Pins: base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `253ab11468bd91f47f2a458ecc3f333a00eaa677` (draft PR #337), fixtures
`9f6ee40d7005abe79a07767e48ca8deaa921b96d` (the original receipt's `fixtures_ref`). The generators (`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`) are byte-identical between the run pin `9f6ee40d7` and every later commit of this branch, so the earlier commands that pin a later evidence commit run the same fixture.

Every case reproduced the recorded result (pass/fail and every recorded
outcome field are equal to the original receipt):

| Case | Original: recorded → re-run | Corrected: recorded → re-run |
| --- | --- | --- |
| `reload` | passed=False `retained=1 reload_value=7 reload_count=1` → passed=False, same | passed=True `retained=0 reload_value=7 reload_count=1` → passed=True, same |
| `control` | passed=True `control_value=3 control_count=1` → passed=True, same | passed=True `control_value=3 control_count=1` → passed=True, same |
| `asan` | passed=False `320 byte(s) leaked in 4 allocation(s).` → passed=False, same | passed=True `240 byte(s) leaked in 3 allocation(s).` → passed=True, same |

- Controls: `control` byte-identical.
- Full native gates, parsed from the committed alltests logs: before: 965/965 and 5/5 (0 failed lines); after: 965/965 and 5/5 (0 failed lines).
- Byte identity with the original receipt's hashes: 10 of 19 re-run files are byte-identical
  to the hashes the original receipt recorded (`after-build-asan.log`, `after-control.log`, `after-reload.log`, `after-results.json`, `after-scenarios.log`, `before-control.log`, `before-reload.log`, `before-results.json`, `before-scenarios.log`, `result.json`).
  The others (`after-alltests.log`, `after-asan-reload.log`, `after-build.log`, `before-alltests.log`, `before-asan-reload.log`, `before-build-asan.log`, `before-build.log`, `configure-asan.log`, `configure.log`) carry build paths, timings and toolchain output:
  this run used gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and cmake version 3.28.3; the original used gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 and cmake version 4.2.3.
- Render check of the committed `replay.html` (sha256 `bb54f24e2983…`) with
  `tools/c-evidence/render-smoke.js` (headless Chromium, clicks every scenario option and button):
  `RENDER-SMOKE actions=2 errors=0 OK`.

The logs, both results JSONs and `result.json` of this run are in
[`reverify-2026-11-08/`](reverify-2026-11-08/); its
[`receipt.json`](reverify-2026-11-08/receipt.json) records the pins, per-case results,
gate totals, toolchain, render result and the sha256 of every file there.

Repeat (fixtures and fix pinned; `reproduce.py` hard-codes the base and fix
commits and refuses any other source hunk, so it has no `--fix-ref` flag):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/calcreload-325-evidence
git show 9f6ee40d7005abe79a07767e48ca8deaa921b96d:docs/audit/c-reports/calcreload-325/reproduce.py > /tmp/reproduce-calcreload-325.py
python3 /tmp/reproduce-calcreload-325.py --full-tests --fixtures-ref 9f6ee40d7005abe79a07767e48ca8deaa921b96d
```

## Errata (2026-11-08)

- The "Source proposal" link near the top was labelled `PR-SOURCE` and pointed at an unfilled placeholder URL; on 2026-11-08 it was corrected in place to the real link. The source proposal is draft PR [#337](https://github.com/diegoami/neo-angband-tracker/pull/337); this evidence is draft PR [#338](https://github.com/diegoami/neo-angband-tracker/pull/338).
- The repeat commands above pin `47d5600af81a1880be98a0ae4a80391e7dd43bd5` while `receipt.json` records `fixtures_ref` `9f6ee40d7005abe79a07767e48ca8deaa921b96d`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `9f6ee40d7005abe79a07767e48ca8deaa921b96d` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
- Re-run files whose bytes reproduce the original receipt's sha256 exactly are also committed at their original paths in this directory, so the original receipt's hashes check against them: `after-build-asan.log`, `after-control.log`, `after-reload.log`, `after-scenarios.log`, `before-control.log`, `before-reload.log`, `before-scenarios.log`. The original run's build, configure and gate logs cannot be reproduced byte for byte; the complete re-run set is under `reverify-2026-11-08/`.
- The original sanitizer scenario logs (`after-asan-reload.log`, `before-asan-reload.log`) cannot be reproduced byte for byte: they carry process ids, memory addresses and the run's temporary paths. The re-run's sanitizer logs, which give the same verdicts recorded in the original receipt, are committed at those paths and `receipt.json` now hashes them; the original hashes are kept under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
- `receipt.json` was amended on 2026-11-08 for this append: its `README.md` hash now matches this README, and a `render_check` entry points at the re-verification render result. The old and new values are recorded under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
