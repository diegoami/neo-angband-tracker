# The Borg kept a ghost monster that had left the west edge of its light

This is **lightwest-40 / issue40**, a C Borg defect
(diegoami/neo-angband-tracker#40).

The Borg keeps a map of the grids its own light reaches and uses it to decide whether it should be able to see a monster it last saw on a grid. The west part of the radius-2 block listed the grids two rows above and below (which the north and south blocks add already) instead of the grids one row above and below, so the two grids one row up and down and two columns west were never marked as lit. A monster last seen there was treated as unseen rather than gone, so the Borg kept it in its monster list. The correction lists the grids one row above and below, like the east block.

## What to watch

The vanished monster stays in the Borg's list (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `6dd5874b4`) |
| --- | --- | --- |
| Monster last seen two columns west and one row south | failed, ghost_kept=1 | passed, ghost_kept=0 |
| Control: the same monster two columns east and one row south | passed, ghost_kept=0 | byte-identical log |

- **Monster last seen two columns west and one row south.** A level-20 Warrior with a Lantern stands in a dark room. A cave bear was last seen two columns west and one row south of it, in the lantern light, and has since walked away. The Borg's own light map is updated by the production routine and then the production follow-up for the vanished monster runs. Original: the grid is not marked as lit, so the Borg does not expect to see the bear there and keeps it as a ghost in its monster list. Corrected: the grid is lit, the follow-up runs and the ghost is forgotten.

- **Control: the same monster two columns east and one row south.** The mirror position on the east side, which the original already lights. The follow-up forgets the monster before and after.

## Conditions

Native Human Warrior, seed 40, character level 20, a Lantern, depth 5, a dark open arena (no lit room). A cave bear was last seen on the grid two columns west and one row south of the Borg (control: two columns east); the game monster is then removed and the Borg's record of it kept. The production borg_update_light() builds the light map and the production borg_follow_kill() handles the vanished monster. Fixture interventions: the monster is placed, observed and removed by the fixture; the Borg's map update is not run.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lightwest-40
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lightwest-40-evidence
git show 483a7a12c8470d43f890ac64f62f666691690a42:docs/audit/c-reports/lightwest-40/reproduce.py > /tmp/reproduce-lightwest-40.py
python3 /tmp/reproduce-lightwest-40.py --full-tests --fixtures-ref 483a7a12c8470d43f890ac64f62f666691690a42 --fix-ref 6dd5874b444b00b8185fce58c823002ab6c544ad
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-cave-light.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `6dd5874b444b00b8185fce58c823002ab6c544ad` (branch `task/lightwest-40`), fixtures
  `483a7a12c8470d43f890ac64f62f666691690a42` (branch `task/lightwest-40-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: east
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With the monster last seen on the missing west grid, the original Borg did not treat the grid as lit and kept the monster as a ghost; with the grid lit the follow-up forgets it, as it does for the mirror grid on the east side, which is byte-identical.

Priority low: this changes whether a monster that left one of two grids is remembered; it does not measure a fight or danger. The forgetting itself is the existing follow-up behaviour (see #96). The report's other point, duplicate entries at radius 4 and above, is not changed.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
