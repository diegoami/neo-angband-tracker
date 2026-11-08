# An arc that hit everything in range

A Mage considers Acid Spray, a sixty-degree arc, at a kobold four
grids north. Another kobold stands four grids south — inside the
spell's ten-grid range but one hundred eighty degrees off the spray.

**The angle test that delineates the arc reoriented the spell's target
grid instead of the grid the blast scan was looking at, so every grid
received the same angle difference: the arc counted every monster in
range and line of sight, or none.** In practice the constant passed,
and the spray was valued as if it hit everything around the caster:
17 with the southern kobold counted, against a reference of 6 with
that kobold hidden from the Borg's knowledge. The correction measures
each scanned grid; the off-arc bystander adds nothing and the value
returns to 6.

Control: with two monsters standing inside the arc, both versions
count both and the complete log is byte-identical before and after —
the correction removes only off-angle counting, not legitimate
coverage.

Consequence: arc spells were over-valued in exactly the crowded,
surrounded situations where the Borg relies on the angle to choose
them. No death or survival claim is made.

Canonical report: [issue253](https://github.com/diegoami/neo-angband-tracker/issues/253).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [#297](https://github.com/diegoami/neo-angband-tracker/pull/297).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/arc-253-evidence
git show 4af016461e556cae771c4870e5dffad27c1a3077:docs/audit/c-reports/arc-253/reproduce.py > /tmp/reproduce-arc-253.py
python3 /tmp/reproduce-arc-253.py --fixtures-ref 4af016461e556cae771c4870e5dffad27c1a3077 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the scanned-grid
orientation, runs two recorded situations per variant, checks the
fail-before/pass-after matrix and the byte-identical control, records
both full gates and writes an offline `replay.html`. The native
verification ran with these generators except the replay generator's
final parenthesis, completed for the captured package; no native log,
source variant or gate differs.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A bystander behind the spray**: the original values the spray at 17
with a reference of 6; the correction values it at 6. The control
shows both in-arc monsters counted identically.

Actual recorded output, original (`before-arc-bystander.log`):

```text
GEOMETRY front=(11,7) behind=(11,15) player=(11,11) temp_n=2
RATING reference=6 value=17
OUTCOME reference=6 value=17
```

Corrected (`after-arc-bystander.log`): `RATING reference=6 value=6`,
`OUTCOME reference=6 value=6`. Control (`arc-both`): value 44 in both
variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`7f33e9fbf83059a0a1ed5e530f463124ce1e4be0`. Two tokens in
`src/borg/borg-fight-attack.c`. Inside the blast scan
`for (ry ...) for (rx ...)` of `borg_launch_arc_at_location()`, the
table lookup for the current grid was built from the function's target
parameters (`y - y1 + 20`, `x - x1 + 20`) — a constant for the whole
scan — instead of the scanned grid (`ry - y1 + 20`, `rx - x1 + 20`).
The comparison therefore evaluated the same angle difference for every
grid.

```sh
git fetch --no-tags origin task/c-borg-arc-grid-angle
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '1930,1985p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 7f33e9fbf83059a0a1ed5e530f463124ce1e4be0 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Mage level 26, depth 30, seed 1000, lit constructed
40x40 arena, the Attacks and Knowledge magic book, Acid Spray learned
as the only spell, full mana, a torch. The production valuation call
and its damage expression are used unchanged; the reference value
hides the bystander from the Borg's knowledge for one measurement and
restores it (kill record save/restore). Damage contributions fall with
distance. This isolates the angle test, not full autonomous play. No
death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).


## Re-verification (2026-11-08)

Why: the original run's phase, build and gate logs were listed and hashed in
`receipt.json` but never committed (the repository's `*.log` ignore rule
dropped them), so the receipt could not be checked against published bytes.
The same pinned runner was run again on 2026-11-08 at the receipt's own pins
(base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `7f33e9fbf83059a0a1ed5e530f463124ce1e4be0`, fixtures `01709aeafc333823f3f941626ecc9e8bfa6a8e2a`) with `--full-tests`.
Every case reproduced the recorded outcome; the runner's per-case results
(`result.json` `cases`) are identical to the committed original.

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `arc-bystander` | arc rated `value=17` against reference 6; 0/1 | `value=6` (equals the reference); 1/1 |
| `arc-both` (control) | `value=44`; 1/1 | byte-identical log |

Gates from the re-run's alltests logs: before Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines); after Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines).
Re-run toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, cmake version 3.28.3 (the original receipt records
gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3; the toolchain differs, and the files listed
below as byte-identical are identical nonetheless).
Corrected source sha256 matches the original receipt.
Render check of the committed `replay.html` with `tools/c-evidence/render-smoke.js`
(Playwright Chromium, every option and button clicked): `RENDER-SMOKE actions=4 errors=0 OK`.
The original receipt records its own headless check under the `browser` key.
At the receipt pin `01709aeaf` the runner finished every native step, assertion and gate and wrote `result.json`, then stopped with exit 1 in the last step because `make_replay.py` at that pin does not parse (see Errata and `reverify-2026-11-08/runner-stdout.txt`). The generator from the guide pin `4af016461` regenerates the committed `replay.html` and `replay-data.json` byte-identically from this re-run's logs, so the repeat command below pins `4af016461`.

Where the logs are: [`reverify-2026-11-08/`](reverify-2026-11-08/) holds every log,
`result.json` and per-mode results of the re-run, with
[`reverify-2026-11-08/receipt.json`](reverify-2026-11-08/receipt.json) (pins, per-case
results, gate lines, toolchain, render result, sha256 of every file there).
9 re-run files are byte-identical to the hashes in the original
`receipt.json`; those not already committed were also placed at their original paths in this directory, so the original receipt now resolves for them: `after-arc-both.log`, `after-arc-bystander.log`, `after-scenarios.log`, `before-arc-both.log`, `before-arc-bystander.log`, `before-scenarios.log`.
The rest (`after-alltests.log`, `after-build.log`, `before-alltests.log`, `before-build.log`, `configure.log`) differ in
run-specific content (temporary paths, timings, build output of the different toolchain) and are
published only under `reverify-2026-11-08/`.

Repeat (fixtures pinned by sha; the fix sha is fixed inside `reproduce.py` at that pin, and the runner
refuses any other source diff):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/arc-253-evidence
git show 4af016461e556cae771c4870e5dffad27c1a3077:docs/audit/c-reports/arc-253/reproduce.py > /tmp/reproduce-arc-253.py
python3 /tmp/reproduce-arc-253.py --full-tests --fixtures-ref 4af016461e556cae771c4870e5dffad27c1a3077
```

## Errata (2026-11-08)

- `make_replay.py` changed after the run pin: at the receipt's `fixtures_ref` `01709aeaf` it does not parse, so a run there ends with exit 1 after all native checks and gates. The capture commit `4af016461` fixed it (one line); no other generator changed between `01709aeaf` and `6f1f66446`, and no native log differs.
- The source-proposal link near the top of this README was a placeholder; it now points to the source PR [#297](https://github.com/diegoami/neo-angband-tracker/pull/297) (evidence PR #298, canonical issue #253).
- `receipt.json` was amended on 2026-11-08 for this only: its `README.md` hash now covers the README with these sections and the link fix, and a `render_check` key records the smoke check above; the old→new values are listed under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
