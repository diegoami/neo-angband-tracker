# A newly seen wall-walker made the Borg forget the wall

This is **newkill-95 / issue95**, a C Borg defect
(diegoami/neo-angband-tracker#95).

Some monsters, such as an earth spirit, can stand inside solid rock. The Borg has a rule not to shoot at such a monster unless it is on known floor, because the game stops a missile in front of a wall. When the Borg first sees a monster it creates a tracking record; the last step of that routine looked up the monster's race before the record was filled in, so it read an empty race (one with no wall-walking ability) and overwrote the grid in the Borg's map with floor. An earth spirit seen inside a granite pillar four steps away therefore turned the pillar into floor in the Borg's map, and on that turn the Borg fired a sling shot at it: the real fire command spent the turn and the shot, and the earth spirit took no damage. The correction drops that last step, because the routine has already placed the monster on floor or wall with the right race a few lines earlier; the Borg's map keeps the granite and it does not shoot into the rock.

## What to watch

Fires a sling shot at the earth spirit (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `1a220cb2d`) |
| --- | --- | --- |
| Earth spirit inside a granite pillar | failed, fired=1 | passed, fired=0 |
| Control: earth spirit on open floor | passed, fired=1 | byte-identical log |

- **Earth spirit inside a granite pillar.** Original: the first sighting overwrites the pillar with floor in the Borg's map (map_granite=0); the Borg fires (keys *p66665f05); the real fire command uses a full turn (energy 100) and one shot, the shot stops at the wall and the earth spirit keeps 56/56 hit points. Corrected: the map keeps granite (map_granite=1), the Borg's own 'never shoot walls' rule holds and no attack is queued.

- **Control: earth spirit on open floor.** The same earth spirit on a floor grid. Both versions keep the grid as floor and fire; the shot hits for 4 damage. The logs are byte-identical.

## Conditions

Native Human Warrior, seed 1000, character level 1, depth 20, wielding a sling with 20 iron shots, in a lit 38x38 room. The fixture's terrain change is one granite grid four steps east of the Borg (wall case only); the Borg's map is filled from the real terrain, as borg_update_map() does for seen grids. A stock earth spirit is placed one grid further east and moved into that grid with the game's monster_swap(); it is awake and visible. The production first-sighting path observe_kill_diff() -> borg_new_kill() runs once, then the production borg_attack(false) chooses an action and its queued keys are read back. When the Borg chose to fire, the fixture carries it out with the real CMD_FIRE command (quiver shot, target = the monster's grid); game messages are printed as MSG lines. This isolates one decision turn; it does not play out the fight.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/newkill-95
git fetch https://github.com/diegoami/neo-angband-tracker.git task/newkill-95-evidence
git show f56325e33a1ea1e074dab923fa84f1a620eb292c:docs/audit/c-reports/newkill-95/reproduce.py > /tmp/reproduce-newkill-95.py
python3 /tmp/reproduce-newkill-95.py --full-tests --fixtures-ref f56325e33a1ea1e074dab923fa84f1a620eb292c --fix-ref 1a220cb2db15c4d142f2ee8a97fa111b42d7d806
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-kill.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `1a220cb2db15c4d142f2ee8a97fa111b42d7d806` (branch `task/newkill-95`), fixtures
  `f56325e33a1ea1e074dab923fa84f1a620eb292c` (branch `task/newkill-95-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: floor
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_new_kill() read the race of the reused tracking slot before storing the new race, so its final floor/wall step used an empty race and marked every newly seen monster's grid as floor. A pass-wall monster first seen inside rock thus turned the rock into floor in the Borg's map, defeating the 'never shoot walls' check: a level-1 Warrior fired a sling shot at an earth spirit inside a granite pillar, losing the turn and the shot for 0 damage. borg_update_kill_old(), called just before, already does that step with the right race; removing the stale duplicate keeps the wall and the shot is not taken. The open-floor control (shot hits for 4) is byte-identical.

Measured on the first-sighting turn of a seen granite grid. For a seen grid the next borg_update_map() pass restores granite; for a grid whose terrain the Borg has never seen (FEAT_NONE, e.g. a monster sensed in unexplored rock) the map pass does not overwrite it and borg_update_kill_old() keeps floor, so the false floor can persist; that longer path is from code reading, not measured. How often a wall-walker is first seen inside rock in real games is not measured. The alternative of only moving the race lookup after the store was tried in the scratch tree: it also forces granite under a pass-wall monster standing on seen floor, so the Borg stops shooting it (the floor control changes), which is why the duplicate step is removed instead. The stale read itself is in bounds (race 0, or an older race after the twitchy reset of borg_kills_nxt without wiping), so no sanitizer variant applies.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
