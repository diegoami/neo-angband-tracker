# The light list that outgrew its shelf

When the Borg is in the dark it flows back towards remembered light,
listing every perma-lit grid on the screen into two fixed-size arrays
— with no bound.

**`borg_flow_light()` appends one entry per glowing grid on the
visible panel to `borg_glow_y`/`borg_glow_x`, which hold
`AUTO_LIGHT_MAX` (1536) entries; an ordinary panel can show more
glowing grids than that.** Measured natively on a fully lit arena
panel (1600 glowing grids): the original collects all 1600 —
`overflow=64` — and AddressSanitizer catches the overflowing write as
a `global-buffer-overflow` (`WRITE of size 1`) in the scan itself; the
correction stops collecting at the bound (`glow_n=1536`, no overflow)
and the flow still runs.

Control: a panel with a thousand glowing grids fits the arrays in both
variants — `glow_n=1000`, byte-identical logs.

Consequence: past the bound the writes land in the neighbouring arrays
(the tail of `borg_glow_y` in `borg_glow_x` and vice versa), so the
read-back loop enqueues swapped and off-panel coordinates as flow
targets — the report's source-derived mechanism. The measured facts
here are the unbounded write and the sanitizer-caught overflow; the
live out-of-light trigger is the supplied evidence, not claimed here.
No death or survival claim.

Canonical report: [issue311](https://github.com/diegoami/neo-angband-tracker/issues/311).
Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowoverflow-311-evidence
git show 83a131b3187b125521dcc76a099807c78167dc54:docs/audit/c-reports/glowoverflow-311/reproduce.py > /tmp/reproduce-glowoverflow-311.py
python3 /tmp/reproduce-glowoverflow-311.py --fixtures-ref 83a131b3187b125521dcc76a099807c78167dc54 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` (a normal build and an AddressSanitizer build each), verifies
the source diff is exactly the glow-array bound, runs the overflowing
and control situations per variant through the production out-of-light
flow, runs the overflow under AddressSanitizer, checks the
fail-before and pass-after matrix, the sanitizer report and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The light list that outgrew its shelf**: the original writes 1600
entries into arrays of 1536; the correction stops at the bound.

Actual recorded output, original (`before-glow.log`):

```text
GEOMETRY glow_n=1600 limit=1536
RATING glow_n=1600 overflow=64
```

Corrected (`after-glow.log`): `RATING glow_n=1536 overflow=0`.
AddressSanitizer on the same run (`before-asan-glow.log`):
`WRITE of size 1` — `SUMMARY: AddressSanitizer:
global-buffer-overflow ... in borg_flow_light`; the corrected run
passes with no sanitizer report. Control (`control`): `RATING
glow_n=1000 overflow=0` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`d4dcb983c9623757b523a7fae861b624d858e23d`. One check in
`src/borg/borg-flow-misc.c`. The scan appended every glowing grid on
the panel with no bound; the correction stops collecting once the
arrays are full.

```sh
git fetch --no-tags origin task/c-borg-glow-array-bound
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-flow-misc.c | sed -n '833,850p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 d4dcb983c9623757b523a7fae861b624d858e23d -- src/borg/borg-flow-misc.c
```

### Conditions and limits

Native Human Warrior on a fully perma-lit arena (the Borg map marked
`BORG_GLOW` throughout), seed one thousand, terminal sized so the panel
sits inside the map; the production out-of-light flow runs in full.
The memory-safety proof is the same run built with AddressSanitizer
(the strcmp interceptor is disabled — the pristine base trips an
unrelated interceptor read while sorting monster names at init). No
death or survival claim.

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
at the same pins (base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `d4dcb983c9623757b523a7fae861b624d858e23d`, fixtures
`5f5aa3a857d593d0f053b46c0786256f4d953a0c`) with `--full-tests`, and every case reproduced the recorded
outcome exactly (the `cases` block of the new `result.json` equals the
original receipt's):

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `glow` | fail: `glow_n=1600 overflow=64` | pass: `glow_n=1536 overflow=0` |
| `control` (control) | pass: `glow_n=1000 overflow=0` | pass: `glow_n=1000 overflow=0` |
| `asan` | fail: `WRITE of size 1` SUMMARY: AddressSanitizer: global-buffer-overflow | pass: no sanitizer report |

Controls byte-identical before/after: `control`. Full native gates from the
`*-alltests.log` summaries: before: Total: 965/965 passed (100.0%), Total: 5/5; after: Total: 965/965 passed (100.0%), Total: 5/5. Toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0; cmake version 3.28.3.

These re-run files have the same sha256 as the original receipt lists, i.e.
they are byte-identical to the original run's: `after-build-asan.log`, `after-build.log`, `after-scenarios.log`, `before-control.log`, `before-glow.log`, `after-asan-glow.log`, `after-control.log`, `after-glow.log`, `after-results.json`. Files that differ
only by the run's temporary paths and build timing: `after-alltests.log`, `before-alltests.log`, `before-build-asan.log`, `before-build.log`, `before-scenarios.log`, `configure-asan.log`, `configure.log`, `result.json`, `before-asan-glow.log`, `before-results.json`.

Render check: `node tools/c-evidence/render-smoke.js replay.html` on the
committed page (every scenario option and visible button):
`RENDER-SMOKE actions=2 errors=0 FAILED: rendered text contains "undefined"`.

Logs, `result.json` and a receipt with the sha256 of every re-run file are
in [`reverify-2026-11-08/`](reverify-2026-11-08/). Repeat, both pins fixed
(the fix sha is the runner's built-in `FIX` constant, checked against the
branch diff before building):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowoverflow-311-evidence
git show 5f5aa3a857d593d0f053b46c0786256f4d953a0c:docs/audit/c-reports/glowoverflow-311/reproduce.py > /tmp/reproduce-glowoverflow-311.py
python3 /tmp/reproduce-glowoverflow-311.py --full-tests --fixtures-ref 5f5aa3a857d593d0f053b46c0786256f4d953a0c
```

The earlier text of this guide is left as written.

## Errata (2026-11-08)

- **Source proposal link.** The opening's "Source proposal" link above points to a placeholder URL. The source proposal is [#346](https://github.com/diegoami/neo-angband-tracker/pull/346) (branch `task/c-borg-glow-array-bound`); the evidence PR is [#347](https://github.com/diegoami/neo-angband-tracker/pull/347); the canonical issue is [#311](https://github.com/diegoami/neo-angband-tracker/issues/311).
- **Fixtures pins.** The receipt's `fixtures_ref` is `5f5aa3a857d593d0f053b46c0786256f4d953a0c` (the commit the native run executed at). The commands above pin `83a131b3187b125521dcc76a099807c78167dc54` and the issue's commands pinned `1a9946c6111c541a4041edc7a200e7440434d85e`; all three commits carry byte-identical `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py` and `borg.txt` (`git diff --stat 5f5aa3a857d593d0f053b46c0786256f4d953a0c 1a9946c6111c541a4041edc7a200e7440434d85e -- docs/audit/c-reports/glowoverflow-311/` lists only README/receipt/replay/result artifacts), so any of them reproduces the same run. The re-verification above used `5f5aa3a857d593d0f053b46c0786256f4d953a0c`.
- **Missing logs and render record.** The file inventory and receipt list phase logs that were not committed with the original package; the re-run's logs are in `reverify-2026-11-08/`. The receipt records its browser check under `browser`; the generic render-smoke result above is the 2026-11-08 record.
- **README hash.** Because this section was appended, `README.md` no longer matches the `README.md` hash in `receipt.json` (that hash is for the guide before this section was appended); every other hashed file is unchanged.
- **Consequence paragraph retracted.** The opening's "Consequence" paragraph ("the tail of `borg_glow_y` in `borg_glow_x` and vice versa ... the read-back loop enqueues swapped and off-panel coordinates as flow targets") is not established and is withdrawn (correction note on issue #311, 2026-11-07). What the overflowing write corrupts is undefined behaviour and depends on the build's memory layout. For one analysed build (pristine `e473720`, `cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSUPPORT_BORG=ON`, gcc 15.2.0; `nm -S` places `borg_glow_y`, then `borg_glow_x`, `borg_glow_n`, `borg_light_y`), a probe that simulates that layout and the compiled scan (`docs/audit/c-reports/spec-checks/glow_overflow_probe.c` and `RECHECK.md` §3 on PR #317, branch `task/borg-spec-audit`) gives, for 1817 glowing grids: 279 enqueued grids with the right y and a wrong x, **none off the panel**, and 249 entries of `borg_light_y[]` overwritten. That is a simulation of one build, not a game run; other builds can differ. What this package measures is unchanged: the unbounded write (1600 entries into arrays of 1536) and the AddressSanitizer `global-buffer-overflow` `WRITE of size 1` in `borg_flow_light` before, none after.
- **Render check correction.** The receipt's browser record says "no n/a or undefined fields". The 2026-11-08 render smoke of the same committed `replay.html` finds one: choosing **Control** shows "AddressSanitizer on the same run: undefined", because the control has no sanitizer run and the page has no text for that case. The glow scenario's values render correctly and there are no console errors. The page is left as committed; the correct reading for the control is "no sanitizer run for this case".
- **Restored original logs.** The re-run files whose sha256 equals the value in the original `receipt.json` (`after-build-asan.log`, `after-build.log`, `after-scenarios.log`, `before-control.log`, `before-glow.log`, `after-asan-glow.log`, `after-control.log`, `after-glow.log`) are byte-for-byte the original run's files, so they are also committed here at the paths the receipt names. `after-alltests.log`, `before-alltests.log`, `before-build-asan.log`, `before-build.log`, `before-scenarios.log`, `configure-asan.log`, `configure.log`, `result.json`, `before-asan-glow.log`, `before-results.json` differ by temporary paths and timing and stay only in `reverify-2026-11-08/`.
