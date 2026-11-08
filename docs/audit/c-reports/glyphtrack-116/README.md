# The Sea of Runes flow reported no action after queueing a rune read

This is **glyphtrack-116 / issue116**, a C Borg defect
(diegoami/neo-angband-tracker#116).

When the Borg stands on the grid it chose for a rune, the Sea of Runes flow reads a Rune of Protection (or casts or activates the equivalent) and tracks the new rune. If the grid was already in the tracker, the function returned false although the read had just been queued. Its caller treats false as nothing done and goes on to its remaining flows in the same turn, so their keypresses are queued behind the read. A complete sea makes the Borg return to its centre, which is always tracked, so this is the ordinary way to reach it. The correction returns true in that case too: the read is the action.

## What to watch

A step is queued after the rune read (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `d0164206a`) |
| --- | --- | --- |
| Arrival at the centre of a complete sea (the grid is already tracked) | failed, extra_step=1 | passed, extra_step=0 |
| Control: first arrival at a new sea centre | passed, extra_step=n/a | byte-identical log |

- **Arrival at the centre of a complete sea (the grid is already tracked).** The Borg has read a Rune of Protection at the centre of a Sea of Runes and the other 24 grids of the sea hold runes. The flow makes the centre its goal and the Borg arrives there again: the read is queued, and the grid is already in the glyph tracker. Original: the flow returns false, so the next flow in the caller's sequence (here a step towards a potion three grids away) is queued after the read in the same turn. Corrected: the flow returns true and only the read is queued.

- **Control: first arrival at a new sea centre.** The first arrival at the chosen centre: the grid is new, the read is queued, the rune is tracked and the flow returns true, before and after.

## Conditions

Native Human Warrior, seed 116, character level 50, depth 99, a lit open arena, a lit Lantern and 40 Scrolls of Rune of Protection. Fixture interventions: the rune grids are laid by the fixture (the centre after the Borg's first read, and the other 24 grids of the sea at once) as the Borg's earlier reads would have left them, in both the game map and the Borg's map entries; a Potion of Cure Light Wounds is placed in the Borg's object list three grids east, in view; the Borg's map update is not run, so the Borg does not mark itself as sitting in the sea (borg.morgoth_position stays false). The production borg_flow_glyph() makes both calls and the production borg_flow_take() is the next flow in the caller's sequence; the keypress queue is read, not played.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glyphtrack-116
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glyphtrack-116-evidence
git show d98f485ed5f5cab1b52e533ff19749fb10a95c3f:docs/audit/c-reports/glyphtrack-116/reproduce.py > /tmp/reproduce-glyphtrack-116.py
python3 /tmp/reproduce-glyphtrack-116.py --full-tests --fixtures-ref d98f485ed5f5cab1b52e533ff19749fb10a95c3f --fix-ref d0164206a3c1111f386f9d9c13ed72850400a939
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-glyph.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `d0164206a3c1111f386f9d9c13ed72850400a939` (branch `task/glyphtrack-116`), fixtures
  `d98f485ed5f5cab1b52e533ff19749fb10a95c3f` (branch `task/glyphtrack-116-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: new-sea
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

At the centre of a complete Sea of Runes, the original flow queued the rune read and returned false, and the caller's next flow queued a step east behind the read in the same turn (5 keys: read, then step). With the correction the flow returns true and only the read (4 keys) is queued. The first-arrival control is byte-identical.

Low priority: this measures the flow's return value and the keys queued behind the read, not play. The next flow in the caller's sequence that is used here, borg_flow_take(), declines when the Borg already counts itself as sitting in a sea of runes; the fixture does not run the map update that sets that, so it shows what a flow without that guard queues. It does not measure an actual move off the rune, a fight or Morgoth. The correction does not change whether the Borg reads a rune on a rune it already has.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
