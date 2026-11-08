# The Phial that shone twice as bright

Noticing the equipment added an item's light modifier to the Borg's
light radius twice — once before and once after the flag adjustments.

**`borg_notice_equipment()` adds `modifiers[OBJ_MOD_LIGHT]` to
`BI_LIGHT` on both sides of the `LIGHT_2`/`LIGHT_3` and Necromancer
unlight adjustments, so any item with a real light modifier — the
artifact lights — lit twice as much in the Borg's mind as in the
dungeon.** Measured natively with the Phial of Galadriel (LIGHT[4]):
the original reports light radius 8; the correction counts the
modifier once and reports 4.

Control: a stock Wooden Torch draws its radius from the `LIGHT_2`
flag, not a modifier — light 2 in both variants, byte-identical logs.

Consequence: the Borg overestimates its light radius by the modifier,
feeding every darkness and out-of-light decision with an inflated
value. No crash; no death or survival claim.

Canonical report: [issue238](https://github.com/diegoami/neo-angband-tracker/issues/238).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/light-238-evidence
git show 488c10497b9ed995d5eed18cea196b720ae4845b:docs/audit/c-reports/light-238/reproduce.py > /tmp/reproduce-light-238.py
python3 /tmp/reproduce-light-238.py --fixtures-ref 488c10497b9ed995d5eed18cea196b720ae4845b --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the removal of the
doubled add, runs the Phial and torch situations per variant through
the production equipment notice, checks the fail-before and pass-after
matrix and the byte-identical control, records both full gates and
writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The Phial that shone twice as bright**: the original counts the
modifier twice (radius 8 for LIGHT[4]); the correction counts it once
(4).

Actual recorded output, original (`before-phial.log`):

```text
GEOMETRY light_mod=4 bi_light=8
RATING bi_light=8
```

Corrected (`after-phial.log`): `RATING bi_light=4`. Control
(`torch`): `RATING bi_light=2` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`66b183340a567811051a5745c8d3a53792963860`. Two deleted lines in
`src/borg/borg-trait.c`: the stray second
`borg.trait[BI_LIGHT] += item->modifiers[OBJ_MOD_LIGHT];` after the
flag and unlight adjustments.

```sh
git fetch --no-tags origin task/c-borg-light-mod-once
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-trait.c | sed -n '1508,1530p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 66b183340a567811051a5745c8d3a53792963860 -- src/borg/borg-trait.c
```

### Conditions and limits

Native Human Warrior on a lit arena at depth 30, seed one thousand,
wearing the Phial of Galadriel (created through the artifact data) or
a stock Wooden Torch; the Borg's equipment notice runs in full. The
measured value is the Borg's own light-radius trait; the consequence
is an inflated radius in darkness and out-of-light decisions, not a
crash — no death or survival claim.

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
at the same pins (base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `66b183340a567811051a5745c8d3a53792963860`, fixtures
`3c7b2c8076be2afbbb5a39857ccb851466de92c8`) with `--full-tests`, and every case reproduced the recorded
outcome exactly (the `cases` block of the new `result.json` equals the
original receipt's):

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `phial` | fail: `bi_light=8` | pass: `bi_light=4` |
| `torch` (control) | pass: `bi_light=2` | pass: `bi_light=2` |

Controls byte-identical before/after: `torch`. Full native gates from the
`*-alltests.log` summaries: before: Total: 965/965 passed (100.0%), Total: 5/5; after: Total: 965/965 passed (100.0%), Total: 5/5. Toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0; cmake version 3.28.3.

These re-run files have the same sha256 as the original receipt lists, i.e.
they are byte-identical to the original run's: `after-build.log`, `after-scenarios.log`, `before-scenarios.log`, `result.json`, `before-phial.log`, `before-results.json`, `before-torch.log`, `after-phial.log`, `after-results.json`, `after-torch.log`. Files that differ
only by the run's temporary paths and build timing: `after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`.

Render check: `node tools/c-evidence/render-smoke.js replay.html` on the
committed page (every scenario option and visible button):
`RENDER-SMOKE actions=2 errors=0 OK`.

Logs, `result.json` and a receipt with the sha256 of every re-run file are
in [`reverify-2026-11-08/`](reverify-2026-11-08/). Repeat, both pins fixed
(the fix sha is the runner's built-in `FIX` constant, checked against the
branch diff before building):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/light-238-evidence
git show 3c7b2c8076be2afbbb5a39857ccb851466de92c8:docs/audit/c-reports/light-238/reproduce.py > /tmp/reproduce-light-238.py
python3 /tmp/reproduce-light-238.py --full-tests --fixtures-ref 3c7b2c8076be2afbbb5a39857ccb851466de92c8
```

The earlier text of this guide is left as written.

## Errata (2026-11-08)

- **Source proposal link.** The opening's "Source proposal" link above points to a placeholder URL. The source proposal is [#349](https://github.com/diegoami/neo-angband-tracker/pull/349) (branch `task/c-borg-light-mod-once`); the evidence PR is [#350](https://github.com/diegoami/neo-angband-tracker/pull/350); the canonical issue is [#238](https://github.com/diegoami/neo-angband-tracker/issues/238).
- **Fixtures pins.** The receipt's `fixtures_ref` is `3c7b2c8076be2afbbb5a39857ccb851466de92c8` (the commit the native run executed at). The commands above pin `488c10497b9ed995d5eed18cea196b720ae4845b` and the issue's commands pinned `f1e899b66b383b19ab2d69b4d7ffbb2148ba5444`; all three commits carry byte-identical `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py` and `borg.txt` (`git diff --stat 3c7b2c8076be2afbbb5a39857ccb851466de92c8 f1e899b66b383b19ab2d69b4d7ffbb2148ba5444 -- docs/audit/c-reports/light-238/` lists only README/receipt/replay/result artifacts), so any of them reproduces the same run. The re-verification above used `3c7b2c8076be2afbbb5a39857ccb851466de92c8`.
- **Missing logs and render record.** The file inventory and receipt list phase logs that were not committed with the original package; the re-run's logs are in `reverify-2026-11-08/`. The receipt records its browser check under `browser`; the generic render-smoke result above is the 2026-11-08 record.
- **README hash.** Because this section was appended, `README.md` no longer matches the `README.md` hash in `receipt.json` (that hash is for the guide before this section was appended); every other hashed file is unchanged.
- **Restored original logs.** The re-run files whose sha256 equals the value in the original `receipt.json` (`after-build.log`, `after-scenarios.log`, `before-scenarios.log`, `before-phial.log`, `before-torch.log`, `after-phial.log`, `after-torch.log`) are byte-for-byte the original run's files, so they are also committed here at the paths the receipt names. `after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log` differ by temporary paths and timing and stay only in `reverify-2026-11-08/`.
