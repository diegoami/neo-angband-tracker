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
