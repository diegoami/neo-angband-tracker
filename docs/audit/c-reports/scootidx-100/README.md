# A monster out of sight made the Borg phase away from a harmless neighbour

This is **scootidx-100 / issue #100**, a C Borg defect
(diegoami/neo-angband-tracker#100).

Rogues, warriors and other archers use 'Shoot N Scoot': when a dangerous monster reaches melee range they cast Phase Door to get distance and keep shooting. To decide whether the monster next to the Borg is worth it, one test looks up that monster's danger. It looked up the wrong monster: it used the direction number of the grid (0 to 7) as the monster number, so it read whichever monster the Borg happened to record in that slot. A level-38 Rogue at full health (296 hit points) with only a mummified orc beside it (danger 16 at its grid) therefore cast Phase Door, because monster number 1 in its records was an ogre mage down a corridor, out of sight of the Rogue. It spent a turn and 2 spell points teleporting away from a neighbour whose own danger the test rates 0, to a random grid. The correction looks up the monster actually standing in that grid, and the Rogue stays put.

## What to watch

Cast Phase Door to Shoot'N'Scoot (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `dbafd291a`) |
| --- | --- | --- |
| Mummified orc north (direction slot 1 = the ogre mage's number) | failed, cast_phase=1 | passed, cast_phase=0 |
| Control: mummified orc south (direction slot 0, unused) | passed, cast_phase=0 | byte-identical log |
| Control: olog north (pack monster, qualifies anyway) | passed, cast_phase=1 | byte-identical log |

- **Mummified orc north (direction slot 1 = the ogre mage's number).** Original: the test reads the ogre mage's danger at the orc's grid (144, above 30% of 296 = 88) instead of the orc's (0); '# Shoot N Scoot. (Danger Level 7.1)', Phase Door is cast, the Rogue moves from (20,20) to (29,23) and spends 2 spell points. Corrected: the orc's own danger is read; no Shoot'N'Scoot, no cast.

- **Control: mummified orc south (direction slot 0, unused).** Slot 0 holds no monster and the orc's own danger is 0, so neither version scoots; the logs are identical.

- **Control: olog north (pack monster, qualifies anyway).** The olog qualifies through the pack-monster test, so both versions Shoot'N'Scoot and cast Phase Door to the same grid; the logs are identical.

## Conditions

Native Human Rogue, seed 100, level 38, depth 30, 296/296 hit points, 154 spell points, Phase Door learned from [First Spells] (no other spell), a sling with 30 iron shots and 5 Phase Door scrolls, in a lit 40x40 arena. The Borg's map is copied from the game cave. An awake ogre mage stands at the end of a walled corridor running west from the grid north of the Borg; the Borg saw it from that grid a moment earlier (the fixture moves the player there to observe it, then back), so it is the Borg's monster number 1 but is out of sight of the Borg's grid. 'north': an awake mummified orc (monster number 2, no spells, not in a group) stands north of the Borg, in direction slot 1. 'south' (control): the same orc stands south, direction slot 0, an unused monster number. 'pack' (control): an olog, a pack monster close to the Borg's level, stands north; it qualifies for Shoot'N'Scoot without the danger test. The production borg_caution() runs once; the queued keys are resolved to the book and spell and executed with the real CMD_CAST command. The danger lines are diagnostic calls of borg_danger_one_kill() with the two candidate indices.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/scootidx-100
git fetch https://github.com/diegoami/neo-angband-tracker.git task/scootidx-100-evidence
git show dbf954ba92ced9b87da786812dfbf4a950abae6b:docs/audit/c-reports/scootidx-100/reproduce.py > /tmp/reproduce-scootidx-100.py
python3 /tmp/reproduce-scootidx-100.py --full-tests --fixtures-ref dbf954ba92ced9b87da786812dfbf4a950abae6b --fix-ref dbafd291a3d4d1ff2b4338ba1ee0cc8c65224277
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-kill.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `dbafd291a3d4d1ff2b4338ba1ee0cc8c65224277` (branch `task/scootidx-100`), fixtures
  `dbf954ba92ced9b87da786812dfbf4a950abae6b` (branch `task/scootidx-100-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: south, pack
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_shoot_scoot_safe() passed the direction index i (0-7) to borg_danger_one_kill() instead of the adjacent monster's kill index ag->kill. A full-health level-38 Rogue next to a mummified orc (own danger 0) cast Phase Door because kill slot 1, matching the orc's direction, held an out-of-sight ogre mage (danger 144 at the orc's grid). With ag->kill it stays; the south (unused slot) and olog (pack, qualifies anyway) controls are byte-identical.

Measured: the danger values the test can read, the production borg_caution() decision, and the real Phase Door cast (position, spell points, energy) in one turn. Not measured: what the Rogue does on the following turns or a whole-game survival effect. The misread only matters when the monster in the slot numbered like the direction is dangerous at the neighbour's grid; when the numbers coincide the original reads the right monster. The intended test itself rarely decides the outcome on its own: it uses partial damage, which for monsters without spells is a small fraction of their melee, and monsters with spells already qualify through the ranged-attack test. The fixture also disables nothing in the decision path; its interventions are the arena, the injected kit, the learned spell, and moving the player one grid to let the Borg see the ogre mage.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
