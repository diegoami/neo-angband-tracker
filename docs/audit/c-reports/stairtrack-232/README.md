# The staircase he kept discovering

The "do I already know this up staircase?" scan never stopped at a
match — so a known staircase under the Borg was tracked again.

**`borg_think_stair_scum()`'s up-staircase scan used `continue` where
the matching down-staircase scan uses `break`: a match no longer
stopped the loop, the scan always ended at the end of the array, and
the staircase under the Borg — already tracked, for example from being
seen on the map earlier — was appended to `track_less` again on every
visit.** Measured natively through the production discovery path with
the staircase pre-tracked: the original grows the array from 1 to 2 (a
duplicate per visit, until the array fills and later staircases cannot
be tracked); the correction breaks on the match and leaves it at 1.

Control: a fresh, never-tracked staircase is added exactly once in
both variants (0 to 1), byte-identical logs.

Consequence: `track_less` fills with duplicate entries for the Borg's
own staircase; once full, newly seen up staircases can no longer be
tracked. The measured fact is the array growth; no death or survival
claim.

Canonical report: [issue232](https://github.com/diegoami/neo-angband-tracker/issues/232).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairtrack-232-evidence
git show 6a28f95e98d046ddaa0ba29b265df2ba42291090:docs/audit/c-reports/stairtrack-232/reproduce.py > /tmp/reproduce-stairtrack-232.py
python3 /tmp/reproduce-stairtrack-232.py --fixtures-ref 6a28f95e98d046ddaa0ba29b265df2ba42291090 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the break-on-match,
runs the known-stair and fresh-discovery situations per variant
through the production stair-scum path, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The staircase he kept discovering**: the original re-adds the known
staircase; the correction leaves the array alone.

Actual recorded output, original (`before-known.log`):

```text
GEOMETRY before=1 after=2
RATING before=1 after=2
```

Corrected (`after-known.log`): `RATING before=1 after=1`. Control
(`fresh`): `RATING before=0 after=1` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`8310e9cc9fe90a82f93609f542d5f057b835fbcf`. One keyword in
`src/borg/borg-think-dungeon-util.c`: the up-staircase "already known"
scan breaks on a match, like the down-staircase scan above it.

```sh
git fetch --no-tags origin task/c-borg-stair-track-break
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-think-dungeon-util.c | sed -n '395,412p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 8310e9cc9fe90a82f93609f542d5f057b835fbcf -- src/borg/borg-think-dungeon-util.c
```

### Conditions and limits

Native Human Warrior at depth 2, seed one thousand, standing on a real
up staircase while the map belief is still plain floor (the discovery
moment); the production stair-scum path runs in full. The measured
fact is the tracking array growth. No death or survival claim.

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
at the same pins (base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `8310e9cc9fe90a82f93609f542d5f057b835fbcf`, fixtures
`91eb5b3400cc56b407db6c9011e989c216edfac0`) with `--full-tests`, and every case reproduced the recorded
outcome exactly (the `cases` block of the new `result.json` equals the
original receipt's):

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `known` | fail: `before=1 after=2` | pass: `before=1 after=1` |
| `fresh` (control) | pass: `before=0 after=1` | pass: `before=0 after=1` |

Controls byte-identical before/after: `fresh`. Full native gates from the
`*-alltests.log` summaries: before: Total: 965/965 passed (100.0%), Total: 5/5; after: Total: 965/965 passed (100.0%), Total: 5/5. Toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0; cmake version 3.28.3.

These re-run files have the same sha256 as the original receipt lists, i.e.
they are byte-identical to the original run's: `after-build.log`, `after-scenarios.log`, `before-scenarios.log`, `result.json`, `before-fresh.log`, `before-known.log`, `before-results.json`, `after-fresh.log`, `after-known.log`, `after-results.json`. Files that differ
only by the run's temporary paths and build timing: `after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`.

Render check: `node tools/c-evidence/render-smoke.js replay.html` on the
committed page (every scenario option and visible button):
`RENDER-SMOKE actions=2 errors=0 OK`.

Logs, `result.json` and a receipt with the sha256 of every re-run file are
in [`reverify-2026-11-08/`](reverify-2026-11-08/). Repeat, both pins fixed
(the fix sha is the runner's built-in `FIX` constant, checked against the
branch diff before building):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairtrack-232-evidence
git show 91eb5b3400cc56b407db6c9011e989c216edfac0:docs/audit/c-reports/stairtrack-232/reproduce.py > /tmp/reproduce-stairtrack-232.py
python3 /tmp/reproduce-stairtrack-232.py --full-tests --fixtures-ref 91eb5b3400cc56b407db6c9011e989c216edfac0
```

The earlier text of this guide is left as written.

## Errata (2026-11-08)

- **Source proposal link.** The opening's "Source proposal" link above points to a placeholder URL. The source proposal is [#358](https://github.com/diegoami/neo-angband-tracker/pull/358) (branch `task/c-borg-stair-track-break`); the evidence PR is [#359](https://github.com/diegoami/neo-angband-tracker/pull/359); the canonical issue is [#232](https://github.com/diegoami/neo-angband-tracker/issues/232).
- **Fixtures pins.** The receipt's `fixtures_ref` is `91eb5b3400cc56b407db6c9011e989c216edfac0` (the commit the native run executed at). The commands above pin `6a28f95e98d046ddaa0ba29b265df2ba42291090` and the issue's commands pinned `07a71c16084398643ac09da9cc56e3b2b5b6ea6b`; all three commits carry byte-identical `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py` and `borg.txt` (`git diff --stat 91eb5b3400cc56b407db6c9011e989c216edfac0 07a71c16084398643ac09da9cc56e3b2b5b6ea6b -- docs/audit/c-reports/stairtrack-232/` lists only README/receipt/replay/result artifacts), so any of them reproduces the same run. The re-verification above used `91eb5b3400cc56b407db6c9011e989c216edfac0`.
- **Missing logs and render record.** The file inventory and receipt list phase logs that were not committed with the original package; the re-run's logs are in `reverify-2026-11-08/`. The receipt records its browser check under `browser`; the generic render-smoke result above is the 2026-11-08 record.
- **README hash.** Because this section was appended, `README.md` no longer matches the `README.md` hash in `receipt.json` (that hash is for the guide before this section was appended); every other hashed file is unchanged.
- **Restored original logs.** The re-run files whose sha256 equals the value in the original `receipt.json` (`after-build.log`, `after-scenarios.log`, `before-scenarios.log`, `before-fresh.log`, `before-known.log`, `after-fresh.log`, `after-known.log`) are byte-for-byte the original run's files, so they are also committed here at the paths the receipt names. `after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log` differ by temporary paths and timing and stay only in `reverify-2026-11-08/`.
