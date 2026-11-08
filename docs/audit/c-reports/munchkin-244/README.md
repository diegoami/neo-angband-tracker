# The staircase the Borg was not standing on

A young Borg in munchkin mode stands on plain floor and remembers a down
staircase one diagonal step to the north-west — the grid the
adjacent-monster scan examines last. A second, seen stair is sealed
inside a permanent-wall pocket so every stair flow fails and the
decision reaches the take-the-stair tests.

**The adjacent-monster scan reuses `ag`, the pointer to the Borg's own
grid, for each neighbour, so after the scan it points at the last
in-bounds neighbour. The take-the-stair tests that follow read that
neighbour's feature: the Borg queues the take-stair key while standing
on plain floor.** In the game that keypress fails without taking a
turn, so the Borg can repeat the same decision indefinitely.

The correction re-acquires the Borg's own grid after the scan; the
phantom keypress disappears and the dungeon brain moves on to its next
goal.

Control: standing on a real, seen down staircase, both versions queue
the take-stair key and the complete log is byte-identical before and
after — legitimate stair-taking is handled before the scan (the
power-diving section) and is untouched by the correction. This also
documents why the missed-stair direction of the report is shadowed in
practice: a Borg standing on a staircase is served by the earlier
sections, which read the pointer before the scan clobbers it.

Consequence: with the munchkin configuration enabled (a supported
`borg.txt` option, off in the stock configuration) and a young Borg
(max character level under 12), a remembered stair on the last-scanned
neighbour produces a phantom stair keypress on floor — the seed of a
repeat-forever stuck state. No death or survival claim is made.

Canonical report: [issue244](https://github.com/diegoami/neo-angband-tracker/issues/244).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [#301](https://github.com/diegoami/neo-angband-tracker/pull/301).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/munchkin-244-evidence
git show 4873ed2a9e2992752b603758c058ef0940068054:docs/audit/c-reports/munchkin-244/reproduce.py > /tmp/reproduce-munchkin-244.py
python3 /tmp/reproduce-munchkin-244.py --fixtures-ref 4873ed2a9e2992752b603758c058ef0940068054 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the own-grid
re-acquire, runs two recorded situations per variant (the fixture
profile enables `borg_munchkin_start`), checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A staircase the Borg was not standing on**: the original queues
`>` while standing on floor; the correction queues no stair key.

Actual recorded output, original (`before-phantom.log`):

```text
MODE phantom
GEOMETRY player=(11,11) own_feat=1 nw_feat=6 far_stair=(30,30) track_more=1
RATING acted=1 keys=">" stair_key=62
OUTCOME acted=1 stair_key=62 own_is_stair=0
```

Corrected (`after-phantom.log`): `RATING acted=1 keys="" stair_key=0`,
`OUTCOME acted=1 stair_key=0 own_is_stair=0`. Control (`on-stair`):
`keys=">"` with `own_is_stair=1` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`5b31e13d1023a64bf4f8613352e738757b5f56ec`. Five lines in
`src/borg/borg-think-dungeon.c`. The adjacent-monster scan in
`borg_think_dungeon_munchkin()` assigns `ag = &borg_grids[y][x]` for
each of the eight neighbours, so after the scan `ag` points at the
last in-bounds neighbour instead of the Borg's own grid; the
"Take the Up Stair" and "Take the Stair" tests that follow compare
`ag->feat` against the staircase features. The correction re-points
`ag` at the Borg's own grid between the scan and the tests. The
power-climbing section above the scan already uses a correct local
re-acquire (`tmp_ag`) — the same pattern, applied to the section below
the scan.

```sh
git fetch --no-tags origin task/c-borg-munchkin-stair-grid
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-think-dungeon.c | sed -n '836,920p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 5b31e13d1023a64bf4f8613352e738757b5f56ec -- src/borg/borg-think-dungeon.c
```

### Conditions and limits

Native Human Warrior level 1, depth 30, seed 1000, lit constructed
40x40 arena, birth equipment only (rapier, torches wielded so nothing
in the pack is worth wearing), munchkin mode enabled in the Borg
configuration with the dispatch gate it requires (max character level
under 12). One seen stair is sealed inside a permanent-wall pocket so
every stair flow fails and the decision reaches the take-the-stair
tests; a remembered-but-untracked down stair sits on the north-west
neighbour, the grid the scan examines last. The scenario calls the
public `borg_think_dungeon()` and collects every keypress it queues.
This isolates the stale-pointer test, not full autonomous play; the
measured claim is the phantom keypress itself, not the full loop it
can seed. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration; the runner enables the munchkin
option when staging it), `result.json`/`receipt.json` (outcomes and
hashes), and the captured phase logs. [Full
receipt](receipt.json).


## Re-verification (2026-11-08)

Why: the original run's phase, build and gate logs were listed and hashed in
`receipt.json` but never committed (the repository's `*.log` ignore rule
dropped them), so the receipt could not be checked against published bytes.
The same pinned runner was run again on 2026-11-08 at the receipt's own pins
(base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `5b31e13d1023a64bf4f8613352e738757b5f56ec`, fixtures `e735a4fcedda19cac11f493b8c429893cf19a553`) with `--full-tests`.
Every case reproduced the recorded outcome; the runner's per-case results
(`result.json` `cases`) are identical to the committed original.

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `phantom` (plain floor, remembered stair on the NW neighbour) | `keys=">" stair_key=62 own_is_stair=0` (take-stair key queued on floor); 0/1 | `keys="" stair_key=0`; 1/1 |
| `on-stair` (control) | `keys=">"`, `own_is_stair=1`; 1/1 | byte-identical log |

Gates from the re-run's alltests logs: before Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines); after Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines).
Re-run toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, cmake version 3.28.3 (the original receipt records
gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3; the toolchain differs, and the files listed
below as byte-identical are identical nonetheless).
Corrected source sha256 matches the original receipt.
Render check of the committed `replay.html` with `tools/c-evidence/render-smoke.js`
(Playwright Chromium, every option and button clicked): `RENDER-SMOKE actions=4 errors=0 OK`.
The original receipt records its own headless check under the `browser` key.
The re-run at the receipt pin completed (exit 0). Its regenerated `replay-data.json` is byte-identical to the committed one; its regenerated `replay.html` differs because `make_replay.py` changed after the run pin (see Errata). The generator from `4873ed2a9` regenerates the committed `replay.html` byte-identically from this re-run's logs, so the repeat command below pins `4873ed2a9`, the guide's own pin.

Where the logs are: [`reverify-2026-11-08/`](reverify-2026-11-08/) holds every log,
`result.json` and per-mode results of the re-run, with
[`reverify-2026-11-08/receipt.json`](reverify-2026-11-08/receipt.json) (pins, per-case
results, gate lines, toolchain, render result, sha256 of every file there).
9 re-run files are byte-identical to the hashes in the original
`receipt.json`; those not already committed were also placed at their original paths in this directory, so the original receipt now resolves for them: `after-on-stair.log`, `after-phantom.log`, `after-scenarios.log`, `before-on-stair.log`, `before-phantom.log`, `before-scenarios.log`.
The rest (`after-alltests.log`, `after-build.log`, `before-alltests.log`, `before-build.log`, `configure.log`) differ in
run-specific content (temporary paths, timings, build output of the different toolchain) and are
published only under `reverify-2026-11-08/`.

Repeat (fixtures pinned by sha; the fix sha is fixed inside `reproduce.py` at that pin, and the runner
refuses any other source diff):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/munchkin-244-evidence
git show 4873ed2a9e2992752b603758c058ef0940068054:docs/audit/c-reports/munchkin-244/reproduce.py > /tmp/reproduce-munchkin-244.py
python3 /tmp/reproduce-munchkin-244.py --full-tests --fixtures-ref 4873ed2a9e2992752b603758c058ef0940068054
```

## Errata (2026-11-08)

- `make_replay.py` changed after the run pin `e735a4fce`: the capture commit `4873ed2a9` changes one expression so the replay's "Queued keys" panel reads the recorded `rating` field (at the run pin it read a non-existent `RATING` field and showed `n/a`). No other generator changed between `e735a4fce` and `f5b3c50e2`; no native log differs.
- The source-proposal link near the top of this README was a placeholder; it now points to the source PR [#301](https://github.com/diegoami/neo-angband-tracker/pull/301) (evidence PR #302, canonical issue #244).
- `receipt.json` was amended on 2026-11-08 for this only: its `README.md` hash now covers the README with these sections and the link fix, and a `render_check` key records the smoke check above; the old→new values are listed under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
