# Remove the unreachable visible-trap case from dark-flow exploration

This is **trapdead-122 / issue #122**, a C Borg cleanup with no behaviour change (owner decision)
(diegoami/neo-angband-tracker#122).

Cleanup, no behaviour change; owner decision. When the Borg explores, borg_flow_dark_interesting() decides which grids are worth walking to: unknown grids, closed doors, rubble, treasure veins. It also has a case for 'visible traps', but that case can never run: the function returns early for every grid type below secret doors (except closed doors), and in the stock terrain data the only trap-holding terrain is the open floor, which is below that line. The Borg also records known traps in its own trap flag, not in the terrain. The change deletes the dead case and the 'visible trap' wording in the function comment. Traps that lie on a chosen route are still checked by the flow spread and disarmed by the step code, as the 'unknown' scenario shows.

## What to watch

First key the dark flow queued (0 = nothing to explore, 68 = 'D' disarm on the route, 52 = step west); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Changed (fix `b53fa471a`) |
| --- | --- | --- |
| Control: fully known room, a known trap next to the player | passed, key=0 | byte-identical log |
| Control: unknown area to the east, the trap on the route | passed, key=68 | byte-identical log |
| Control: a closed door four grids west | passed, key=52 | byte-identical log |

- **Control: fully known room, a known trap next to the player.** Neither version treats the trap as somewhere to explore: both dark-flow passes return false and no key is queued. The logs are byte-identical.

- **Control: unknown area to the east, the trap on the route.** Both versions flow toward the unknown area (far pass). The first step is onto the trap grid, so the step code queues 'D' (key 68) to disarm it: traps on the route are handled outside the deleted case. The logs are byte-identical.

- **Control: a closed door four grids west.** Both versions choose the door (near pass) and step west toward it (key 52), passing the trap by. The logs are byte-identical.

## Conditions

Native Dwarf Rogue, seed 122, clevel 25, depth 5, full HP (231), a lantern, in a lit, open 24x40 arena. The Borg's map is seeded from the real cave; a known trap is recorded the way the Borg records it (open floor with the Borg's trap flag set) on the grid directly east of the player, inside the lantern's light so the first dark-flow method examines it. 'known': the whole room is known, so the trap is the only possible target. 'unknown': the columns from x=30 eastward are unknown. 'door': a real closed door four grids west. The production borg_flow_dark(true) runs, then borg_flow_dark(false) if the first finds nothing; the first queued key and the step grid are read back. Fixture interventions: the arena and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_MARK, the trap exists only as the Borg's trap flag, and borg.avoidance is set to current HP.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/trapdead-122
git fetch https://github.com/diegoami/neo-angband-tracker.git task/trapdead-122-evidence
git show 7faad43e6c6d7f99fc2a4b78bed15f0e17d723b8:docs/audit/c-reports/trapdead-122/reproduce.py > /tmp/reproduce-trapdead-122.py
python3 /tmp/reproduce-trapdead-122.py --full-tests --fixtures-ref 7faad43e6c6d7f99fc2a4b78bed15f0e17d723b8 --fix-ref b53fa471a59305abf4061ca5b3dc8ea3f812cc7a
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
change is exactly the expected change in `src/borg/borg-flow-dark.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `b53fa471a59305abf4061ca5b3dc8ea3f812cc7a` (branch `task/trapdead-122`), fixtures
  `7faad43e6c6d7f99fc2a4b78bed15f0e17d723b8` (branch `task/trapdead-122-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: known, unknown, door
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The deleted case in borg_flow_dark_interesting() is unreachable with stock terrain data: only the open floor has the TRAP terrain flag, and floor grids return early at the FEAT_SECRET test. Three native dark-flow scenarios, one with only a known trap as a candidate, one with a trap on the route to unknown grids and one with a closed door, give byte-identical logs before and after the deletion, and both full native gates pass on both builds.

This is a cleanup with no failing mode: the scenarios show unchanged behaviour, they cannot show a difference. Unreachability is argued from the source and the stock lib/gamedata/terrain.txt (TRAP appears only on the open floor); a modified terrain.txt giving TRAP to a terrain at or above FEAT_SECRET would have reached the deleted case. Not measured: whole-game exploration runs.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
