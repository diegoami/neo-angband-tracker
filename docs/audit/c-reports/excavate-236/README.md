# The excavated vault grid was remembered as glowing

A level-12 Druid with Stone to Mud digs a quartz grid beside a
permanent vault wall. When the dig succeeds, `borg_excavate_vault()`
updates the Borg's map of that grid — clear the glow flag, then set
the flag the comment calls "Dark".

**The original set the glow flag again under the "Dark" comment** —
`BORG_GLOW` was almost certainly meant to be `BORG_DARK` — so the
freshly dug, actually-dark grid was remembered as glowing. Glow-driven
decisions could treat a dark excavation pocket as safe light (resting
there, flowing there while out of light). Measured on the exact
situation (seed 1000, production `borg_excavate_vault(9)`, the
stone-to-mud cast queued): the original ends with `glow=1 dark=0`; the
corrected version ends with `glow=0 dark=1`, matching the comment. A
no-dig-site control stays byte-identical (nothing touched in either
variant). No damage or survival claim beyond the tracked flags is
made.

Canonical report: [issue236](https://github.com/diegoami/neo-angband-tracker/issues/236).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/excavate-236-evidence
git show be69260bfcf7c51ce762ccf75282b18fbb5f7ad2:docs/audit/c-reports/excavate-236/reproduce.py > /tmp/reproduce-excavate-236.py
python3 /tmp/reproduce-excavate-236.py --fixtures-ref be69260bfcf7c51ce762ccf75282b18fbb5f7ad2 --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the `BORG_GLOW` → `BORG_DARK` flag change, runs two recorded
situations on each, compares controls and gates, and writes
`replay.html` for offline review. Your checkout and character/profile
are not used or edited. The disposable source and binary finish on the
correction.

The native run recorded in this directory's logs executed at fixtures
pin `be69260bfcf7c51ce762ccf75282b18fbb5f7ad2`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **Excavating a vault dig site**, then **The dig site** and
**The tracked flags**: the original shows glow 1, dark 0; the
corrected shows glow 0, dark 1. Choose **Control: no dig site beside a
permanent wall**: the logs match byte for byte between versions.

Actual recorded output, original (`before-excavate.log`; the scenario
asserts the corrected flags, so the original fails it — that is the
fail-before contract):

```text
SETUP vault=1 spelllegal=1 wallfeat=18 perma_adjacent=1
DECISION acted=1 keys=9 sequence=* p 3 6 5 m b c 5
STATE glow=1 dark=0
native-c/excavate-236 finished: 0/1 passed
```

Corrected (`after-excavate.log`):

```text
STATE glow=0 dark=1
native-c/excavate-236 finished: 1/1 passed
```

Control (`before-no-target.log` / `after-no-target.log` —
byte-identical pair):

```text
DECISION acted=0 keys=0 sequence=
STATE glow=0 dark=0
```

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Druid level 13, depth 30, seed 1000, glowing constructed
arena, the Gifts of Nature book carried, Turn Stone to Mud learned,
full mana. The vault flag is set directly and a quartz grid is planted
beside a permanent wall — recorded as a fixture intervention, the
state a vault wall run would present. The production excavation
routine runs directly; the cast keys are shown, not executed. This
isolates the flag marking, not full autonomous play.

### File inventory

- `scenario.c` — the native observer (two modes via `EV236_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-excavate.log`, `after-excavate.log`, `before-no-target.log`,
  `after-no-target.log` — captured scenario logs (the control pair is
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
`4478e689cf5518cbb72a9a34efa1f6b0e3b58f61`, base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `8e0f8448e9ebc6d164e4e621840acb0005094f81`) with `--full-tests`.
The earlier text above is left unchanged.

The run reproduced every recorded case: per-case pass/fail and every
parsed outcome value in its `result.json` equal the `cases` in the
original `receipt.json` (`matches_original_receipt_cases: true`).
Runner verdict: `VERIFIED: the original marks the dug grid glowing, the correction marks it dark; control log identical.`

| Mode | Original (base) | Corrected (fix) |
| --- | --- | --- |
| `excavate` | FAIL: glow=1 dark=0; acted=1 dark_marked=0 | pass: glow=0 dark=1; acted=1 dark_marked=1 |
| `no-target` (control, byte-identical) | pass: glow=0 dark=0; acted=0 dark_marked=0 | pass: glow=0 dark=0; acted=0 dark_marked=0 |

Full native gates, parsed from the committed alltests logs: original
965/965 unit checks and 5/5 frontend tests; corrected
965/965 and 5/5. Toolchain for this run: `cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`cmake version 3.28.3` (the original run recorded `gcc 15.2.0`, `cmake 4.2.3`). Replay render
check on the committed `replay.html`: `RENDER-SMOKE actions=6 errors=0 OK`
(`tools/c-evidence/render-smoke.js`, Playwright Chromium headless).

Logs, `result.json`, per-phase results and a `receipt.json` with the
sha256 of every file are in [`reverify-2026-11-08/`](reverify-2026-11-08/).
Repeat (fixtures pinned by sha; this runner has no `--fix-ref` flag: the
fix sha `8e0f8448e9ebc6d164e4e621840acb0005094f81` is a constant in the pinned script, which
asserts the source diff against it):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/excavate-236-evidence
git fetch https://github.com/diegoami/neo-angband-tracker.git task/excavate-236
git show 4478e689cf5518cbb72a9a34efa1f6b0e3b58f61:docs/audit/c-reports/excavate-236/reproduce.py > /tmp/reproduce-excavate-236.py
python3 /tmp/reproduce-excavate-236.py --full-tests --fixtures-ref 4478e689cf5518cbb72a9a34efa1f6b0e3b58f61
```

## Errata (2026-11-08)

The fixtures pin printed above, `be69260bfcf7c51ce762ccf75282b18fbb5f7ad2`,
does not exist in this repository (it is also recorded as `fixtures_ref`
in `receipt.json`), so the three repeat commands above fail at
`git show`. The real fixtures commit is
`4478e689cf5518cbb72a9a34efa1f6b0e3b58f61` ("CBUG: native vault
excavation flag fixture (excavate-236)"), the parent of the artifact
commit `793f723ebb684f1f2abcdb267dc759861bc7874d`. Its `scenario.c`,
`run-cases.py`, `make_replay.py`, `reproduce.py` and `borg.txt` match
the sha256 values in `receipt.json` byte for byte, and the
re-verification above ran at that commit. Corrected repeat commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/excavate-236-evidence
git show 4478e689cf5518cbb72a9a34efa1f6b0e3b58f61:docs/audit/c-reports/excavate-236/reproduce.py > /tmp/reproduce-excavate-236.py
python3 /tmp/reproduce-excavate-236.py --fixtures-ref 4478e689cf5518cbb72a9a34efa1f6b0e3b58f61 --full-tests
```

"Source proposal: PR (see the issue)" above refers to
diegoami/neo-angband-tracker#400; this evidence package is PR #401.
