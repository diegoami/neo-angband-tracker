# The free that left the token list behind

Releasing a parsed calculation — as a borg reinitialize does for every
dynamic formula — freed every token and the token-list struct, but not
the list's backing array.

**`calc_free()` freed each token and the `token_array` struct, but not
`token_array->items`, which `borg_array_add()` had allocated
separately — releasing a parsed calculation lost its backing pointer
array.** Measured natively on parse-and-free of "1 + 2": both variants
free identically on the surface (the count is 0 either way,
byte-identical logs), but LeakSanitizer finds the token list's 80-byte
backing array lost in the original (160 bytes in 2 allocations) and
freed with the correction (80 bytes in 1 — the remainder being the
operator-block scratch array, the separate lead in issue 324).

Control: a parse that is never freed never reaches the free path —
both variants evaluate 42 with one calculation and byte-identical
logs.

Consequence: 80 bytes lost per freed calculation — a bounded leak per
reinitialize, not a crash; no death or survival claim.

Canonical report: [issue323](https://github.com/diegoami/neo-angband-tracker/issues/323).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [#341](https://github.com/diegoami/neo-angband-tracker/pull/341).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/calfree-323-evidence
git show 9c13d0adccf11567bfa588873572c452bbbb004f:docs/audit/c-reports/calfree-323/reproduce.py > /tmp/reproduce-calfree-323.py
python3 /tmp/reproduce-calfree-323.py --fixtures-ref 9c13d0adccf11567bfa588873572c452bbbb004f --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` (a normal build and an AddressSanitizer build each), verifies
the source diff is exactly the token-array release, runs the
parse-and-free and the unfreed control per variant, runs the free path
under LeakSanitizer, checks the disappearing token-array leak, the
surviving sibling leak and the byte-identical control and functional
logs, records both full gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The free that left the token list behind**: both variants free
identically; the original loses the token list's backing array, the
correction frees it.

Actual recorded output (`before-free.log` and `after-free.log`,
byte-identical):

```text
GEOMETRY freed count=0
RATING freed_value=3 freed_count=0
```

LeakSanitizer on the same free (`before-asan-free.log` /
`after-asan-free.log`): `160 byte(s) leaked in 2 allocation(s)` —
including the token list's backing array, allocated in `borg_array_add`
from `tokenize_math` — down to `80 byte(s) leaked in 1 allocation(s)`
with that block gone; the remaining operator-block leak (issue 324)
appears in both. Control (`control`): `RATING control_value=42
control_count=1` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`2b98c831ba7b3f53b05ddcf620b8b2c54f5ed6a3`. One block in
`src/borg/borg-formulas-calc.c`. `calc_free()` freed every token and
the `token_array` struct, but `borg_array_add()` had allocated the
list's backing array separately; the correction frees it along with
the struct.

```sh
git fetch --no-tags origin task/c-borg-calc-token-array-free
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-formulas-calc.c | sed -n '548,560p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 2b98c831ba7b3f53b05ddcf620b8b2c54f5ed6a3 -- src/borg/borg-formulas-calc.c
```

### Conditions and limits

Native calculation machinery only (no game state needed): parse the
constant formula "1 + 2", evaluate it (3), and free the calculations
as a reinitialize does; the leak proof is the same parse-and-free
built with AddressSanitizer and run under LeakSanitizer. The lost
allocation is 80 bytes per freed calculation — a bounded leak per
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

Pins: base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `2b98c831ba7b3f53b05ddcf620b8b2c54f5ed6a3` (draft PR #341), fixtures
`e299a3e95423b4185665818a7007b7f912922cf5` (the original receipt's `fixtures_ref`). The generators (`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`) are byte-identical between the run pin `e299a3e95` and every later commit of this branch, so the earlier commands that pin a later evidence commit run the same fixture.

Every case reproduced the recorded result (pass/fail and every recorded
outcome field are equal to the original receipt):

| Case | Original: recorded → re-run | Corrected: recorded → re-run |
| --- | --- | --- |
| `free` | passed=True `freed_value=3 freed_count=0` → passed=True, same | passed=True `freed_value=3 freed_count=0` → passed=True, same |
| `control` | passed=True `control_value=42 control_count=1` → passed=True, same | passed=True `control_value=42 control_count=1` → passed=True, same |
| `asan` | passed=True `160 byte(s) leaked in 2 allocation(s).` → passed=True, same | passed=True `80 byte(s) leaked in 1 allocation(s).` → passed=True, same |

- Controls: `control` byte-identical, `free` byte-identical.
- Full native gates, parsed from the committed alltests logs: before: 965/965 and 5/5 (0 failed lines); after: 965/965 and 5/5 (0 failed lines).
- Byte identity with the original receipt's hashes: 10 of 19 re-run files are byte-identical
  to the hashes the original receipt recorded (`after-build-asan.log`, `after-control.log`, `after-free.log`, `after-results.json`, `after-scenarios.log`, `before-control.log`, `before-free.log`, `before-results.json`, `before-scenarios.log`, `result.json`).
  The others (`after-alltests.log`, `after-asan-free.log`, `after-build.log`, `before-alltests.log`, `before-asan-free.log`, `before-build-asan.log`, `before-build.log`, `configure-asan.log`, `configure.log`) carry build paths, timings and toolchain output:
  this run used gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and cmake version 3.28.3; the original used gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 and cmake version 4.2.3.
- Render check of the committed `replay.html` (sha256 `491998ce7062…`) with
  `tools/c-evidence/render-smoke.js` (headless Chromium, clicks every scenario option and button):
  `RENDER-SMOKE actions=2 errors=0 OK`.

The logs, both results JSONs and `result.json` of this run are in
[`reverify-2026-11-08/`](reverify-2026-11-08/); its
[`receipt.json`](reverify-2026-11-08/receipt.json) records the pins, per-case results,
gate totals, toolchain, render result and the sha256 of every file there.

Repeat (fixtures and fix pinned; `reproduce.py` hard-codes the base and fix
commits and refuses any other source hunk, so it has no `--fix-ref` flag):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/calfree-323-evidence
git show e299a3e95423b4185665818a7007b7f912922cf5:docs/audit/c-reports/calfree-323/reproduce.py > /tmp/reproduce-calfree-323.py
python3 /tmp/reproduce-calfree-323.py --full-tests --fixtures-ref e299a3e95423b4185665818a7007b7f912922cf5
```

## Errata (2026-11-08)

- The "Source proposal" link near the top was labelled `PR-SOURCE` and pointed at an unfilled placeholder URL; on 2026-11-08 it was corrected in place to the real link. The source proposal is draft PR [#341](https://github.com/diegoami/neo-angband-tracker/pull/341); this evidence is draft PR [#342](https://github.com/diegoami/neo-angband-tracker/pull/342).
- The repeat commands above pin `9c13d0adccf11567bfa588873572c452bbbb004f` while `receipt.json` records `fixtures_ref` `e299a3e95423b4185665818a7007b7f912922cf5`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `e299a3e95423b4185665818a7007b7f912922cf5` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
- `receipt.json` records `cases.before.asan.passed: true`. That field only says the scenario's functional assertion passed inside the AddressSanitizer build; it is **not** a clean sanitizer verdict. The original variant leaks: LeakSanitizer reports `160 byte(s) leaked in 2 allocation(s)` with `token_array_leaks: 1` (the freed calculation's token pointer array). The sanitizer result is the `summary` and `token_array_leaks` fields: 1 before, 0 after. The one remaining 80-byte allocation in both variants is the separate operator-block leak (`operator_block_leaks: 1`, issue #324), untouched here.
- `receipt.json` lists `free` under `identical_controls`. `free` is the **defect case**, not an unchanged-situation control: its functional log is byte-identical before and after only because the defect is a leak that does not change the observable result (`freed_value=3 freed_count=0` in both); the defect is visible only in the asan log (`before-asan-free.log` vs `after-asan-free.log`). The only unchanged-situation control is `control` (value 42, byte-identical logs).
- Re-run files whose bytes reproduce the original receipt's sha256 exactly are also committed at their original paths in this directory, so the original receipt's hashes check against them: `after-build-asan.log`, `after-control.log`, `after-free.log`, `after-scenarios.log`, `before-control.log`, `before-free.log`, `before-scenarios.log`. The original run's build, configure and gate logs cannot be reproduced byte for byte; the complete re-run set is under `reverify-2026-11-08/`.
- The original sanitizer scenario logs (`after-asan-free.log`, `before-asan-free.log`) cannot be reproduced byte for byte: they carry process ids, memory addresses and the run's temporary paths. The re-run's sanitizer logs, which give the same verdicts recorded in the original receipt, are committed at those paths and `receipt.json` now hashes them; the original hashes are kept under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
- `receipt.json` was amended on 2026-11-08 for this append: its `README.md` hash now matches this README, and a `render_check` entry points at the re-verification render result. The old and new values are recorded under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
