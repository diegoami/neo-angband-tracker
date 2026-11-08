# Tap Unlife was rated at the target's full hit points

A level-14 Necromancer stands in a lit arena with both shadow books in
the pack, Nether Bolt and Tap Unlife learned, full mana, no torch. Two
vampire bats are in sight: a healthy one five grids away and one hurt
to a fifth of its hp three grids away, each with a wall just beyond it.
The player is undamaged and in no danger; the only question the attack
chooser answers is which of its two castable attacks is worth more.

**The Borg's Tap Unlife case computes its mana gain as
`BI_CURSP - BI_CURSP` — current mana minus current mana, always zero —
and then rates the cast at `kill->power - sp_drain`: the target's full
estimated hp. Any undead therefore looks like a one-shot worth
finishing.** Measured on this exact situation (seed 1000): the original
casts Tap Unlife toward the healthy bat (keys `* p 2 2 2 2 2 2 5 m c a`,
targeting the healthy ray) while the bat at a fifth of its hp — the one
the same free cast could actually finish — is ignored. The correction
rates the tap at the spell's real damage like the sibling dispel cases
and casts toward the nearly-dead bat (`* p 6 6 6 6 5 m c a`).

The wrong action is a rating inversion, not a crash: a free level-7
spell is preferred against a full-health undead over finishing a
near-dead one, and — against dangerous undead such as a master lich
(observed danger term 1164 vs tap damage 15 in the same engine) — the
inflated full-hp rating is exactly what makes the tap win ratings it
should lose. The corrected build passes the same assertion; two control
situations stay byte-identical before and after: the same arena with
only a hill giant (tap rates zero, Nether Bolt is cast) and only one
bat (the tap wins in both variants).

The one-line effect: the corrected code keeps the passed average damage
for undead — the same shape as `BORG_ATTACK_DISP_UNDEAD`,
`BORG_ATTACK_DISP_EVIL` and `BORG_ATTACK_DRAIN_LIFE` — and ignores the
mana gain, which is what the case comment ("for now ignore the gain in
sp") already says to do. No damage, survival or upstream-behaviour
claim beyond this chooser rating is made.

Canonical report: [issue249](https://github.com/diegoami/neo-angband-tracker/issues/249).
Lead supplied by borg-rs (docs/c-bugs.md section on borg-fight-attack.c);
independently validated and reproduced natively here. Owner intent
recorded on the issue 2026-11-07: damage-based rating (option a).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tapunlife-249-evidence
git show 91eedca363182fe4aa4dfdea5cc16860cd106ca5:docs/audit/c-reports/tapunlife-249/reproduce.py > /tmp/reproduce-tapunlife-249.py
python3 /tmp/reproduce-tapunlife-249.py --fixtures-ref 91eedca363182fe4aa4dfdea5cc16860cd106ca5 --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the removal of the self-subtracting `sp_drain` block, runs
three recorded situations on each, compares controls and gates, and
writes `replay.html` for offline review. Your checkout and
character/profile are not used or edited. The disposable source and
binary finish on the correction. On this machine the full run
(configure, two builds, six scenario suites, two full regression
gates) took about six minutes; it needs `flock`, `cmake`, `cc` and
`node`-free plain Python 3.

The native run recorded in this directory's logs executed at fixtures
pin `91eedca363182fe4aa4dfdea5cc16860cd106ca5`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run, and no native log, source variant or gate
differs between the pin and this head (commit list: pin, artifact
commit, receipt-head fix).

Open [replay.html](replay.html) (download raw and open locally). Choose
**A healthy bat and a nearly-dead one**, then **The target list** and
**The decision**. The original casts Tap Unlife toward the healthy bat;
the corrected casts toward the nearly-dead one. Choose **Control: only
a plain giant** and **Control: only one healthy bat**: both logs match
byte for byte between versions.

Actual recorded output, original (`before-undead.log`; the scenario's
assertion pins the corrected targeting, so the original fails it — that
is the fail-before contract):

```text
SETUP healthy kill=1 power=47 level=24 injury=0 undead=1
SETUP injured kill=2 power=10 level=24 injury=82 undead=1
DECISION acted=1 keys=12 sequence=* p 2 2 2 2 2 2 5 m c a
OUTCOME acted=1 keys=12 tap_cast=1
native-c/tapunlife-249 finished: 0/1 passed
```

Corrected (`after-undead.log`):

```text
DECISION acted=1 keys=10 sequence=* p 6 6 6 6 5 m c a
OUTCOME acted=1 keys=10 tap_cast=1
native-c/tapunlife-249 finished: 1/1 passed
```

Controls (`before-non-undead.log` / `after-non-undead.log`,
`before-bat.log` / `after-bat.log` — byte-identical pairs):

```text
DECISION acted=1 keys=12 sequence=* p 2 2 2 2 2 5 m b a 5   (giant: Nether Bolt, no tap)
DECISION acted=1 keys=10 sequence=* p 6 6 6 6 5 m c a       (single bat: tap in both variants)
```

Key reading: `*` and `p` are the target-mode keys, the digit run walks
the target cursor (`2` is down toward the healthy bat, `6` is right
toward the injured one), `5` selects the grid, and `m c a` is the cast
command, the Dark Rituals book (pack slot c) and the Tap Unlife letter.
`m b a` is the Nether Bolt cast (Into the Shadows in pack slot b).

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Necromancer level 14, depth 30, seed 1000, glowing
constructed arena, both shadow books carried, Nether Bolt and Tap
Unlife learned, full mana and hp, no light source (a necromancer's
spell-fail rate rises by 25 on a lit square, which would block Tap
Unlife from the chooser's ≤25% gate). The production attack chooser
`borg_attack(false)` acts on its own filled target list; the cast is
shown by its queued keys, not executed in the game. The rad-10 dispel
walk that rates Tap Unlife only returns a value when the ray reaches a
wall, so the fixture places one wall just beyond each undead; without
it both variants rate the tap zero and the situation degenerates. The
chooser's 2% simulation paranoia roll is deterministic at seed 1000
and misses at this call position; 18 other seeds were also observed to
flip identically. This isolates the tap rating, not full autonomous
play.

### Why the bats are harmless

`borg_launch_bolt_aux_hack` adds the monster's danger to every attack
rating. A master lich (the original report's example, 1584 hp) carries
a danger term of ~1164 at this level, which dwarfs any tap rating and
would hide the 1482 → 15 rating change behind the danger difference.
Two vampire bats (no ranged attacks) contribute p1 = 0, so the ratings
differ only by the defect under test.

### File inventory

- `scenario.c` — the native observer (three modes via `TAP249_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-undead.log`, `after-undead.log`, `before-non-undead.log`,
  `after-non-undead.log`, `before-bat.log`, `after-bat.log` — captured
  scenario logs (the control pairs are byte-identical).
- `before-results.json`, `after-results.json` — parsed per-mode rows.
- `configure.log`, `before-build.log`, `after-build.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, identical controls, gates,
  fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.


## Re-verification (2026-11-08)

Why: the receipt above hashes the original run's phase logs, but they were
never committed (the repository's `*.log` ignore rule dropped them), so the
package could not be checked against its own receipt. The run was repeated
at the same pins (base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `21a1b9079e612adaf8108390e5ec2379ffce33b7`, fixtures
`91eedca363182fe4aa4dfdea5cc16860cd106ca5`) with `--full-tests`, and every case reproduced the recorded
outcome exactly (the `cases` block of the new `result.json` equals the
original receipt's):

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `undead` | fail: `* p 2 2 2 2 2 2 5 m c a`; `acted=1 keys=12 tap_cast=1` | pass: `* p 6 6 6 6 5 m c a`; `acted=1 keys=10 tap_cast=1` |
| `non-undead` (control) | pass: `* p 2 2 2 2 2 5 m b a 5`; `acted=1 keys=12 tap_cast=0` | pass: `* p 2 2 2 2 2 5 m b a 5`; `acted=1 keys=12 tap_cast=0` |
| `bat` (control) | pass: `* p 6 6 6 6 5 m c a`; `acted=1 keys=10 tap_cast=1` | pass: `* p 6 6 6 6 5 m c a`; `acted=1 keys=10 tap_cast=1` |

Controls byte-identical before/after: `non-undead`, `bat`. Full native gates from the
`*-alltests.log` summaries: before: Total: 965/965 passed (100.0%), Total: 5/5; after: Total: 965/965 passed (100.0%), Total: 5/5. Toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0; cmake version 3.28.3.

These re-run files have the same sha256 as the original receipt lists, i.e.
they are byte-identical to the original run's: `after-build.log`, `after-scenarios.log`, `before-scenarios.log`, `result.json`, `before-bat.log`, `before-non-undead.log`, `before-results.json`, `before-undead.log`, `after-bat.log`, `after-non-undead.log`, `after-results.json`, `after-undead.log`. Files that differ
only by the run's temporary paths and build timing: `after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`.

Render check: `node tools/c-evidence/render-smoke.js replay.html` on the
committed page (every scenario option and visible button):
`RENDER-SMOKE actions=9 errors=0 OK`.

Logs, `result.json` and a receipt with the sha256 of every re-run file are
in [`reverify-2026-11-08/`](reverify-2026-11-08/). Repeat, both pins fixed
(the fix sha is the runner's built-in `FIX` constant, checked against the
branch diff before building):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tapunlife-249-evidence
git show 91eedca363182fe4aa4dfdea5cc16860cd106ca5:docs/audit/c-reports/tapunlife-249/reproduce.py > /tmp/reproduce-tapunlife-249.py
python3 /tmp/reproduce-tapunlife-249.py --full-tests --fixtures-ref 91eedca363182fe4aa4dfdea5cc16860cd106ca5
```

The earlier text of this guide is left as written.

## Errata (2026-11-08)

- **Links.** Source proposal: [#362](https://github.com/diegoami/neo-angband-tracker/pull/362) (branch `task/tapunlife-249`, fix `21a1b9079e612adaf8108390e5ec2379ffce33b7`); evidence PR: [#363](https://github.com/diegoami/neo-angband-tracker/pull/363); canonical issue: [#249](https://github.com/diegoami/neo-angband-tracker/issues/249).
- **README hash in the receipt.** `receipt.json` lists `README.md` sha256 `1603240646e5…`, which is the guide as committed in `3f3cebf76`. Commit `98a12bdd0` then changed one sentence of the guide (the commit-list wording "pin, artifact commit, receipt-head fix"), so the guide at `fc592e4f5` hashes to `e307549f4972…` and no longer matched the receipt; this section changes it again. No other hashed file changed. The re-verification receipt in `reverify-2026-11-08/` hashes only the re-run files.
- **Missing logs.** The file inventory lists phase logs that were not committed with the original package; see the re-verification section above.
- **Restored original logs.** The re-run files whose sha256 equals the value in the original `receipt.json` (`after-build.log`, `after-scenarios.log`, `before-scenarios.log`, `before-bat.log`, `before-non-undead.log`, `before-undead.log`, `after-bat.log`, `after-non-undead.log`, `after-undead.log`) are byte-for-byte the original run's files, so they are also committed here at the paths the receipt names. `after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log` differ by temporary paths and timing and stay only in `reverify-2026-11-08/`.
