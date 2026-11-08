# A stale Teleport Other entry erased an adjacent giant's danger

A Warrior stands next to an awake hill giant (measured danger 179 at
one turn) while a Teleport Other evaluation is under way. The Borg
keeps a list of monsters it plans to banish so their danger is not
counted twice; this time the list holds one real entry that is not the
giant, plus the leftover a longer earlier evaluation wrote one past
the count — and that leftover happens to name the giant.

**The exclusion loop in `borg_danger_one_kill()` ran to
`ii <= borg_tp_other_n`, reading the slot one past the entries the
count describes. The giant's index in that stale slot matched, and the
whole danger check returned 0: an adjacent giant was rated harmless
while the Borg scored whether to teleport away.** Measured on the
exact situation (seed 1000, production `borg_danger_one_kill()`): the
original scores danger 0 for the adjacent giant; the corrected loop
reads only the real entries and scores 179.

Two controls stay byte-identical: with the giant genuinely in the
list, both variants exclude its danger (that exclusion is correct and
is kept), and with an empty list both count 179. The writer side of
the same list (appending past its 255 entries) is the other half of
issue 250 and is handled separately; this fix is the reader slip (C
bug 17 of the borg-rs import). No damage or survival claim beyond
this one score is made.

Canonical report: [issue250](https://github.com/diegoami/neo-angband-tracker/issues/250).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tpreader-250-evidence
git show 6f6710ccc3a3dc1b035544338444048b2f7cbced:docs/audit/c-reports/tpreader-250/reproduce.py > /tmp/reproduce-tpreader-250.py
python3 /tmp/reproduce-tpreader-250.py --fixtures-ref 6f6710ccc3a3dc1b035544338444048b2f7cbced --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the bounded exclusion loop, runs three recorded situations on
each, compares controls and gates, and writes `replay.html` for
offline review. Your checkout and character/profile are not used or
edited. The disposable source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `6f6710ccc3a3dc1b035544338444048b2f7cbced`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact
commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **A stale entry one past the count names the giant**, then
**The list** and **The danger score**: the original rates the giant
harmless, the corrected counts 179. Choose **Control: the giant
genuinely is in the list** and **Control: empty list**: both logs
match byte for byte between versions.

Actual recorded output, original (`before-stale-hit.log`; the
scenario asserts the counted danger, so the original fails it — that
is the fail-before contract):

```text
SETUP n=1 idx0=0 idx1=1
DANGER danger=0
native-c/tpreader-250 finished: 0/1 passed
```

Corrected (`after-stale-hit.log`):

```text
DANGER danger=179
native-c/tpreader-250 finished: 1/1 passed
```

Controls (`before-in-list.log` / `after-in-list.log`,
`before-empty.log` / `after-empty.log` — byte-identical pairs):

```text
DANGER danger=0     (in-list: the genuine exclusion is kept)
DANGER danger=179   (empty: no exclusions, full danger)
```

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Warrior, seed 1000, glowing constructed arena, lit
torch, an awake hill giant adjacent to the Borg. The exclusion list
is set directly in the state a longer earlier Teleport Other
evaluation leaves behind (one real entry, one stale leftover past the
count) — recorded as a fixture intervention; the production
per-monster danger routine then runs unmodified. This isolates the
stale-read, not full autonomous play.

### File inventory

- `scenario.c` — the native observer (three modes via `TP250_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-stale-hit.log`, `after-stale-hit.log`,
  `before-in-list.log`, `after-in-list.log`, `before-empty.log`,
  `after-empty.log` — captured scenario logs (the control pairs are
  byte-identical).
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
`6f6710ccc3a3dc1b035544338444048b2f7cbced`, base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `e890819a6b5ac6e7f7135dcc2b234fcc520b9752`) with `--full-tests`.
The earlier text above is left unchanged.

The run reproduced every recorded case: per-case pass/fail and every
parsed outcome value in its `result.json` equal the `cases` in the
original `receipt.json` (`matches_original_receipt_cases: true`).
Runner verdict: `VERIFIED: the original lets a stale entry erase the giant's danger, the correction counts it; two control logs identical.`

| Mode | Original (base) | Corrected (fix) |
| --- | --- | --- |
| `stale-hit` | FAIL: idx0=0 idx1=1; danger=0 | pass: idx0=0 idx1=1; danger=1 |
| `in-list` (control, byte-identical) | pass: idx0=1 idx1=0; danger=0 | pass: idx0=1 idx1=0; danger=0 |
| `empty` (control, byte-identical) | pass: idx0=0 idx1=0; danger=1 | pass: idx0=0 idx1=0; danger=1 |

Full native gates, parsed from the committed alltests logs: original
965/965 unit checks and 5/5 frontend tests; corrected
965/965 and 5/5. Toolchain for this run: `cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`cmake version 3.28.3` (the original run recorded `gcc 15.2.0`, `cmake 4.2.3`). Replay render
check on the committed `replay.html`: `RENDER-SMOKE actions=9 errors=0 OK`
(`tools/c-evidence/render-smoke.js`, Playwright Chromium headless).

Logs, `result.json`, per-phase results and a `receipt.json` with the
sha256 of every file are in [`reverify-2026-11-08/`](reverify-2026-11-08/).
Repeat (fixtures pinned by sha; this runner has no `--fix-ref` flag: the
fix sha `e890819a6b5ac6e7f7135dcc2b234fcc520b9752` is a constant in the pinned script, which
asserts the source diff against it):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tpreader-250-evidence
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tpreader-250
git show 6f6710ccc3a3dc1b035544338444048b2f7cbced:docs/audit/c-reports/tpreader-250/reproduce.py > /tmp/reproduce-tpreader-250.py
python3 /tmp/reproduce-tpreader-250.py --full-tests --fixtures-ref 6f6710ccc3a3dc1b035544338444048b2f7cbced
```

## Errata (2026-11-08)

`receipt.json` records `fixtures_ref`
`6cc5ae14af46a8c269bb5ca22609f882791aa485`; that commit does not exist
in this repository. The correct fixtures pin is the one this README
already prints, `6f6710ccc3a3dc1b035544338444048b2f7cbced` ("CBUG:
native Teleport Other stale-exclusion fixture (tpreader-250)"), the
parent of the artifact commit `9ca89f4461e5`. Its `scenario.c`,
`run-cases.py`, `make_replay.py`, `reproduce.py` and `borg.txt` match
the sha256 values in `receipt.json` byte for byte, and the
re-verification above ran at that pin. The repeat commands above are
correct as printed.

"Source proposal: PR (see the issue)" above refers to
diegoami/neo-angband-tracker#380; this evidence package is PR #381.

Scope note: the corrected loop is the same one-comparison change as the
reader correction already demonstrated for issue #67 (CBUG-0051, pair
#285/#286); this package re-demonstrates it with a different fixture
(adjacent hill giant, danger 0 vs 179). Whether to keep it as a separate
pair or fold it into #67 is an owner decision recorded on issue #250.
