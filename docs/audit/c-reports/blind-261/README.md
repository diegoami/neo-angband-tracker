# The fear spell that promised confusion

A level fifty necromancer stands surrounded by four young red dragons
and considers its blind-creature maneuver — which casts Frighten.

**The maneuver was scored with the confusion attack type: the Borg's
danger model suppresses a confused dragon massively (the probe records
a confusion delta of 408 against a fear delta of zero), so a fear
spell was valued at the confusion it cannot deliver — 955, high enough
to win the chooser and be cast.** The correction scores the cast as
the fear effect it is (`BORG_ATTACK_TURN_ALL`, the type the dedicated
Frighten maneuver already uses): the maneuver values −5, **exactly what
the dedicated Frighten maneuver values** — the corrected contract the
scenario asserts.

Control: the dedicated Frighten maneuver, already scored as fear,
values −5 before and after with byte-identical logs — the fix changes
only the mislabeled maneuver.

Consequence: in exactly the situations where confusion is tactically
precious — dangerous adjacent monsters — the Borg could cast Frighten
believing it confuses, then act on a model of a confused monster that
is actually afraid. No death or survival claim is made.

Canonical report: [issue261](https://github.com/diegoami/neo-angband-tracker/issues/261).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/blind-261-evidence
git show da1e319cefd69054651e2f485d836503808f4cd6:docs/audit/c-reports/blind-261/reproduce.py > /tmp/reproduce-blind-261.py
python3 /tmp/reproduce-blind-261.py --fixtures-ref da1e319cefd69054651e2f485d836503808f4cd6 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the fear-scoring token,
runs two recorded situations per variant, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally).
Choose **The fear spell that promised confusion**: the original values
the maneuver at 955; the correction at −5, matching the dedicated
maneuver.

Actual recorded output, original (`before-blind.log`):

```text
FEARPROBE p1=488 p2=488 fear_delta=0 conf_delta=408
RATING blind=955 frighten=-5
OUTCOME blind=955 frighten=-5
```

Corrected (`after-blind.log`): `RATING blind=-5 frighten=-5`. Control
(`control`): `RATING frighten=-5` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`a49b7a6c15f6353cd427af28903355fe258af4d5`. One token plus a comment
in `src/borg/borg-fight-attack.c`. The `BF_SPELL_BLIND_CREATURE` case
casts FRIGHTEN — a fear effect — but passed `BORG_ATTACK_OLD_CONF` to
the launcher, whose damage table then computed the confusion
suppression instead of the fear suppression. The dedicated
`BF_SPELL_FRIGHTEN` case (casting the same spell) already uses
`BORG_ATTACK_TURN_ALL`; the correction matches it.

```sh
git fetch --no-tags origin task/c-borg-frighten-scored-as-fear
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '3924,3932p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 a49b7a6c15f6353cd427af28903355fe258af4d5 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Necromancer level fifty, depth thirty, seed one thousand,
lit constructed 40x40 arena, every class book carried, four awake
young red dragons adjacent, avoidance tuned low. The probe records the
danger model deltas directly: confusion suppresses each dragon by 408
while fear suppresses nothing here. The measurement calls the public
maneuver chooser with `borg_simulate` set. This isolates the
attack-type token, not full autonomous play. No death or survival
claim.

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

Pins: base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `a49b7a6c15f6353cd427af28903355fe258af4d5` (draft PR #331), fixtures
`9cc297fe75ad6c2dd3f9e52a06896832852b59aa` (the original receipt's `fixtures_ref`). The generators (`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`) are byte-identical between the run pin `9cc297fe7` and every later commit of this branch, so the earlier commands that pin a later evidence commit run the same fixture.

Every case reproduced the recorded result (pass/fail and every recorded
outcome field are equal to the original receipt):

| Case | Original: recorded → re-run | Corrected: recorded → re-run |
| --- | --- | --- |
| `blind` | passed=False `blind=955 frighten=-5` → passed=False, same | passed=True `blind=-5 frighten=-5` → passed=True, same |
| `control` | passed=True `frighten=-5` → passed=True, same | passed=True `frighten=-5` → passed=True, same |

- Controls: `control` byte-identical.
- Full native gates, parsed from the committed alltests logs: before: 965/965 and 5/5 (0 failed lines); after: 965/965 and 5/5 (0 failed lines).
- Byte identity with the original receipt's hashes: 10 of 14 re-run files are byte-identical
  to the hashes the original receipt recorded (`after-blind.log`, `after-build.log`, `after-control.log`, `after-results.json`, `after-scenarios.log`, `before-blind.log`, `before-control.log`, `before-results.json`, `before-scenarios.log`, `result.json`).
  The others (`after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`) carry build paths, timings and toolchain output:
  this run used gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and cmake version 3.28.3; the original used gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 and cmake version 4.2.3.
- Render check of the committed `replay.html` (sha256 `5409e7bbb584…`) with
  `tools/c-evidence/render-smoke.js` (headless Chromium, clicks every scenario option and button):
  `RENDER-SMOKE actions=4 errors=0 OK`.

The logs, both results JSONs and `result.json` of this run are in
[`reverify-2026-11-08/`](reverify-2026-11-08/); its
[`receipt.json`](reverify-2026-11-08/receipt.json) records the pins, per-case results,
gate totals, toolchain, render result and the sha256 of every file there.

Repeat (fixtures and fix pinned; `reproduce.py` hard-codes the base and fix
commits and refuses any other source hunk, so it has no `--fix-ref` flag):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/blind-261-evidence
git show 9cc297fe75ad6c2dd3f9e52a06896832852b59aa:docs/audit/c-reports/blind-261/reproduce.py > /tmp/reproduce-blind-261.py
python3 /tmp/reproduce-blind-261.py --full-tests --fixtures-ref 9cc297fe75ad6c2dd3f9e52a06896832852b59aa
```

## Errata (2026-11-08)

- The "Source proposal" link near the top labelled `PR-SOURCE` points at an unfilled placeholder URL. The source proposal is draft PR [#331](https://github.com/diegoami/neo-angband-tracker/pull/331); this evidence is draft PR [#332](https://github.com/diegoami/neo-angband-tracker/pull/332).
- The repeat commands above pin `da1e319cefd69054651e2f485d836503808f4cd6` while `receipt.json` records `fixtures_ref` `9cc297fe75ad6c2dd3f9e52a06896832852b59aa`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `9cc297fe75ad6c2dd3f9e52a06896832852b59aa` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
