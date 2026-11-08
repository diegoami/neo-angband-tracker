# The out-of-light stair command was queued and then disowned

A Warrior with a burned-out torch stands on a down staircase at depth
30. The out-of-light routine reaches its staircase branch and queues
the stair key (`<`) — the right move when the light is gone and the
Borg is standing on a way down.

**The branch queued the stair key and then fell through without
returning, so the routine reported "nothing done"** — measured on the
exact situation (seed 1000, production `borg_think_dungeon_light()`):
keys `* <` are queued (`<`, one key) while the return value says
nothing was queued. The caller's think loop, believing nothing was
queued, keeps going: the next routines queue their own commands behind
the stair key, and everything queued after `<` lands on the new level
after the stairs are taken. The corrected branch returns true after
queueing the stair command, and the routine reports done. Two controls
stay byte-identical: a healthy torch (both variants leave the routine
alone) and a dead torch away from stairs (both find nothing). No
damage or survival claim beyond the reported state is made.

Canonical report: [issue230](https://github.com/diegoami/neo-angband-tracker/issues/230).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairreturn-230-evidence
git show 484c534d59c47a4a84273b7651b0c483cf8b5b67:docs/audit/c-reports/stairreturn-230/reproduce.py > /tmp/reproduce-stairreturn-230.py
python3 /tmp/reproduce-stairreturn-230.py --fixtures-ref 484c534d59c47a4a84273b7651b0c483cf8b5b67 --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the return-true addition, runs three recorded situations on
each, compares controls and gates, and writes `replay.html` for
offline review. Your checkout and character/profile are not used or
edited. The disposable source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `484c534d59c47a4a84273b7651b0c483cf8b5b67`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact
commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **Out of light on a staircase**, then **The situation** and
**The routine**: the original queues `<` and reports nothing done, the
corrected reports done. Choose the two controls: each matches byte for
byte between versions.

Actual recorded output, original (`before-stairs-dead-torch.log`; the
scenario asserts the corrected report, so the original fails it — that
is the fail-before contract):

```text
SETUP light=0 torchtimeout=0 onstairs=1
DECISION acted=0 keys=1 sequence=<
native-c/stairreturn-230 finished: 0/1 passed
```

Corrected (`after-stairs-dead-torch.log`):

```text
DECISION acted=1 keys=1 sequence=<
native-c/stairreturn-230 finished: 1/1 passed
```

Controls (`before-lit.log` / `after-lit.log`,
`before-dead-no-stairs.log` / `after-dead-no-stairs.log` —
byte-identical pairs):

```text
DECISION acted=0 keys=0 sequence=
```

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Warrior, depth 30, seed 1000, glowing constructed arena,
the worn torch's fuel set per situation (0 / 2000 / 0), spare torches
and the recall scroll removed from the pack — recorded as a fixture
intervention so the wear-stuff and recall branches have nothing to act
on and the routine reaches the staircase branch. The production
out-of-light routine runs directly; its queued keys are shown, not
executed. This isolates the missing return, not full autonomous play.

### File inventory

- `scenario.c` — the native observer (three modes via `SR230_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-stairs-dead-torch.log`, `after-stairs-dead-torch.log`,
  `before-lit.log`, `after-lit.log`, `before-dead-no-stairs.log`,
  `after-dead-no-stairs.log` — captured scenario logs (the control
  pairs are byte-identical).
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
`484c534d59c47a4a84273b7651b0c483cf8b5b67`, base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `db0d342ecf9fae2e129740d50d3bb42498f7ac0e`) with `--full-tests`.
The earlier text above is left unchanged.

The run reproduced every recorded case: per-case pass/fail and every
parsed outcome value in its `result.json` equal the `cases` in the
original `receipt.json` (`matches_original_receipt_cases: true`).
Runner verdict: `VERIFIED: the original queues the stair key and reports nothing done, the correction reports done; two control logs identical.`

| Mode | Original (base) | Corrected (fix) |
| --- | --- | --- |
| `stairs-dead-torch` | FAIL: acted=0 keys=1; sequence=< | pass: acted=1 keys=1; sequence=< |
| `lit` (control, byte-identical) | pass: acted=0 keys=0; sequence= | pass: acted=0 keys=0; sequence= |
| `dead-no-stairs` (control, byte-identical) | pass: acted=0 keys=0; sequence= | pass: acted=0 keys=0; sequence= |

Full native gates, parsed from the committed alltests logs: original
965/965 unit checks and 5/5 frontend tests; corrected
965/965 and 5/5. Toolchain for this run: `cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`cmake version 3.28.3` (the original run recorded `gcc 15.2.0`, `cmake 4.2.3`). Replay render
check on the committed `replay.html`: `RENDER-SMOKE actions=9 errors=0 OK`
(`tools/c-evidence/render-smoke.js`, Playwright Chromium headless).

Logs, `result.json`, per-phase results and a `receipt.json` with the
sha256 of every file are in [`reverify-2026-11-08/`](reverify-2026-11-08/).
Repeat (fixtures pinned by sha; this runner has no `--fix-ref` flag: the
fix sha `db0d342ecf9fae2e129740d50d3bb42498f7ac0e` is a constant in the pinned script, which
asserts the source diff against it):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairreturn-230-evidence
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairreturn-230
git show 484c534d59c47a4a84273b7651b0c483cf8b5b67:docs/audit/c-reports/stairreturn-230/reproduce.py > /tmp/reproduce-stairreturn-230.py
python3 /tmp/reproduce-stairreturn-230.py --full-tests --fixtures-ref 484c534d59c47a4a84273b7651b0c483cf8b5b67
```

## Errata (2026-11-08)

The opening above says the Warrior "stands on a down staircase". The
fixture places `FEAT_LESS` (scenario.c: `square_set_feat(cave,
loc(11, 11), FEAT_LESS)`), an **up** staircase, which is what the `<`
key takes; the staircase branch of `borg_think_dungeon_light()` only
acts on `FEAT_LESS`. Nothing else in the recorded run depends on this
wording.

"Source proposal: PR (see the issue)" above refers to
diegoami/neo-angband-tracker#393; this evidence package is PR #394.
The pins and repeat commands above are correct as printed.
