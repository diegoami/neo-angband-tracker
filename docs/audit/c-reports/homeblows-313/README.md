# The ten rapiers that weighed as one

Totalling up the home, the Borg asked how many blows a stack of
weapons gives — and weighed the whole stack as if it were one weapon.

**`borg_calc_blows()` multiplied the weapon's weight by the stack
size, while the engine's `calc_blows()` weighs a single weapon
(`object_weight_one`). For the wielded weapon the two agree (quantity
one), but a stack of light weapons in the home was treated as one very
heavy weapon and under-counted its blows.** Measured natively with a
stack of ten rapiers (strength 17, dexterity 16): the original
computes 1 blow where the engine gives 2 per weapon; the correction
weighs one weapon and both say 2.

Control: a single rapier (stack size one) agrees in both variants —
2 blows, byte-identical logs.

Consequence: the home-damage total is wrong for stacked weapons,
biasing the Borg's home comparisons. The measured fact is the
divergence itself; what it does to a home decision is not claimed. No
crash; no death or survival claim.

Canonical report: [issue313](https://github.com/diegoami/neo-angband-tracker/issues/313).
Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/homeblows-313-evidence
git show 0d3c1316a971270c67c6fed234a0d1ea0fa1c70e:docs/audit/c-reports/homeblows-313/reproduce.py > /tmp/reproduce-homeblows-313.py
python3 /tmp/reproduce-homeblows-313.py --fixtures-ref 0d3c1316a971270c67c6fed234a0d1ea0fa1c70e --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the single-weapon
weight, parses a ten-rapier stack and a single rapier through the
production item analyzer, computes the Borg and engine blows side by
side per variant, checks the fail-before and pass-after matrix and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The ten rapiers that weighed as one**: the original weighs the whole
stack (1 blow against the engine's 2); the correction weighs one
weapon (2 equals 2).

Actual recorded output, original (`before-stack.log`):

```text
GEOMETRY iqty=10 weight=40 weight_one=40
RATING borg_blows=1 engine_blows=2
```

Corrected (`after-stack.log`): `RATING borg_blows=2 engine_blows=2`.
Control (`single`): `RATING borg_blows=2 engine_blows=2` in both
variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`d7b42db02cc74473bc175d2c37dd6feba1a77811`. One line in
`src/borg/borg-trait.c`: `item->weight * item->iqty` becomes
`item->weight`, matching the engine's single-weapon weight.

```sh
git fetch --no-tags origin task/c-borg-blows-single-weight
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-trait.c | sed -n '1035,1048p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 d7b42db02cc74473bc175d2c37dd6feba1a77811 -- src/borg/borg-trait.c
```

### Conditions and limits

Native Human Warrior (strength 17, dexterity 16) on an arena at depth
30, seed one thousand, with a stack of ten identified rapiers in the
pack; the item is parsed through the production borg item analyzer and
the blows are computed by the production borg formula and the engine
formula side by side. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).


## Re-verification (2026-11-08)

Why: the receipt above hashes the original run's phase logs, but they were
never committed (the repository's `*.log` ignore rule dropped them), so the
package could not be checked against its own receipt. The run was repeated
at the same pins (base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `d7b42db02cc74473bc175d2c37dd6feba1a77811`, fixtures
`c2b19026ecc7dcbe95fcc398301fa0d83c064fbc`) with `--full-tests`, and every case reproduced the recorded
outcome exactly (the `cases` block of the new `result.json` equals the
original receipt's):

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `stack` | fail: `borg_blows=1 engine_blows=2` | pass: `borg_blows=2 engine_blows=2` |
| `single` (control) | pass: `borg_blows=2 engine_blows=2` | pass: `borg_blows=2 engine_blows=2` |

Controls byte-identical before/after: `single`. Full native gates from the
`*-alltests.log` summaries: before: Total: 965/965 passed (100.0%), Total: 5/5; after: Total: 965/965 passed (100.0%), Total: 5/5. Toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0; cmake version 3.28.3.

These re-run files have the same sha256 as the original receipt lists, i.e.
they are byte-identical to the original run's: `after-scenarios.log`, `before-scenarios.log`, `result.json`, `before-results.json`, `before-single.log`, `before-stack.log`, `after-results.json`, `after-single.log`, `after-stack.log`. Files that differ
only by the run's temporary paths and build timing: `after-alltests.log`, `after-build.log`, `before-alltests.log`, `before-build.log`, `configure.log`.

Render check: `node tools/c-evidence/render-smoke.js replay.html` on the
committed page (every scenario option and visible button):
`RENDER-SMOKE actions=2 errors=0 OK`.

Logs, `result.json` and a receipt with the sha256 of every re-run file are
in [`reverify-2026-11-08/`](reverify-2026-11-08/). Repeat, both pins fixed
(the fix sha is the runner's built-in `FIX` constant, checked against the
branch diff before building):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/homeblows-313-evidence
git show c2b19026ecc7dcbe95fcc398301fa0d83c064fbc:docs/audit/c-reports/homeblows-313/reproduce.py > /tmp/reproduce-homeblows-313.py
python3 /tmp/reproduce-homeblows-313.py --full-tests --fixtures-ref c2b19026ecc7dcbe95fcc398301fa0d83c064fbc
```

The earlier text of this guide is left as written.

## Errata (2026-11-08)

- **Source proposal link.** The opening's "Source proposal" link above points to a placeholder URL. The source proposal is [#353](https://github.com/diegoami/neo-angband-tracker/pull/353) (branch `task/c-borg-blows-single-weight`); the evidence PR is [#354](https://github.com/diegoami/neo-angband-tracker/pull/354); the canonical issue is [#313](https://github.com/diegoami/neo-angband-tracker/issues/313).
- **Fixtures pins.** The receipt's `fixtures_ref` is `c2b19026ecc7dcbe95fcc398301fa0d83c064fbc` (the commit the native run executed at). The commands above pin `0d3c1316a971270c67c6fed234a0d1ea0fa1c70e` and the issue's commands pinned `d73c67ab573e5406e4993315d846836b3e7b052a`; all three commits carry byte-identical `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py` and `borg.txt` (`git diff --stat c2b19026ecc7dcbe95fcc398301fa0d83c064fbc d73c67ab573e5406e4993315d846836b3e7b052a -- docs/audit/c-reports/homeblows-313/` lists only README/receipt/replay/result artifacts), so any of them reproduces the same run. The re-verification above used `c2b19026ecc7dcbe95fcc398301fa0d83c064fbc`.
- **Missing logs and render record.** The file inventory and receipt list phase logs that were not committed with the original package; the re-run's logs are in `reverify-2026-11-08/`. The receipt records its browser check under `browser`; the generic render-smoke result above is the 2026-11-08 record.
- **README hash.** Because this section was appended, `README.md` no longer matches the `README.md` hash in `receipt.json` (that hash is for the guide before this section was appended); every other hashed file is unchanged.
- **Restored original logs.** The re-run files whose sha256 equals the value in the original `receipt.json` (`after-scenarios.log`, `before-scenarios.log`, `before-single.log`, `before-stack.log`, `after-single.log`, `after-stack.log`) are byte-for-byte the original run's files, so they are also committed here at the paths the receipt names. `after-alltests.log`, `after-build.log`, `before-alltests.log`, `before-build.log`, `configure.log` differ by temporary paths and timing and stay only in `reverify-2026-11-08/`.
