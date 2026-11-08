# Unknown blow names walked into the NULL end entry

The Borg maps monster blow names — strings that come from the game's
own data (`blow_effects.txt`) — to its internal danger categories
through a small lookup table that ends with a `{ NULL, MONBLOW_NONE }`
marker. A game with modded data can supply a blow name that is not in
the table (the Borg runs on whatever data the game loads).

**The lookup loop visited every table entry including that NULL
marker, so an unknown blow name reached `strcmp(name, NULL)` and
crashed the Borg.** Measured natively (seed 1000, the lookup function
called directly): the original segfaults (exit 139) on the made-up
name `MADE_UP_BLOW_NAME`; the corrected loop stops one entry short of
the marker, unknown names return `MONBLOW_NONE` (0), and the in-table
name `HURT` still resolves (1). Under the address sanitizer the
original's run names the crash; the corrected run is clean. Stock
`blow_effects.txt` names are all in the table — the lead's own
80.6M-call cross-check never hit it — so this is a crash on modded
data, not on stock play.

No damage or survival claim is made; this is a crash fix on a
supported (modded) data configuration.

Canonical report: [issue247](https://github.com/diegoami/neo-angband-tracker/issues/247).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/blowname-247-evidence
git show eea8f6e1cc24ef72fb0dc07ff6df888e100bf3f9:docs/audit/c-reports/blowname-247/reproduce.py > /tmp/reproduce-blowname-247.py
python3 /tmp/reproduce-blowname-247.py --fixtures-ref eea8f6e1cc24ef72fb0dc07ff6df888e100bf3f9 --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories — each in a plain and an
address-sanitizer configuration — verifies the source diff is exactly
the sentinel-safe loop bound, runs the recorded situation on each,
checks the gates, and writes `replay.html` for offline review. Your
checkout and character/profile are not used or edited. The disposable
source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `eea8f6e1cc24ef72fb0dc07ff6df888e100bf3f9`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact commit).

Open [replay.html](replay.html) (download raw and open locally): the
original panel shows the segfault with the sanitizer report, the
corrected panel shows the NONE return.

Actual recorded output, original (`before-main.log`; the process
segfaults, so the suite never finishes — that is the fail-before
contract; stdout buffering hides the prints in the kept log):

```text
exit=139 (SIGSEGV)
```

Address-sanitizer original (`before-asan-main.log`; note the run also
surfaces an unrelated pre-existing init-time overflow in
`borg_sort_comp_hook`, filed separately — it does not mask this
defect, which the plain-build segfault shows):

```text
AddressSanitizer: SEGV on unknown address ... in borg_mon_blow_effect
```

Corrected (`after-main.log`):

```text
KNOWN value=1 expected=1
UNKNOWN value=0 expected=0
native-c/blowname-247 finished: 1/1 passed
```

Full native gates on the corrected variant: 965/965 unit checks and
5/5 frontend tests (`after-alltests.log`; the original variant
segfaults before the gate target is built).

### Conditions and limits

Native Human Warrior, seed 1000. The lookup function is called
directly with an in-table name (`HURT`) and a made-up name
(`MADE_UP_BLOW_NAME`); no game simulation runs. This isolates the
table lookup, not full autonomous play. The address-sanitizer runs use
`-fsanitize-recover=address` with `ASAN_OPTIONS=halt_on_error=0`
because borg initialization trips a separate, pre-existing
heap-buffer-overflow in `borg_sort_comp_hook` (borg-util.c:106) that
would otherwise halt every asan run before the scenario; that report
is visible in the asan logs and filed separately.

### File inventory

- `scenario.c` — the native observer.
- `run-cases.py` — runs the situation on the plain and sanitizer
  binaries, writes logs and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-main.log`, `before-asan-main.log`, `after-main.log`,
  `after-asan-main.log` — captured runs (the original segfaults; the
  corrected passes clean on both builds).
- `before-results.json`, `after-results.json` — parsed rows.
- `configure.log`, `configure-asan.log`, `before-build.log`,
  `before-build-asan.log`, `after-build.log`, `after-build-asan.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, gates, fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.

## Control mode and re-verification (2026-11-08)

Why: the package above had no unchanged-situation control (only the
unknown-name case), and its logs were listed in the receipt but never
committed (the repository's `*.log` ignore rule dropped them). Fixtures
commit `f4244bd11af0a571adcb985f54d0a4c08648cf86` adds a control mode
and the runner was executed again with `--full-tests`. **This fixtures
pin supersedes `eea8f6e1cc24ef72fb0dc07ff6df888e100bf3f9`**; the
unknown-name scenario code is unchanged, only the control mode, its
runner/replay wiring and the control assertions in `reproduce.py` were
added. The earlier text above is left as written; the previous receipt
is kept as `receipt-2026-11-07-eea8f6e1c.json`, and `receipt.json` now
describes this run.

The control (`BW247_MODE=known`) looks up only names that are in the
table: `HURT` and `BLACK_BREATH`, the last real entry, so the loop walks
every entry up to the bound the correction changes and returns before
the NULL marker in both versions. `reproduce.py` requires it to pass in
both phases with byte-identical plain-build logs.

### What to watch

| Situation | Original (base `e473720ed`) | Corrected (fix `541e03b1e`) |
| --- | --- | --- |
| Unknown name `MADE_UP_BLOW_NAME` (plain build) | segfault, exit -11, suite never finishes | `UNKNOWN value=0`, `KNOWN value=1`, 1/1 passed |
| Unknown name (address-sanitizer build) | `AddressSanitizer: SEGV on unknown address 0x000000000000`, frame `#1 borg_mon_blow_effect borg-fight-attack.c:93` | 1/1 passed |
| Control `known`: `HURT`, `BLACK_BREATH` | `KNOWN value=1`, `LAST value=29`, 1/1 passed | **byte-identical log** (`before-known.log` = `after-known.log`) |

The unknown-name rows equal the 2026-11-07 receipt, and `before-main.log`
and `after-main.log` are byte-identical to the hashes recorded there.
The sanitizer logs of both phases (including `*-asan-known.log`) also
show the separately filed init-time `borg_sort_comp_hook`
heap-buffer-overflow; they carry process ids and addresses, so only the
plain-build control logs are compared byte for byte.

Full native gates on both variants (`before-alltests.log`,
`after-alltests.log`): 965/965 unit checks and 5/5 frontend tests.
Replay: the committed `replay.html` passes `node --check` on its
generated script and `tools/c-evidence/render-smoke.js` (no console
errors, no undefined values); it now shows the control. Toolchain for
this run: `cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`, `cmake version
3.28.3` (the 2026-11-07 run recorded gcc 15.2.0, cmake 4.2.3).

Repeat (fixtures pinned; this runner has no `--fix-ref` flag: the fix
sha `541e03b1e6e0bed42e0b71ec520cb6ced8f29d65` is a constant in the
pinned script, which asserts the source diff against it):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/blowname-247-evidence
git fetch https://github.com/diegoami/neo-angband-tracker.git task/blowname-247
git show f4244bd11af0a571adcb985f54d0a4c08648cf86:docs/audit/c-reports/blowname-247/reproduce.py > /tmp/reproduce-blowname-247.py
python3 /tmp/reproduce-blowname-247.py --full-tests --fixtures-ref f4244bd11af0a571adcb985f54d0a4c08648cf86
```

New files: `before-known.log`, `after-known.log`,
`before-asan-known.log`, `after-asan-known.log` (control), and
`receipt-2026-11-07-eea8f6e1c.json` (the superseded receipt).

## Errata (2026-11-08)

- "Full native gates on the corrected variant ... (the original variant
  segfaults before the gate target is built)" above is inaccurate: the
  runner builds and runs the full gates for both variants, and both pass
  965/965 and 5/5 (`before-alltests.log`). Only the scenario binary
  segfaults on the original.
- The `asan` row's `summary` field reads "failed without sanitizer
  report" for the original because of a regex slip in `run-cases.py`
  (`AddressSanitizer:SB?`); `asan_report` is true and
  `before-asan-main.log` contains the SEGV report quoted above.
- "Source proposal: PR (see the issue)" refers to
  diegoami/neo-angband-tracker#397; this evidence package is PR #398.
