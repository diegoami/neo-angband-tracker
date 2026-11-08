# The staff that only pointed at one

A level fifty priest wears a ring carrying the staff-holiness
activation and faces three white harpies to its east.

**The staff-holiness activation floods the area with goodness,
striking every evil monster in sight — but the Borg valued it as a
single-target bolt: radius zero, one monster on a line.** The maneuver
never set the LOS-dispel radius its effect and damage already describe
(`PROJECT_LOS:DISP_EVIL`, the same convention the dispel prayers use).
With the LOS-dispel launch corrected (the stacked walk fix), the
radius change turns the valuation into the sight-line sum it should
be: **one harpy at 28 becomes the sight lines at 60**.

The Wonder half of the report is deliberately left as a bolt: the
wonder activation is an aimed random-effect device (`aim:1`), for
which a single-target valuation is the only sane model.

Control: a second ring carrying the lesser acid bolt — a genuine
single-target activation — is valued identically at 28 before and
after, with byte-identical logs: the fix changes only the
staff-holiness radius.

Note on stacking: this correction is one token stacked on the
LOS-dispel walk correction (issue326, PR327) it depends on — at the
pristine base the radius change alone would route the maneuver into
the walk that discards its blast. The evidence runner verifies the
ancestry and measures the bolt reading at the walk correction alone.

Consequence: the Borg's staff-of-holiness-style activations were valued
as single-target attacks. No death or survival claim is made.

Canonical report: [issue262](https://github.com/diegoami/neo-angband-tracker/issues/262).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE),
stacked on [PR327](https://github.com/diegoami/neo-angband-tracker/pull/327).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staff-262-evidence
git show 4b7fd0c88cdb59dde0ed72e456a3480332af5d91:docs/audit/c-reports/staff-262/reproduce.py > /tmp/reproduce-staff-262.py
python3 /tmp/reproduce-staff-262.py --fixtures-ref 4b7fd0c88cdb59dde0ed72e456a3480332af5d91 --full-tests
```

The runner verifies the ancestry (the correction is stacked on the
walk correction), builds the bolt-radius and LOS-radius variants from
the walk-correction commit `53ef84c7`, runs two recorded situations
per variant, checks the fail-before and pass-after matrix and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally).
Choose **The staff that only pointed at one**: the bolt radius values
one harpy at 28; the LOS radius values the sight lines at 60.

Actual recorded output, bolt radius (`before-holy.log`):

```text
GEOMETRY player=(5,20) harpies=(10,20)(11,19)(11,21) temp_n=3
RATING holy=28
OUTCOME holy=28
```

LOS radius (`after-holy.log`): `RATING holy=60`. Control (`control`):
`RATING bolt2=28` in both variants, byte-identical logs.

Full native gates (both variants, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Walk correction: `53ef84c743768fdde9abaebc27bd8526b4423de9` (issue326).
Stacked correction: `fe34f38d0c998cf34ace02d09bf97774f38e65df`. Three
lines in `src/borg/borg-fight-attack.c`: the `BF_ACT_STAFF_HOLY` case
gains `rad = 10` with a comment, matching the effect's data
(`effect:PROJECT_LOS:DISP_EVIL`) and the convention every other
LOS-dispel maneuver uses.

```sh
git fetch --no-tags origin task/c-borg-staff-holy-los
git show 53ef84c743768fdde9abaebc27bd8526b4423de9:src/borg/borg-fight-attack.c | sed -n '/case BF_ACT_STAFF_HOLY/,+8p'
git diff 53ef84c743768fdde9abaebc27bd8526b4423de9 fe34f38d0c998cf34ace02d09bf97774f38e65df -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Priest level fifty, depth thirty, seed one thousand, lit
constructed 40x40 arena, a worn ring whose activation is the table's
STAFF_HOLY entry (and one carrying ACID_BOLT2 as the control),
charged, three awake white harpies east of the priest. Stacked on the
LOS-dispel walk correction; the bolt-radius reading is measured at
that correction alone. The measurement calls the public maneuver
chooser with `borg_simulate` set. This isolates the radius token, not
full autonomous play. No death or survival claim.

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

Pins: base `53ef84c743768fdde9abaebc27bd8526b4423de9`, fix `fe34f38d0c998cf34ace02d09bf97774f38e65df` (draft PR #329), fixtures
`cf783dd629f2ee016ebb0ac2395d7b891f588e42` (the original receipt's `fixtures_ref`). The generators (`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`) are byte-identical between the run pin `cf783dd62` and every later commit of this branch, so the earlier commands that pin a later evidence commit run the same fixture.

Every case reproduced the recorded result (pass/fail and every recorded
outcome field are equal to the original receipt):

| Case | Original: recorded → re-run | Corrected: recorded → re-run |
| --- | --- | --- |
| `holy` | passed=False `holy=28` → passed=False, same | passed=True `holy=60` → passed=True, same |
| `control` | passed=True `bolt2=28` → passed=True, same | passed=True `bolt2=28` → passed=True, same |

- Controls: `control` byte-identical.
- Full native gates, parsed from the committed alltests logs: before: 965/965 and 5/5 (0 failed lines); after: 965/965 and 5/5 (0 failed lines).
- Byte identity with the original receipt's hashes: 10 of 14 re-run files are byte-identical
  to the hashes the original receipt recorded (`after-build.log`, `after-control.log`, `after-holy.log`, `after-results.json`, `after-scenarios.log`, `before-control.log`, `before-holy.log`, `before-results.json`, `before-scenarios.log`, `result.json`).
  The others (`after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`) carry build paths, timings and toolchain output:
  this run used gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and cmake version 3.28.3; the original used gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 and cmake version 4.2.3.
- Render check of the committed `replay.html` (sha256 `f99d644e36ad…`) with
  `tools/c-evidence/render-smoke.js` (headless Chromium, clicks every scenario option and button):
  `RENDER-SMOKE actions=4 errors=0 OK`.

The logs, both results JSONs and `result.json` of this run are in
[`reverify-2026-11-08/`](reverify-2026-11-08/); its
[`receipt.json`](reverify-2026-11-08/receipt.json) records the pins, per-case results,
gate totals, toolchain, render result and the sha256 of every file there.

Repeat (fixtures and fix pinned; `reproduce.py` hard-codes the base and fix
commits and refuses any other source hunk, so it has no `--fix-ref` flag):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staff-262-evidence
git show cf783dd629f2ee016ebb0ac2395d7b891f588e42:docs/audit/c-reports/staff-262/reproduce.py > /tmp/reproduce-staff-262.py
python3 /tmp/reproduce-staff-262.py --full-tests --fixtures-ref cf783dd629f2ee016ebb0ac2395d7b891f588e42
```

## Errata (2026-11-08)

- The "Source proposal" link near the top labelled `PR-SOURCE` points at an unfilled placeholder URL. The source proposal is draft PR [#329](https://github.com/diegoami/neo-angband-tracker/pull/329); this evidence is draft PR [#330](https://github.com/diegoami/neo-angband-tracker/pull/330).
- The repeat commands above pin `4b7fd0c88cdb59dde0ed72e456a3480332af5d91` while `receipt.json` records `fixtures_ref` `cf783dd629f2ee016ebb0ac2395d7b891f588e42`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `cf783dd629f2ee016ebb0ac2395d7b891f588e42` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
- Stacking: this package's "before" variant is not pristine C. Its base is `53ef84c743768fdde9abaebc27bd8526b4423de9`, the head of the LOS-dispel walk correction (issue #326, draft PR #327); `receipt.json` keeps the pristine root `e473720ed` as `root_base`. The re-verification ran at these same stacked pins. Before any upstream filing, #327 must be accepted upstream first; this branch is then rebased onto upstream master and the evidence re-run and re-pinned.
