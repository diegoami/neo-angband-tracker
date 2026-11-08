# The Borg did not light dark corridors to the north or south in most of the dungeon

This is **lightbeam-88 / issue #88**, a C Borg defect
(diegoami/neo-angband-tracker#88).

When a corridor ahead is unlit, the Borg can cast Spear of Light or aim a Wand or Rod of Light along it. A north or south beam is skipped when the Borg is "on the edge of shifting the panel". The test compared the Borg's row in the whole dungeon with the height of the visible map panel instead of its row on the panel, so on a 22-row panel a north or south beam was refused everywhere below dungeon row 20. The correction subtracts the panel offset, as the panel-shift code in borg-fight-defend.c already does.

## What to watch

The Borg aims the light beam (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `828318ab5`) |
| --- | --- | --- |
| Dark corridor ahead to the north, map panel scrolled to row 24 (dungeon row 30) | failed, beam_cast=0 | passed, beam_cast=1 |
| The same with the unknown grid to the south | failed, beam_cast=0 | passed, beam_cast=1 |
| Control: dungeon row 10, panel at the top | passed, beam_cast=1 | byte-identical log |

- **Dark corridor ahead to the north, map panel scrolled to row 24 (dungeon row 30).** A Warrior with a Wand of Light stands at dungeon row 30, which is row 6 of a 22-row map panel scrolled to row 24. The grid just beyond the torch light to the north is unknown, so the Borg wants to light the corridor. Original: the panel-edge test compares row 30 with 22 - 2 = 20 and refuses. Corrected: it compares row 6 and aims the wand north (keys a e 8).

- **The same with the unknown grid to the south.** The same position with the unknown grid to the south. Original: refused for the same reason. Corrected: the wand is aimed south (keys a e 2).

- **Control: dungeon row 10, panel at the top.** The same corridor at dungeon row 10 with the panel at row 0, where the absolute row and the row on the panel are the same. The wand is aimed north before and after, with an identical log.

## Conditions

Native Human Warrior, seed 88, depth 5, a 60-row open arena lit and known, an 80 by 24 terminal (map panel 22 rows), a lit torch (light 2), rations and a Wand of Light with 5 charges. Fixture interventions: the one grid just beyond the light is set to unknown so the Borg wants to light the corridor, the panel offset is set by the fixture (the UI sets it in play) and the panel-contains hook is unset, so any grid counts as on the panel; the Borg's map update is not run, so the aim is queued, not played. The production borg_light_beam() decides.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lightbeam-88
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lightbeam-88-evidence
git show 05e527351d607583b9c30316369f8d00934a7fe0:docs/audit/c-reports/lightbeam-88/reproduce.py > /tmp/reproduce-lightbeam-88.py
python3 /tmp/reproduce-lightbeam-88.py --full-tests --fixtures-ref 05e527351d607583b9c30316369f8d00934a7fe0 --fix-ref 828318ab58cbedc3f90cfe1a343ebd314257933b
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-light.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `828318ab58cbedc3f90cfe1a343ebd314257933b` (branch `task/lightbeam-88`), fixtures
  `05e527351d607583b9c30316369f8d00934a7fe0` (branch `task/lightbeam-88-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: top
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

At dungeon row 30 with the panel scrolled to row 24 the original refused the north and the south beam; the corrected Borg aims the wand in both directions. At dungeon row 10 (panel at the top) both aim north with identical logs.

Priority low: the effect depends on the terminal height (a taller map panel pushes the refusal row down) and what is measured is the decision to aim the wand, not the lit corridor or any time saved. The north test keeps the existing "bottom edge" comparison; whether a north beam should instead be refused near the top of the panel is a separate question that is not changed here.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
