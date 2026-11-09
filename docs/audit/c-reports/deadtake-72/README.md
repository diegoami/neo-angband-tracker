# The Borg's Lunal mode treated forgotten object entries as targets at the corner of the level

This is **deadtake-72 / issue #72**, a C Borg defect
(diegoami/neo-angband-tracker#72).

The Borg keeps a list of the objects it has seen. When one is forgotten its entry is zeroed but stays in the list. The Lunal mode take routine meant to skip such entries, but its test looked at the address of the kind record, which is never NULL, so a forgotten entry was treated like a live one. Its location is (0,0), the corner of the level, and when that grid is in view it became a flow target next to the real items; a Borg near the corner went towards it. The correction tests the kind index, as the other take routines do.

## What to watch

The first step goes towards the gold (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `9492d74ce`) |
| --- | --- | --- |
| A forgotten object entry sits below the live gold | failed, walks_toward_gold=0 | passed, walks_toward_gold=1 |
| Control: the gold is the only entry | passed, walks_toward_gold=1 | byte-identical log |

- **A forgotten object entry sits below the live gold.** A level-12 Warrior stands at (3,3) near the top-left corner of a lit open room, with a pile of gold four grids east. An earlier object was recorded and forgotten with the production delete, so entry 1 of the object list is zeroed (kind 0, location 0,0) and the gold is entry 2. The production Lunal take routine decides which way to go. Original: the dead entry is not skipped, the corner grid (0,0), which is in view, becomes a target next to the gold and is nearer, so the Borg steps north-west towards the corner (goal 2,2). Corrected: the dead entry is skipped and the Borg steps east towards the gold (goal 4,3).

- **Control: the gold is the only entry.** The same Warrior and gold with no dead entry in the list. The Borg steps east towards the gold (goal 4,3) before and after.

## Conditions

Native Human Warrior, seed 72, character level 12, depth 5, a lit open arena, the Borg at (3,3), a free pack slot. Fixture interventions: the object list entries are placed by the fixture (kind, position, value 100), the dead entry by placing a pile of gold and forgetting it with the production borg_delete_take(); every grid of the arena is marked in view, as the lit open room would be; the Borg's map update is not run, so the detection arrays are sized for a two-by-two panel map as it would. The production borg_flow_take_lunal() decides; its keypress is read, not played.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/deadtake-72
git fetch https://github.com/diegoami/neo-angband-tracker.git task/deadtake-72-evidence
git show 6332987b90c9eeb55a4f90014a1e4e22cef18e0e:docs/audit/c-reports/deadtake-72/reproduce.py > /tmp/reproduce-deadtake-72.py
python3 /tmp/reproduce-deadtake-72.py --full-tests --fixtures-ref 6332987b90c9eeb55a4f90014a1e4e22cef18e0e --fix-ref 9492d74ce57ae63fca41866113bc7f35463c5704
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-take.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `9492d74ce57ae63fca41866113bc7f35463c5704` (branch `task/deadtake-72`), fixtures
  `6332987b90c9eeb55a4f90014a1e4e22cef18e0e` (branch `task/deadtake-72-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: live-only
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With a forgotten entry in the object list and the gold four grids east, the original Lunal take routine targeted the corner grid (0,0) and queued a step north-west (goal 2,2); corrected, it queues a step east towards the gold (goal 4,3). With the gold as the only entry the step is east before and after, and that control is byte-identical.

Priority low: Lunal mode only runs with borg_self_lunal on (off by default), the Borg must be within view of the corner of the level, and the open pull on a real level depends on the route; this measures the take routine's first step in one scene. Fixing item_bad (a separate finding) would also skip forgotten entries, since they are marked bad; this change is independent of it and correct on its own. The object list entries are placed by the fixture.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
