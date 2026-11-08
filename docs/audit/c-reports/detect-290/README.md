# The detection that outlived its cave

The Borg updates on the second panel row of a large cave, queues the
self-reaction of a trap detection — the same call the spell casting path
uses — and then the level changes to a cave exactly one panel high: the
height where the old-offset reset, which compares strictly, lets equal
offsets through while the panel count yields a single row pair.

**The queued reaction still carries the old panel offsets, and its
four-cell mark is indexed with them after the update resized the
detection arrays for the new cave.** The mark reads its row pointer one
row past the outer array and writes through it: the recorded native run
of the original **crashes** (exit 139, matching the supplied harness
runs of this scenario), and the AddressSanitizer variant names the
out-of-bounds row-pointer read in `borg_handle_self()`
(borg-update.c:1396, reached from the update's message loop).

The correction drops the marks of reactions whose panel lies outside
the current arrays — in `borg_handle_self()` and in the trap-creation
reset inside `borg_update()` that indexes with the same old offsets.
The stale reaction is dropped and the small cave's fresh detection
stamps stay clear.

Control: when the reaction is queued *after* the level change, with the
current offsets, it marks the panel legitimately in both variants
(`last_row_mark=1`) and the complete log is byte-identical — the
correction drops only reactions whose panel lies outside the current
arrays.

Consequence: a crash of the whole game in the turn after a level change
following a self-detection. No death or survival claim is made.

Canonical report: [issue290](https://github.com/diegoami/neo-angband-tracker/issues/290).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [#306](https://github.com/diegoami/neo-angband-tracker/pull/306).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/detect-290-evidence
git show bab03cb2204389ddad6ce017ae84614fdd39fc66:docs/audit/c-reports/detect-290/reproduce.py > /tmp/reproduce-detect-290.py
python3 /tmp/reproduce-detect-290.py --fixtures-ref bab03cb2204389ddad6ce017ae84614fdd39fc66 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720` in a normal and an AddressSanitizer build, verifies the
source diff is exactly the panel-bounds guards, runs two recorded
situations per variant per build, checks the crash/pass matrix, the
sanitizer report, the byte-identical control and both full gates, and
writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A reaction queued for the previous, larger cave**: the original
crashes (the sanitizer names the row-pointer read past the arrays); the
correction drops the stale mark and completes.

Actual recorded output, original (`normal/before-stale.log`): the
process dies with `exit=139` (segmentation fault) after
`MODE stale`/`QUEUED`. The sanitizer variant (`asan/before-stale.log`):

```text
ERROR: AddressSanitizer: heap-buffer-overflow
    #0 ... in borg_handle_self src/borg/borg-update.c:1396
    #1 ... in borg_update src/borg/borg-update.c:1676
... is located 0 bytes after 16-byte region
```

Corrected (`normal/after-stale.log`): `GEOMETRY new_cave=28x28
panels=1,1 last_row_mark=0`, `Passed`. Control (`fresh`):
`last_row_mark=1` in both variants, byte-identical logs.

Full native gates (original and corrected, normal build,
`before-alltests.log` and `after-alltests.log`): **965/965 unit checks
and 5/5 frontend tests**, no relevant skips, checked from actual run
summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`02fb00af6abdb165fc1b445c42dc85d2263a3cdd`. One file,
`src/borg/borg-update.c`. `borg_handle_self()` computes the panel from
the old offsets `o_w_x`/`o_w_y` — saved at the end of the previous
update — and every detection branch writes the four cells
`[q_y..q_y+1][q_x..q_x+1]`. After a level change,
`borg_update()` resizes the arrays for the new cave
(`borg_alloc_detection()`, sized `panels + 1` per axis) and processes
the still-queued reaction with the stale offsets. The only guard,
`if (o_w_x > cave->width || o_w_y > cave->height)`, compares strictly:
offsets equal to the new cave's dimensions survive it while the derived
panel index already lands one row past the arrays. The correction
computes `detect_panel_ok` from the panel indices against the current
panel counts and guards every detection branch with it, and applies the
same bound to the trap-creation reset inside `borg_update()` that
indexes with the same old offsets.

```sh
git fetch --no-tags origin task/c-borg-detect-panel-bounds
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-update.c | sed -n '1316,1340p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 02fb00af6abdb165fc1b445c42dc85d2263a3cdd -- src/borg/borg-update.c
```

### Conditions and limits

Native Human Warrior level one, seed one thousand, a lit arena sized
from the panel geometry of an eighty-by-thirty term (panel height
twenty-eight): a large cave spanning several panel rows with the view
pushed to the second row, then a cave exactly one panel high. The
reaction is queued with the same call the spell casting path uses, the
level change replaces the cave as the game does, and the scenario reads
the resized arrays afterwards. The AddressSanitizer variant names the
out-of-bounds row-pointer read in `borg_handle_self`; the plain variant
records the crash. This isolates the queued-reaction bounds, not full
autonomous play. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs (with the sanitizer
logs). [Full receipt](receipt.json).


## Re-verification (2026-11-08)

Why: the original run's phase, build and gate logs were listed and hashed in
`receipt.json` but never committed (the repository's `*.log` ignore rule
dropped them), so the receipt could not be checked against published bytes.
The same pinned runner was run again on 2026-11-08 at the receipt's own pins
(base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `02fb00af6abdb165fc1b445c42dc85d2263a3cdd`, fixtures `b7eb76c64b6702ba6d4aa1ce5ec3a1b27472e60a`) with `--full-tests` (normal and AddressSanitizer builds).
Every case reproduced the recorded outcome; the runner's per-case results
(`result.json` `cases`) are identical to the committed original.

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `stale` (normal; queued detection after the level shrank) | segmentation fault (exit -11) before the scenario finishes; 0/1 | stale mark dropped, `last_row_mark=0 rows=2`; 1/1 |
| `stale` (asan) | heap-buffer-overflow `READ of size 8` in `borg_handle_self` (`borg-update.c:1396`); 0/1 | 1/1, no sanitizer report |
| `fresh` (control) | `last_row_mark=1`; 1/1 | byte-identical log (normal and asan) |

Gates from the re-run's alltests logs: before Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines); after Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines).
Re-run toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, cmake version 3.28.3 (the original receipt records
gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3; the toolchain differs, and the files listed
below as byte-identical are identical nonetheless).
Corrected source sha256 matches the original receipt.
Render check of the committed `replay.html` with `tools/c-evidence/render-smoke.js`
(Playwright Chromium, every option and button clicked): `RENDER-SMOKE actions=4 errors=0 OK`.
The original receipt records its own headless check under the `browser` key.
The regenerated `replay.html` and `replay-data.json` are byte-identical to the committed ones. Normal-build logs are restored under `normal/`, asan logs under `asan/`, as the original receipt names them. The repeat command below pins the guide's own pin `bab03cb22` (generators identical to the run pin `b7eb76c64`).

Where the logs are: [`reverify-2026-11-08/`](reverify-2026-11-08/) holds every log,
`result.json` and per-mode results of the re-run, with
[`reverify-2026-11-08/receipt.json`](reverify-2026-11-08/receipt.json) (pins, per-case
results, gate lines, toolchain, render result, sha256 of every file there).
13 re-run files are byte-identical to the hashes in the original
`receipt.json`; those not already committed were also placed at their original paths in this directory, so the original receipt now resolves for them: `after-asan-build.log`, `normal/after-fresh.log`, `normal/after-stale.log`, `asan/after-fresh.log`, `asan/after-stale.log`, `asan/before-fresh.log`, `normal/before-fresh.log`, `normal/before-stale.log`.
The rest (`after-alltests.log`, `asan/before-stale.log`, `before-alltests.log`, `before-asan-build.log`, `configure-asan.log`, `configure.log`) differ in
run-specific content (temporary paths, timings, build output of the different toolchain, sanitizer addresses and process ids) and are
published only under `reverify-2026-11-08/`.

Repeat (fixtures pinned by sha; the fix sha is fixed inside `reproduce.py` at that pin, and the runner
refuses any other source diff):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/detect-290-evidence
git show bab03cb2204389ddad6ce017ae84614fdd39fc66:docs/audit/c-reports/detect-290/reproduce.py > /tmp/reproduce-detect-290.py
python3 /tmp/reproduce-detect-290.py --full-tests --fixtures-ref bab03cb2204389ddad6ce017ae84614fdd39fc66
```

## Errata (2026-11-08)

- The source-proposal link near the top of this README was a placeholder; it now points to the source PR [#306](https://github.com/diegoami/neo-angband-tracker/pull/306) (evidence PR #307, canonical issue #290).
- `asan/before-stale.log` (the sanitizer crash report) cannot be reproduced byte for byte: it contains process ids and heap addresses. The re-run's report (same error class, access size and site) is `reverify-2026-11-08/asan-before-stale.log`.
- `receipt.json` was amended on 2026-11-08 for this only: its `README.md` hash now covers the README with these sections and the link fix, and a `render_check` key records the smoke check above; the old→new values are listed under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
