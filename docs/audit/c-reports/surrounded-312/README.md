# Surrounded by nobody

The "am I about to be surrounded?" check counted some of the Borg's
neighbours twice — and the verdict can fire with no monsters nearby.

**`borg_surrounded()` tested the trap flag with a separate `if` after
the wall/unknown/monster/shop `else if` chain, so a wall that also
carried the trap flag counted as two non-safe grids. Enough
double-counted neighbours push the safe-square count below zero, and
"monsters > safe squares" then fires with zero monsters.** Measured
natively through the production verdict with the report's board —
three trapped walls, three trap-only floors, no monsters: the original
answers surrounded; the correction makes the trap test the fifth
branch of the chain and answers not surrounded.

Control: eight plain walls with no traps never touch the trap test —
not surrounded in both variants, byte-identical logs.

Consequence: the phantom verdict makes the Borg abandon a fight it is
not in, or flee the level. The measured fact is the wrong verdict; the
live in-game abandonment is the report's mechanism, not claimed here.
No death or survival claim.

Canonical report: [issue312](https://github.com/diegoami/neo-angband-tracker/issues/312).
Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/surrounded-312-evidence
git show 2685a3340819668aa6a23c2d3645f90b89c24721:docs/audit/c-reports/surrounded-312/reproduce.py > /tmp/reproduce-surrounded-312.py
python3 /tmp/reproduce-surrounded-312.py --fixtures-ref 2685a3340819668aa6a23c2d3645f90b89c24721 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the chained trap test,
runs the trap-board and plain-wall situations per variant through the
production surrounded check, checks the fail-before and pass-after
matrix and the byte-identical control, records both full gates and
writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Surrounded by nobody**: with zero monsters nearby the original
answers surrounded; the correction answers no.

Actual recorded output, original (`before-traps.log`):

```text
GEOMETRY mode=traps kills_cnt=0
RATING surrounded=1
```

Corrected (`after-traps.log`): `RATING surrounded=0`. Control
(`walls`): `RATING surrounded=0` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`d366ef4e75e40f17fb293135c05fa74524f2e184`. One keyword in
`src/borg/borg-escape.c`: the trap test becomes the fifth `else if` of
the non-safe-grid chain, so each non-safe neighbour counts once.

```sh
git fetch --no-tags origin task/c-borg-surrounded-elseif
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-escape.c | sed -n '206,237p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 d366ef4e75e40f17fb293135c05fa74524f2e184 -- src/borg/borg-escape.c
```

### Conditions and limits

Native Human Warrior on an empty arena at depth 30, seed one thousand,
with the eight neighbouring grids arranged as the report describes;
the production `borg_surrounded()` verdict is read directly. No death
or survival claim.

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
at the same pins (base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `d366ef4e75e40f17fb293135c05fa74524f2e184`, fixtures
`72e61f67abf33aca68ee7f4e29db9e4e823de306`) with `--full-tests`, and every case reproduced the recorded
outcome exactly (the `cases` block of the new `result.json` equals the
original receipt's):

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `traps` | fail: `surrounded=1` | pass: `surrounded=0` |
| `walls` (control) | pass: `surrounded=0` | pass: `surrounded=0` |

Controls byte-identical before/after: `walls`. Full native gates from the
`*-alltests.log` summaries: before: Total: 965/965 passed (100.0%), Total: 5/5; after: Total: 965/965 passed (100.0%), Total: 5/5. Toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0; cmake version 3.28.3.

These re-run files have the same sha256 as the original receipt lists, i.e.
they are byte-identical to the original run's: `after-scenarios.log`, `before-scenarios.log`, `result.json`, `before-results.json`, `before-traps.log`, `before-walls.log`, `after-results.json`, `after-traps.log`, `after-walls.log`. Files that differ
only by the run's temporary paths and build timing: `after-alltests.log`, `after-build.log`, `before-alltests.log`, `before-build.log`, `configure.log`.

Render check: `node tools/c-evidence/render-smoke.js replay.html` on the
committed page (every scenario option and visible button):
`RENDER-SMOKE actions=2 errors=0 OK`.

Logs, `result.json` and a receipt with the sha256 of every re-run file are
in [`reverify-2026-11-08/`](reverify-2026-11-08/). Repeat, both pins fixed
(the fix sha is the runner's built-in `FIX` constant, checked against the
branch diff before building):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/surrounded-312-evidence
git show 72e61f67abf33aca68ee7f4e29db9e4e823de306:docs/audit/c-reports/surrounded-312/reproduce.py > /tmp/reproduce-surrounded-312.py
python3 /tmp/reproduce-surrounded-312.py --full-tests --fixtures-ref 72e61f67abf33aca68ee7f4e29db9e4e823de306
```

The earlier text of this guide is left as written.

## Errata (2026-11-08)

- **Source proposal link.** The opening's "Source proposal" link above points to a placeholder URL. The source proposal is [#351](https://github.com/diegoami/neo-angband-tracker/pull/351) (branch `task/c-borg-surrounded-elseif`); the evidence PR is [#352](https://github.com/diegoami/neo-angband-tracker/pull/352); the canonical issue is [#312](https://github.com/diegoami/neo-angband-tracker/issues/312).
- **Fixtures pins.** The receipt's `fixtures_ref` is `72e61f67abf33aca68ee7f4e29db9e4e823de306` (the commit the native run executed at). The commands above pin `2685a3340819668aa6a23c2d3645f90b89c24721` and the issue's commands pinned `44334c6fc0ccad023b5c632eef39d955e36c19eb`; all three commits carry byte-identical `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py` and `borg.txt` (`git diff --stat 72e61f67abf33aca68ee7f4e29db9e4e823de306 44334c6fc0ccad023b5c632eef39d955e36c19eb -- docs/audit/c-reports/surrounded-312/` lists only README/receipt/replay/result artifacts), so any of them reproduces the same run. The re-verification above used `72e61f67abf33aca68ee7f4e29db9e4e823de306`.
- **Missing logs and render record.** The file inventory and receipt list phase logs that were not committed with the original package; the re-run's logs are in `reverify-2026-11-08/`. The receipt records its browser check under `browser`; the generic render-smoke result above is the 2026-11-08 record.
- **README hash.** Because this section was appended, `README.md` no longer matches the `README.md` hash in `receipt.json` (that hash is for the guide before this section was appended); every other hashed file is unchanged.
- **Constructed state (caveat from issue #312, 2026-11-07).** The measured board (three trapped walls, three trap-only floors) is a constructed state: the Borg marks a trap only on grids the game reports as a visible trap, which are floor grids, so a wall carrying the trap flag needs the map to change under a remembered trap. The ordinary trigger is a monster (awake or asleep) standing on a known trap, counted once by the monster branch and again by the trap test; a sleeping monster adds to the non-safe count without adding to `monsters`, so four sleeping monsters on traps plus one trapped floor gives `safe_grids = -1` and "surrounded" with no awake monster. That case was worked through in the standalone `docs/audit/c-reports/spec-checks/surrounded_test.c` (PR #317, branch `task/borg-spec-audit`), not run in this native fixture. The opening's "a wall that also carried the trap flag" should be read with this caveat.
- **Restored original logs.** The re-run files whose sha256 equals the value in the original `receipt.json` (`after-scenarios.log`, `before-scenarios.log`, `before-traps.log`, `before-walls.log`, `after-traps.log`, `after-walls.log`) are byte-for-byte the original run's files, so they are also committed here at the paths the receipt names. `after-alltests.log`, `after-build.log`, `before-alltests.log`, `before-build.log`, `configure.log` differ by temporary paths and timing and stay only in `reverify-2026-11-08/`.
