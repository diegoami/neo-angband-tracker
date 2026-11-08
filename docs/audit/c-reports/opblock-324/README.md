# The precedence pass that kept its scratch array

Every dynamic formula the Borg parses goes through a precedence pass
that builds a temporary operator list for each block — and the list's
backing array was never freed.

**`adjust_order_ops_depth_block()` freed the temporary operator tokens
and the `operators` struct, but not the separately allocated `items`
backing array — every populated operator block of every parsed formula
lost that allocation, starting with the full formula file at
startup.** Measured natively on the parse of "1 + 2 * 3": both
variants apply precedence identically (the value is 7 either way,
byte-identical logs), but LeakSanitizer finds the two lost backing
arrays in the original (160 bytes in 2 allocations) and no leaks at
all with the correction.

Control: a bare constant builds no operator list — both variants
evaluate 42 with one calculation and byte-identical logs.

Consequence: 80 bytes lost per populated operator block per parse —
the full dynamic formula file parses at every borg startup and
reinitialize, so the leak recurs on the startup path; bounded, no
corruption or crash, no death or survival claim.

Canonical report: [issue324](https://github.com/diegoami/neo-angband-tracker/issues/324).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/opblock-324-evidence
git show b6eac1ed1f7b93bc42ecc5877e235bc92b19ffdf:docs/audit/c-reports/opblock-324/reproduce.py > /tmp/reproduce-opblock-324.py
python3 /tmp/reproduce-opblock-324.py --fixtures-ref b6eac1ed1f7b93bc42ecc5877e235bc92b19ffdf --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` (a normal build and an AddressSanitizer build each), verifies
the source diff is exactly the operator-array release, parses the
operator formula and the constant control per variant, runs the
operator parse under LeakSanitizer, checks the disappearing leak, the
identical precedence results and the byte-identical control and
functional logs, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally).
Choose **The precedence pass that kept its scratch array**: both
variants compute 7; the original loses 160 bytes in 2 backing-array
allocations on the way, the correction none.

Actual recorded output (`before-operators.log` and
`after-operators.log`, byte-identical):

```text
RATING formula_value=7 calc_count=1
OUTCOME formula_value=7 calc_count=1
```

LeakSanitizer on the same parse (`before-asan-operators.log` /
`after-asan-operators.log`): `160 byte(s) leaked in 2 allocation(s)`
allocated in `borg_array_add` from `adjust_order_ops_depth_block` —
down to no leaks reported. Control (`control`): `RATING
control_value=42 control_count=1` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`02be7739db306897f2441562ee8d27dddc15d36c`. One block in
`src/borg/borg-formulas-calc.c`. The temporary operator list's items
and struct were freed, but `borg_array_add()` had allocated the list's
backing array separately; the correction frees it along with the
struct.

```sh
git fetch --no-tags origin task/c-borg-operator-block-free
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-formulas-calc.c | sed -n '426,436p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 02be7739db306897f2441562ee8d27dddc15d36c -- src/borg/borg-formulas-calc.c
```

### Conditions and limits

Native calculation machinery only (no game state needed): parse the
constant-operator formula "1 + 2 * 3" and evaluate it; the leak proof
is the same parse built with AddressSanitizer and run under
LeakSanitizer. The lost allocations are 80 bytes per populated
operator block per parse — a bounded leak at startup and per
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

Pins: base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `02be7739db306897f2441562ee8d27dddc15d36c` (draft PR #339), fixtures
`4b9ab473a51a05b8b428115b79bbea72cc4b86cb` (the original receipt's `fixtures_ref`). The generators (`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`) are byte-identical between the run pin `4b9ab473a` and every later commit of this branch, so the earlier commands that pin a later evidence commit run the same fixture.

Every case reproduced the recorded result (pass/fail and every recorded
outcome field are equal to the original receipt):

| Case | Original: recorded → re-run | Corrected: recorded → re-run |
| --- | --- | --- |
| `operators` | passed=True `formula_value=7 calc_count=1` → passed=True, same | passed=True `formula_value=7 calc_count=1` → passed=True, same |
| `control` | passed=True `control_value=42 control_count=1` → passed=True, same | passed=True `control_value=42 control_count=1` → passed=True, same |
| `asan` | passed=True `160 byte(s) leaked in 2 allocation(s).` → passed=True, same | passed=True `no leaks reported` → passed=True, same |

- Controls: `control` byte-identical, `operators` byte-identical.
- Full native gates, parsed from the committed alltests logs: before: 965/965 and 5/5 (0 failed lines); after: 965/965 and 5/5 (0 failed lines).
- Byte identity with the original receipt's hashes: 12 of 19 re-run files are byte-identical
  to the hashes the original receipt recorded (`after-asan-operators.log`, `after-build-asan.log`, `after-build.log`, `after-control.log`, `after-operators.log`, `after-results.json`, `after-scenarios.log`, `before-control.log`, `before-operators.log`, `before-results.json`, `before-scenarios.log`, `result.json`).
  The others (`after-alltests.log`, `before-alltests.log`, `before-asan-operators.log`, `before-build-asan.log`, `before-build.log`, `configure-asan.log`, `configure.log`) carry build paths, timings and toolchain output:
  this run used gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and cmake version 3.28.3; the original used gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 and cmake version 4.2.3.
- Render check of the committed `replay.html` (sha256 `f6a16bd80cd8…`) with
  `tools/c-evidence/render-smoke.js` (headless Chromium, clicks every scenario option and button):
  `RENDER-SMOKE actions=2 errors=0 OK`.

The logs, both results JSONs and `result.json` of this run are in
[`reverify-2026-11-08/`](reverify-2026-11-08/); its
[`receipt.json`](reverify-2026-11-08/receipt.json) records the pins, per-case results,
gate totals, toolchain, render result and the sha256 of every file there.

Repeat (fixtures and fix pinned; `reproduce.py` hard-codes the base and fix
commits and refuses any other source hunk, so it has no `--fix-ref` flag):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/opblock-324-evidence
git show 4b9ab473a51a05b8b428115b79bbea72cc4b86cb:docs/audit/c-reports/opblock-324/reproduce.py > /tmp/reproduce-opblock-324.py
python3 /tmp/reproduce-opblock-324.py --full-tests --fixtures-ref 4b9ab473a51a05b8b428115b79bbea72cc4b86cb
```

## Errata (2026-11-08)

- The "Source proposal" link near the top labelled `PR-SOURCE` points at an unfilled placeholder URL. The source proposal is draft PR [#339](https://github.com/diegoami/neo-angband-tracker/pull/339); this evidence is draft PR [#340](https://github.com/diegoami/neo-angband-tracker/pull/340).
- The repeat commands above pin `b6eac1ed1f7b93bc42ecc5877e235bc92b19ffdf` while `receipt.json` records `fixtures_ref` `4b9ab473a51a05b8b428115b79bbea72cc4b86cb`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `4b9ab473a51a05b8b428115b79bbea72cc4b86cb` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
- `receipt.json` records `cases.before.asan.passed: true`. That field only says the scenario's functional assertion passed inside the AddressSanitizer build; it is **not** a clean sanitizer verdict. The original variant leaks: LeakSanitizer reports `160 byte(s) leaked in 2 allocation(s)` with `operator_block_leaks: 1` (the operator list's backing array, allocated via `borg_array_add` from `adjust_order_ops_depth_block`). The sanitizer result is the `summary` and `operator_block_leaks` fields: 1 before, 0 after (`no leaks reported`).
- `receipt.json` lists `operators` under `identical_controls`. `operators` is the **defect case**, not an unchanged-situation control: its functional log is byte-identical before and after only because the defect is a leak that does not change the computed value (formula = 7 in both variants); the defect is visible only in the asan log (`before-asan-operators.log` vs `after-asan-operators.log`). The only unchanged-situation control is `control` (value 42, byte-identical logs).
