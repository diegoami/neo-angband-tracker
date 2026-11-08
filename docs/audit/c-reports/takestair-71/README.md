# A far staircase put an item next to the Borg off its leash

This is **takestair-71 / issue #71**, a C Borg defect
(diegoami/neo-angband-tracker#71).

A low-level Borg keeps item hunts within a leash of its escape staircase. borg_flow_take(), borg_flow_take_scum() and borg_flow_take_lunal() each look up the nearest known up staircase for that test, but the scan kept the farthest one. When the Borg knew a second up staircase far away, a Flask of Oil three steps from the Borg (five from the staircase it was standing by) was measured against the far staircase, judged too far from the stairs, and ignored by all three pickup flows. The correction keeps the nearest up staircase, and the Borg steps toward the flask.

## What to watch

Steps toward the flask (1 = key 6, east); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `97c424892`) |
| --- | --- | --- |
| borg_flow_take(true, 250): up staircases 2 and 55 steps away | failed, goes_for_item=0 | passed, goes_for_item=1 |
| borg_flow_take_scum(true, 6): same layout | failed, goes_for_item=0 | passed, goes_for_item=1 |
| borg_flow_take_lunal(true, 4): same layout | failed, goes_for_item=0 | passed, goes_for_item=1 |
| Control: only the near up staircase (borg_flow_take) | passed, goes_for_item=1 | byte-identical log |

- **borg_flow_take(true, 250): up staircases 2 and 55 steps away.** Original: the staircase 55 steps away is the reference, the flask is 52 flow steps from it (over the 12-step leash) and is skipped; the flow returns false. Corrected: the staircase two steps away is the reference (flask 5 steps from it); 'Flowing toward item at cost 3', key 6 (east).

- **borg_flow_take_scum(true, 6): same layout.** Original: borg_flow_far_from_stairs() measures from the far staircase and the flask is skipped. Corrected: 'Flowing toward Scum item at cost 3', key 6.

- **borg_flow_take_lunal(true, 4): same layout.** Original: the flask is skipped as too far from the (far) staircase. Corrected: 'Flowing toward munchkin item at cost 3', key 6.

- **Control: only the near up staircase (borg_flow_take).** With a single staircase nearest and farthest are the same; both versions step toward the flask and the logs are byte-identical.

## Conditions

Native Human Warrior, seed 71, character level 1, depth 1 (pickup and explore leash both 12), starting kit plus a lit torch, a lit 70x22 arena. An up staircase two steps west of the Borg; in the main cases a second up staircase 55 steps east, both tracked by the Borg. A real Flask of Oil lies on the floor three steps east and is registered through the production observe_take_diff(). Fixture interventions: the Borg's map is copied from the game map and marked viewed, the staircases are written to track_less, and borg.panels is set to 1x1 (what borg_update() derives for a one-panel arena) before the detection arrays are allocated. The production pickup flow runs once and the first queued key is read back. This isolates the pickup decision; it does not play out the walk or the pickup.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/takestair-71
git fetch https://github.com/diegoami/neo-angband-tracker.git task/takestair-71-evidence
git show 50e97a3cef4d309fd459352de421aee670f86e3a:docs/audit/c-reports/takestair-71/reproduce.py > /tmp/reproduce-takestair-71.py
python3 /tmp/reproduce-takestair-71.py --full-tests --fixtures-ref 50e97a3cef4d309fd459352de421aee670f86e3a --fix-ref 97c42489220d39e029568717686acf488c5ad56a
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-take.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `97c42489220d39e029568717686acf488c5ad56a` (branch `task/takestair-71`), fixtures
  `50e97a3cef4d309fd459352de421aee670f86e3a` (branch `task/takestair-71-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: control
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The 'nearest up stairs' scans in borg_flow_take(), borg_flow_take_scum() and borg_flow_take_lunal() keep the farthest known up staircase ('skip the closer ones'), so the leash tests measure items against a distant staircase. A level-1 Warrior standing by an up staircase ignores a Flask of Oil three steps away in all three pickup flows because a second up staircase is known 55 steps away. Keeping the nearest staircase restores the pickup; the single-staircase control is byte-identical.

Not measured: how often a low-level Borg knows several up staircases at once in real play, or the opposite case where the far reference lets the Borg chase items it should leave (under the first borg_flow_take() leash test the far reference disables the 'close to stair' skip). The same farthest-staircase scan in borg_flow_dark() (also named in the report) is not changed here; it is a separate function in another file and would be a separate change.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
