# A Borg in real danger never drank its Healing potion

This is **healchance-364 / issue #364**, a C Borg defect
(diegoami/neo-angband-tracker#364).

Before using any healing item, the Borg rolls a chance and skips healing when the roll is below a threshold for how hurt it is. Three adjustments to that roll go the opposite way from their comments. The worst is 'if danger is close to the hp and healing will help, do it': it subtracts 75, so whenever the danger next to the Borg is at least its current hit points (but below its maximum) it skips every healing step until it has lost 75% of its hit points. A level-31 Warrior on 149 of 374 hit points beside a fire giant (danger 192), carrying three Potions of Healing, never drank one in 100 decisions: it tried to teleport (none carried) and read Word of Recall, which only takes effect many turns later, while staying next to the giant. With the three signs flipped as the comments say, it drinks a Potion of Healing in 100 of 100 decisions and the real quaff command takes it to 374 hit points. Next to a unique it now heals more often (18 to 32 of 100, against 23 with no unique) and a risky Borg less often than a careful one (27 to 20, against 23).

## What to watch

Decisions (of 100) in which borg_heal() healed; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `b366550f7`) |
| --- | --- | --- |
| Fire giant adjacent: danger 192 at or above current HP 149 | failed, heals=0 | passed, heals=100 |
| Golfimbul adjacent: ordinary danger 44, unique nearby | failed, heals=18 | passed, heals=32 |
| Cave orc adjacent, borg_plays_risky on | failed, heals=27 | passed, heals=20 |
| Control: cave orc adjacent, ordinary danger 8 | passed, heals=23 | byte-identical log |

- **Fire giant adjacent: danger 192 at or above current HP 149.** Original: 0 of 100 decisions heal; the Borg flees by reading Word of Recall ('Fleeing (failed to teleport)') next to the giant. Corrected: 100 of 100 drink a Potion of Healing ('Healing Level 7'); the real quaff command takes the Warrior from 149 to 374 hit points.

- **Golfimbul adjacent: ordinary danger 44, unique nearby.** 'Increase the odds of healing' near a unique. Original: 18 of 100 decisions heal, fewer than the 23 of the control with the same seeds and no unique. Corrected: 32 of 100.

- **Cave orc adjacent, borg_plays_risky on.** 'Risky Borgs are less likely to heal'. Original: 27 of 100 heal, more than the 23 with the same seeds and risky off. Corrected: 20 of 100, fewer than 23.

- **Control: cave orc adjacent, ordinary danger 8.** No adjustment applies (no unique, not risky, danger below current HP): the gate draws the same roll and only the unchanged class adjustment applies. 23 of 100 heal in both versions; logs byte-identical.

## Conditions

Native Human Warrior, seed 364, level 31, depth 30, 149 of 374 hit points (60% lost: the 51-65% band of the gate), starting kit (Potion of Berserk Strength, Word of Recall, no teleport) plus three known Potions of Healing, a lit 38x22 room, one awake monster adjacent (south-east). For each case the production borg_think_dungeon() unique scan (borg_near_monster_type) and borg_caution() run 100 times with the RNG seeded 1..100, and each decision's queued keys and '# Healing Level' note are recorded; a further decision (seed 1000) is carried out with the real quaff command when the Borg queued one. Modes: 'danger' (fire giant, danger 192, at or above current HP), 'unique' (Golfimbul, danger 44), 'risky' (cave orc, danger 8, borg_plays_risky turned on, compared with the same seeds with it off) and 'control' (cave orc, danger 8). Fixture interventions: the arena, lighting and hit points are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_VIEW, borg.avoidance is set to current HP (as borg_update does), the monster is marked awake in the Borg's list, the RNG is fully reset and reseeded per decision (the WELL index state_i is zeroed, since Rand_state_init() does not reset it), and borg_plays_risky is set in the risky mode.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/healchance-364
git fetch https://github.com/diegoami/neo-angband-tracker.git task/healchance-364-evidence
git show 3adc1b8128edfe31ca9a395d64f860b995396ec6:docs/audit/c-reports/healchance-364/reproduce.py > /tmp/reproduce-healchance-364.py
python3 /tmp/reproduce-healchance-364.py --full-tests --fixtures-ref 3adc1b8128edfe31ca9a395d64f860b995396ec6 --fix-ref b366550f776d4d7a6ac52031529f3254a5914d38
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-caution.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `b366550f776d4d7a6ac52031529f3254a5914d38` (branch `task/healchance-364`), fixtures
  `3adc1b8128edfe31ca9a395d64f860b995396ec6` (branch `task/healchance-364-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: control
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_heal() skips healing when chance is below the band threshold, but the unique (-10), danger-at-or-above-HP (-75) and risky (+5) adjustments push chance the opposite way from their comments. In a native scenario a level-31 Warrior on 149/374 HP beside a fire giant (danger 192) with three Potions of Healing healed in 0 of 100 decisions and read Word of Recall; with the three signs flipped it heals in 100 of 100 and the real quaff restores it to 374 HP. Near a unique heals go 18 -> 32 of 100 (23 without a unique); a risky Borg goes 27 -> 20 (against 23 not risky). The ordinary-danger control is byte-identical.

One class (Warrior), one hit-point band (60% lost) and one adjacent monster per case; 100 seeded decisions per case, not a whole game. Whether the original's Word of Recall escape (or the next decision) would have saved or killed this Warrior is not played out; deaths are not measured. Population frequency of danger at or above current HP at this call is not measured. The magnitudes (75, 10, 5) are kept and only the signs flipped, as the comments read; the -25 class adjustment and the band thresholds are unchanged. The control evaluates the three changed conditions but takes none of them.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
