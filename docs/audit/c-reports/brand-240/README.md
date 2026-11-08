# The brand need that counted twice

A level fifty druid wields a long bow — shooter power three — wears a
firebrand ring, and carries five identified plain arrows.

**The brand-the-ammo test was pasted twice into the inventory scan:
once inside the good-shooter block and once after it, with the same
condition. Every qualifying stack therefore added twenty to the
need-brand trait instead of ten.** Measured: the single stack yields
`need_brand=20` in the original and `10` in the correction.

The correction removes the duplicate, keeping the copy that sits with
the enchant logic inside the good-shooter block.

Control: a stack of two arrows — below the five the test requires —
leaves the need at zero in both variants with byte-identical logs.

Consequence: the Borg's brand-need trait ran double wherever it was
consumed. The shipped configuration formulas do not reference this
trait, so the measured impact is the trait itself and whatever custom
configurations make of it. No death or survival claim is made.

Canonical report: [issue240](https://github.com/diegoami/neo-angband-tracker/issues/240).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [#333](https://github.com/diegoami/neo-angband-tracker/pull/333).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/brand-240-evidence
git show e215578134eb8118b712ec117434ae2112988c98:docs/audit/c-reports/brand-240/reproduce.py > /tmp/reproduce-brand-240.py
python3 /tmp/reproduce-brand-240.py --fixtures-ref e215578134eb8118b712ec117434ae2112988c98 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the removed duplicate
test, runs two recorded situations per variant, checks the fail-before
and pass-after matrix and the byte-identical control, records both
full gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally).
Choose **The brand need that counted twice**: the original counts the
single stack at 20; the correction at 10.

Actual recorded output, original (`before-brand.log`):

```text
SETUP seed=1000 class=Druid level=50 ammo_power=3 ammo_tval=3 brand_legal=1
RATING need_brand=20
OUTCOME need_brand=20
```

Corrected (`after-brand.log`): `RATING need_brand=10`. Control
(`control`): `RATING need_brand=0` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`b30afa1a67436d59b070e3509a7e92b932d01f75`. Fifteen deleted lines in
`src/borg/borg-trait.c`. In `borg_notice_ammo()`, the
brand-the-ammo test (firebrand or the brand spell, five or more shots,
no ego or artifact, the shooter's ammo type) appears once inside the
`BI_AMMO_POWER >= 3` block and once more after the `< 3` return, with
identical conditions; both add ten to `BI_NEED_BRAND_WEAPON`. The
correction deletes the second copy.

```sh
git fetch --no-tags origin task/c-borg-brand-need-single
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-trait.c | sed -n '1155,1210p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 b30afa1a67436d59b070e3509a7e92b932d01f75 -- src/borg/borg-trait.c
```

### Conditions and limits

Native Human Druid level fifty, depth thirty, seed one thousand, lit
constructed 40x40 arena, every class book carried, a long bow wielded
(shooter power three), a firebrand ring worn as the brand ability,
five (or two, in the control) identified plain arrows. The measurement
reads the need-brand trait after one inventory scan. This isolates the
duplicated test, not full autonomous play. No death or survival claim.

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

Pins: base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `b30afa1a67436d59b070e3509a7e92b932d01f75` (draft PR #333), fixtures
`ba3b1c83dcf8739c59dc07605c9ecc7f92611019` (the original receipt's `fixtures_ref`). The generators (`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`) are byte-identical between the run pin `ba3b1c83d` and every later commit of this branch, so the earlier commands that pin a later evidence commit run the same fixture.

Every case reproduced the recorded result (pass/fail and every recorded
outcome field are equal to the original receipt):

| Case | Original: recorded → re-run | Corrected: recorded → re-run |
| --- | --- | --- |
| `brand` | passed=False `need_brand=20` → passed=False, same | passed=True `need_brand=10` → passed=True, same |
| `control` | passed=True `need_brand=0` → passed=True, same | passed=True `need_brand=0` → passed=True, same |

- Controls: `control` byte-identical.
- Full native gates, parsed from the committed alltests logs: before: 965/965 and 5/5 (0 failed lines); after: 965/965 and 5/5 (0 failed lines).
- Byte identity with the original receipt's hashes: 9 of 14 re-run files are byte-identical
  to the hashes the original receipt recorded (`after-brand.log`, `after-control.log`, `after-results.json`, `after-scenarios.log`, `before-brand.log`, `before-control.log`, `before-results.json`, `before-scenarios.log`, `result.json`).
  The others (`after-alltests.log`, `after-build.log`, `before-alltests.log`, `before-build.log`, `configure.log`) carry build paths, timings and toolchain output:
  this run used gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and cmake version 3.28.3; the original used gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 and cmake version 4.2.3.
- Render check of the committed `replay.html` (sha256 `d7e9c6936046…`) with
  `tools/c-evidence/render-smoke.js` (headless Chromium, clicks every scenario option and button):
  `RENDER-SMOKE actions=4 errors=0 OK`.

The logs, both results JSONs and `result.json` of this run are in
[`reverify-2026-11-08/`](reverify-2026-11-08/); its
[`receipt.json`](reverify-2026-11-08/receipt.json) records the pins, per-case results,
gate totals, toolchain, render result and the sha256 of every file there.

Repeat (fixtures and fix pinned; `reproduce.py` hard-codes the base and fix
commits and refuses any other source hunk, so it has no `--fix-ref` flag):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/brand-240-evidence
git show ba3b1c83dcf8739c59dc07605c9ecc7f92611019:docs/audit/c-reports/brand-240/reproduce.py > /tmp/reproduce-brand-240.py
python3 /tmp/reproduce-brand-240.py --full-tests --fixtures-ref ba3b1c83dcf8739c59dc07605c9ecc7f92611019
```

## Errata (2026-11-08)

- The "Source proposal" link near the top was labelled `PR-SOURCE` and pointed at an unfilled placeholder URL; on 2026-11-08 it was corrected in place to the real link. The source proposal is draft PR [#333](https://github.com/diegoami/neo-angband-tracker/pull/333); this evidence is draft PR [#334](https://github.com/diegoami/neo-angband-tracker/pull/334).
- The repeat commands above pin `e215578134eb8118b712ec117434ae2112988c98` while `receipt.json` records `fixtures_ref` `ba3b1c83dcf8739c59dc07605c9ecc7f92611019`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `ba3b1c83dcf8739c59dc07605c9ecc7f92611019` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
- Re-run files whose bytes reproduce the original receipt's sha256 exactly are also committed at their original paths in this directory, so the original receipt's hashes check against them: `after-brand.log`, `after-control.log`, `after-scenarios.log`, `before-brand.log`, `before-control.log`, `before-scenarios.log`. The original run's build, configure and gate logs cannot be reproduced byte for byte; the complete re-run set is under `reverify-2026-11-08/`.
- `receipt.json` was amended on 2026-11-08 for this append: its `README.md` hash now matches this README, and a `render_check` entry points at the re-verification render result. The old and new values are recorded under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
