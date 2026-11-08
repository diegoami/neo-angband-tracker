# The penalty that counted twice

A level twenty-one priest with every class book faces three vampire
bats, its mana tuned so that casting Dispel Undead leaves five points —
inside the final-teleport band of every class reserve.

**The Borg holds back mana for a final teleport, and casting a dispel
from that reserve is punished with a heavy penalty on the maneuver
value — but two back-to-back blocks applied it: one keyed on the class
reserve (six for a Mage, eight for a Priest, up to twenty-two for a
Ranger) and a second keyed on a fixed six. The second is a subset of
the first for every class, since every class reserve is six or more,
so the penalty simply doubled.** Measured: the dispel is valued at
**−21127** with both blocks and **−10627** with one — exactly one
penalty block of 750 × the spell's cost apart.

The correction removes the fixed-six block, keeping the class-based
one.

Control: with twenty points to spare after the cast — outside the
teleport band — neither block applies, and both variants value the
dispel identically at −127 with byte-identical logs: the correction
removes only the duplicated penalty.

Consequence: dispel maneuvers were penalized twice for the same
reserve, biasing the Borg away from its area attacks exactly when its
mana was low. No death or survival claim is made.

Canonical report: [issue259](https://github.com/diegoami/neo-angband-tracker/issues/259).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dispel-259-evidence
git show b0b58d3209d8248d2103ab202ba69e2de61f133d:docs/audit/c-reports/dispel-259/reproduce.py > /tmp/reproduce-dispel-259.py
python3 /tmp/reproduce-dispel-259.py --fixtures-ref b0b58d3209d8248d2103ab202ba69e2de61f133d --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the removed duplicate
block, runs two recorded situations per variant, checks the
fail-before and pass-after matrix and the byte-identical control,
records both full gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Deep in the teleport reserve**: the original values the dispel at
−21127; the correction at −10627.

Actual recorded output, original (`before-reserve.log`):

```text
MANA dispel_power=14 cursp=19 maxsp=116 ok=1
GEOMETRY player=(11,11) bats=3 temp_n=3 cursp=19 maxsp=116
RATING value=-21127
OUTCOME value=-21127
```

Corrected (`after-reserve.log`): `RATING value=-10627`. Control
(`clear`): `RATING value=-127` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`e5da34177368960c9707df6c7b8d55e6c4ab081a`. Five deleted lines in
`src/borg/borg-fight-attack.c`. In `borg_attack_aux_spell_dispel()`,
the mana-needed-for-final-teleport penalty appears twice: first keyed
on a per-class `penalty` (Mage 6, Ranger 22, Rogue 20, Priest 8,
Paladin 20, Necromancer 10), then again — with a comment noting it
duplicates the Mage value — keyed on a fixed `6`. Because the fixed
test `CURSP − spell_power < 6` can only fire when the class test
`CURSP − spell_power < penalty` already has (every class reserve is at
least six), the second block doubled the penalty for every class. The
correction deletes it.

```sh
git fetch --no-tags origin task/c-borg-dispel-single-reserve
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '2680,2720p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 e5da34177368960c9707df6c7b8d55e6c4ab081a -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Priest level twenty-one, depth five, seed one thousand,
lit constructed 40x40 arena, every class book carried, wisdom
maximized, three awake vampire bats in line of sight, current mana
tuned to five points (or twenty in the control) above the dispel cost
after casting. The measurement calls the public maneuver chooser with
`borg_simulate` set; the raw dispel launch value in this fixture is the
beam-path quirk seen elsewhere (−1), which the penalty arithmetic
dwarfs and does not affect. This isolates the penalty blocks, not full
autonomous play. No death or survival claim.

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

Pins: base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `e5da34177368960c9707df6c7b8d55e6c4ab081a` (draft PR #320), fixtures
`d4937b1b2b5fcbc7b10450c6c3842a4a0e846c23` (the original receipt's `fixtures_ref`). The generators (`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`) are byte-identical between the run pin `d4937b1b2` and every later commit of this branch, so the earlier commands that pin a later evidence commit run the same fixture.

Every case reproduced the recorded result (pass/fail and every recorded
outcome field are equal to the original receipt):

| Case | Original: recorded → re-run | Corrected: recorded → re-run |
| --- | --- | --- |
| `reserve` | passed=False `value=-21127` → passed=False, same | passed=True `value=-10627` → passed=True, same |
| `clear` | passed=True `value=-127` → passed=True, same | passed=True `value=-127` → passed=True, same |

- Controls: `clear` byte-identical.
- Full native gates, parsed from the committed alltests logs: before: 965/965 and 5/5 (0 failed lines); after: 965/965 and 5/5 (0 failed lines).
- Byte identity with the original receipt's hashes: 10 of 14 re-run files are byte-identical
  to the hashes the original receipt recorded (`after-build.log`, `after-clear.log`, `after-reserve.log`, `after-results.json`, `after-scenarios.log`, `before-clear.log`, `before-reserve.log`, `before-results.json`, `before-scenarios.log`, `result.json`).
  The others (`after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`) carry build paths, timings and toolchain output:
  this run used gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and cmake version 3.28.3; the original used gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 and cmake version 4.2.3.
- Render check of the committed `replay.html` (sha256 `23ef3c8fe17d…`) with
  `tools/c-evidence/render-smoke.js` (headless Chromium, clicks every scenario option and button):
  `RENDER-SMOKE actions=4 errors=0 OK`.

The logs, both results JSONs and `result.json` of this run are in
[`reverify-2026-11-08/`](reverify-2026-11-08/); its
[`receipt.json`](reverify-2026-11-08/receipt.json) records the pins, per-case results,
gate totals, toolchain, render result and the sha256 of every file there.

Repeat (fixtures and fix pinned; `reproduce.py` hard-codes the base and fix
commits and refuses any other source hunk, so it has no `--fix-ref` flag):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dispel-259-evidence
git show d4937b1b2b5fcbc7b10450c6c3842a4a0e846c23:docs/audit/c-reports/dispel-259/reproduce.py > /tmp/reproduce-dispel-259.py
python3 /tmp/reproduce-dispel-259.py --full-tests --fixtures-ref d4937b1b2b5fcbc7b10450c6c3842a4a0e846c23
```

## Errata (2026-11-08)

- The "Source proposal" link near the top labelled `PR-SOURCE` points at an unfilled placeholder URL. The source proposal is draft PR [#320](https://github.com/diegoami/neo-angband-tracker/pull/320); this evidence is draft PR [#321](https://github.com/diegoami/neo-angband-tracker/pull/321).
- The repeat commands above pin `b0b58d3209d8248d2103ab202ba69e2de61f133d` while `receipt.json` records `fixtures_ref` `d4937b1b2b5fcbc7b10450c6c3842a4a0e846c23`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `d4937b1b2b5fcbc7b10450c6c3842a4a0e846c23` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
