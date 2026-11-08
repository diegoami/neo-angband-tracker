# The emergency bolt that never fired

A level-fourteen mage with no mana — the emergency premise: the reserve
bolt is only considered when the mage can no longer cast anything the
normal way — faces a kobold two grids away, while a young red dragon
stands far across the arena, observed first so it owns the lower kill
index.

**The monster scan behind the emergency magic missile broke at the
first live monster.** It examined whichever monster happened to own the
lowest kill index — here the distant dragon — and the too-many-hit-
points veto applied to it: **a distant tough monster vetoed the
emergency bolt for a nearby weak one, and the maneuver was valued at
zero.** Meanwhile `near_monsters` could never exceed one, so the
only-be-one-near-monster test that guards against fainting never
failed.

The correction scans every monster and applies the hit-points veto to
the near ones it describes: the distant dragon is skipped, the kobold
qualifies, and the maneuver is valued at 40.

Control: with only the kobold present, the first live monster is the
right one, and both variants value the maneuver identically at fifty
with byte-identical logs — the fix changes only which monsters the scan
examines.

Consequence: the young mage's last-resort attack was systematically
miscued by monster index order — either vetoed by an irrelevant distant
monster, or blind to a second nearby one. No death or survival claim is
made.

Canonical report: [issue258](https://github.com/diegoami/neo-angband-tracker/issues/258).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/reserve-258-evidence
git show 47b1142e6201ac7b82da31b86d3b291a5d06dc2b:docs/audit/c-reports/reserve-258/reproduce.py > /tmp/reproduce-reserve-258.py
python3 /tmp/reproduce-reserve-258.py --fixtures-ref 47b1142e6201ac7b82da31b86d3b291a5d06dc2b --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the scan-all loop, runs
two recorded situations per variant, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The distant dragon that vetoed the emergency bolt**: the original
values the maneuver at 0; the correction at 40.

Actual recorded output, original (`before-veto.log`):

```text
GEOMETRY player=(11,11) weakling=(11,13) bully=(11,25) danger=544 avoidance=4 temp_n=2
RATING value=0 danger_gate=1
OUTCOME value=0
```

Corrected (`after-veto.log`): `RATING value=40 danger_gate=1`. Control
(`control`): `RATING value=50 danger_gate=1` in both variants,
byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`7b4ecd6b888a1d87b526f273c769449eb013ad9b`. One loop in
`src/borg/borg-fight-attack.c`. In
`borg_attack_aux_spell_bolt_reserve()`, the monster loop counted
`near_monsters` for grids closer than seven, applied the
too-many-hit-points veto, and then `break`ed — unconditionally — after
the first live monster in the kill list. The correction removes the
`break` and scopes the hit-points veto (and the town check that shared
its block) to the near monsters the veto's comment describes; the
fainting guard then counts every nearby monster.

```sh
git fetch --no-tags origin task/c-borg-reserve-near-monsters
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '2554,2600p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 7b4ecd6b888a1d87b526f273c769449eb013ad9b -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Mage level fourteen, depth five, seed one thousand, lit
constructed 40x40 arena, zero current mana with a full maximum (the
emergency premise the maneuver requires), avoidance tuned low so the
pair of monsters clears the must-be-dangerous gate. The kobold stands
two grids away; the young red dragon fourteen grids away, observed
first so it owns the lower kill index. The measurement calls the public
maneuver chooser with `borg_simulate` set. This isolates the monster
scan, not full autonomous play. No death or survival claim.

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

Pins: base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `7b4ecd6b888a1d87b526f273c769449eb013ad9b` (draft PR #318), fixtures
`756cff926941cdb69526818724eba359cc4074f3` (the original receipt's `fixtures_ref`). The generators (`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`) are byte-identical between the run pin `756cff926` and every later commit of this branch, so the earlier commands that pin a later evidence commit run the same fixture.

Every case reproduced the recorded result (pass/fail and every recorded
outcome field are equal to the original receipt):

| Case | Original: recorded → re-run | Corrected: recorded → re-run |
| --- | --- | --- |
| `veto` | passed=False `value=0` → passed=False, same | passed=True `value=40` → passed=True, same |
| `control` | passed=True `value=50` → passed=True, same | passed=True `value=50` → passed=True, same |

- Controls: `control` byte-identical.
- Full native gates, parsed from the committed alltests logs: before: 965/965 and 5/5 (0 failed lines); after: 965/965 and 5/5 (0 failed lines).
- Byte identity with the original receipt's hashes: 9 of 14 re-run files are byte-identical
  to the hashes the original receipt recorded (`after-control.log`, `after-results.json`, `after-scenarios.log`, `after-veto.log`, `before-control.log`, `before-results.json`, `before-scenarios.log`, `before-veto.log`, `result.json`).
  The others (`after-alltests.log`, `after-build.log`, `before-alltests.log`, `before-build.log`, `configure.log`) carry build paths, timings and toolchain output:
  this run used gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and cmake version 3.28.3; the original used gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 and cmake version 4.2.3.
- Render check of the committed `replay.html` (sha256 `4c09ba5a6a2a…`) with
  `tools/c-evidence/render-smoke.js` (headless Chromium, clicks every scenario option and button):
  `RENDER-SMOKE actions=4 errors=0 OK`.

The logs, both results JSONs and `result.json` of this run are in
[`reverify-2026-11-08/`](reverify-2026-11-08/); its
[`receipt.json`](reverify-2026-11-08/receipt.json) records the pins, per-case results,
gate totals, toolchain, render result and the sha256 of every file there.

Repeat (fixtures and fix pinned; `reproduce.py` hard-codes the base and fix
commits and refuses any other source hunk, so it has no `--fix-ref` flag):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/reserve-258-evidence
git show 756cff926941cdb69526818724eba359cc4074f3:docs/audit/c-reports/reserve-258/reproduce.py > /tmp/reproduce-reserve-258.py
python3 /tmp/reproduce-reserve-258.py --full-tests --fixtures-ref 756cff926941cdb69526818724eba359cc4074f3
```

## Errata (2026-11-08)

- The "Source proposal" link near the top labelled `PR-SOURCE` points at an unfilled placeholder URL. The source proposal is draft PR [#318](https://github.com/diegoami/neo-angband-tracker/pull/318); this evidence is draft PR [#319](https://github.com/diegoami/neo-angband-tracker/pull/319).
- The repeat commands above pin `47b1142e6201ac7b82da31b86d3b291a5d06dc2b` while `receipt.json` records `fixtures_ref` `756cff926941cdb69526818724eba359cc4074f3`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `756cff926941cdb69526818724eba359cc4074f3` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
