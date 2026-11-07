# Maim Foe reported the last monster it looked at

A level-30 Blackguard stands between an adjacent master lich and an
adjacent kobold. The Maim Foe scan examines both, tracks the best —
the lich, scoring 234 with the danger bonus, versus the kobold's 30 —
and in the simulation pass reports exactly that. The chooser then
re-runs the winning attack in its real pass to execute it.

**The real pass of `borg_attack_aux_maim_foe()` saved the best target
(`b_i`/`b_d`), aimed at it, and then returned `d` — the loop variable
holding the score of the last monster examined.** Measured on the
exact situation (seed 1000, the chooser's real-path entry point called
directly with the filled target list): the original reports 30 — the
kobold's score — for a pass that aimed at the lich; the corrected
function returns 234, the best score it tracked. The trailing
`if (borg_simulate) return d;` behind it was dead code (the
simulation path already returned `b_d`).

The chooser currently discards this real-path value, so the measured
wrongness is in the reported score itself, not in a different cast
today; the fix makes the function return what it reports. Two controls
stay byte-identical: only the lich (last and best are the same) and
nothing adjacent (both return nothing). No damage or survival claim is
made.

Canonical report: [issue256](https://github.com/diegoami/neo-angband-tracker/issues/256).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/maimfoe-256-evidence
git show ea1443bd6a98d4dcbb40c7d7224d2ce224f3bbca:docs/audit/c-reports/maimfoe-256/reproduce.py > /tmp/reproduce-maimfoe-256.py
python3 /tmp/reproduce-maimfoe-256.py --fixtures-ref ea1443bd6a98d4dcbb40c7d7224d2ce224f3bbca --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the `return b_d` change, runs three recorded situations on
each, compares controls and gates, and writes `replay.html` for
offline review. Your checkout and character/profile are not used or
edited. The disposable source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `ea1443bd6a98d4dcbb40c7d7224d2ce224f3bbca`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact
commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **Two adjacent monsters, the dangerous one examined first**,
then **The targets** and **The reported score**: the original reports
30, the corrected 234. Choose the two controls: each matches byte for
byte between versions.

Actual recorded output, original (`before-wrong-return.log`; the
scenario asserts the corrected score, so the original fails it — that
is the fail-before contract):

```text
SETUP lich kill=1 power=1699
SETUP kobold kill=2 power=8
REALCALL value=30
native-c/maimfoe-256 finished: 0/1 passed
```

Corrected (`after-wrong-return.log`):

```text
REALCALL value=234
native-c/maimfoe-256 finished: 1/1 passed
```

Controls (`before-single.log` / `after-single.log`,
`before-none.log` / `after-none.log` — byte-identical pairs):

```text
REALCALL value=234   (single: last and best are the same monster)
REALCALL value=0     (none: nothing adjacent)
```

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Blackguard level 30 (two blows), depth 30, seed 1000,
glowing constructed arena, the Fear and Torment book carried, Maim Foe
learned. The chooser's real-path entry point
(`borg_calculate_attack_effectiveness(BF_SPELL_MAIM_FOE)` with
`borg_simulate` false) is called directly with the two-entry target
list the attack chooser would have filled — recorded as a fixture
intervention. The cast keys are queued, not executed. The chooser
discards this value today; the measured wrongness is the reported
score itself.

### File inventory

- `scenario.c` — the native observer (three modes via `MF256_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-wrong-return.log`, `after-wrong-return.log`,
  `before-single.log`, `after-single.log`, `before-none.log`,
  `after-none.log` — captured scenario logs (the control pairs are
  byte-identical).
- `before-results.json`, `after-results.json` — parsed per-mode rows.
- `configure.log`, `before-build.log`, `after-build.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, identical controls, gates,
  fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.
