# Dimension Door aimed at solid rock and landed the Borg back beside its attacker

This is **dimdoor-21 / issue #21**, a C Borg defect
(diegoami/neo-angband-tracker#21).

Dimension Door lets a Mage teleport to a grid of its choice. When the Borg uses it to escape, it scores every grid within 50 steps by its danger estimate and aims at the least dangerous one, but it never checks that the player can stand there. The scan starts in the far north-west corner, so the winner is usually a grid of solid rock the Borg has never seen. The game does not put the player inside rock: it searches outward from the chosen grid for the nearest grid the player can stand on. In this run a badly hurt level-50 Mage is hit by a nether bolt from an invisible dread in its room, and a second, explored room lies 34 steps east. The original Borg aims at (16,1), unknown to it and granite in the game; the game puts the player at (60,31), the west edge of the same room, 9 steps from the dread and in its line of sight, inside the region the Borg was trying to leave. The corrected Borg only aims at grids it knows to be floor with no monster on them; it aims at (100,25) in the explored room, and the game puts the player exactly there, out of the dread's line of sight. When the least dangerous grid is already a known floor grid (an explored open level), both versions aim at it and land on it, with identical logs.

## What to watch

Dimension Door puts the player where the Borg aimed (1 yes, 0 no); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `4b7a9f414`) |
| --- | --- | --- |
| Rock level: player's room with an invisible dread, an explored room 34 steps east | failed, on_aim=0 | passed, on_aim=1 |
| Control: explored open level, least dangerous grid is floor | passed, on_aim=1 | byte-identical log |

- **Rock level: player's room with an invisible dread, an explored room 34 steps east.** Original: aims at (16,1), a grid the Borg has never seen and granite in the game, with a scored danger of 0; the game puts the player at (60,31), in the same room, 9 steps from the dread and in its line of sight, regional fear there 78 (as at the start). Corrected: aims at (100,25), the first zero-danger known floor grid, and lands exactly there, 34 steps from the dread, out of its line of sight, regional fear 0.

- **Control: explored open level, least dangerous grid is floor.** On an explored open level the least dangerous grid in scan order, (16,1), is known floor with no monster. Both versions aim at it and the game puts the player exactly there; byte-identical logs.

## Conditions

Native Human Mage, seed 21, character level 50, depth 40, maximum Intelligence, [Arcane Control] carried and only Dimension Door learned (5% failure when the escape is chosen; no other escape carried). Level 66x198. granite: solid granite except room A (x 60-72, y 25-35) holding the player at (66,30) and an invisible dread at (69,32), and room B (x 100-110, y 25-35). open: the whole level is lit floor (control). Real code: the game's monster spell code (do_mon_spell with the dread's nether bolt, which really damages the player), the Borg's message parser (borg_parse) and map/message update (borg_update, three passes), which raises its regional fear around the unseen caster, the production escape routine borg_escape() called with the danger of the Borg's grid as borg_caution() calls it, and the game's spell code (spell_cast, the TELEPORT_TO effect) which moves the player. Fixture interventions: the level, rooms and dread are placed directly; the player starts at a quarter of maximum hit points (as after earlier fighting); the level is mapped with the game's wiz_light() and the floor of room B (granite) or of the whole level (open) is memorised as seen; the dread's spell is cast by calling do_mon_spell directly (its own spell choice is not run); an EVENT_MESSAGE handler hands each game message to borg_parse instead of the Borg scraping the screen; the Borg's queued keys are decoded by the fixture (the free-targeting cursor walk from the player, then 'm', book, spell and '5'), checked to be Dimension Door from the carried book, and executed by setting that target with target_set_location() and calling spell_cast() with a direction hook that answers '5' (use target).

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dimdoor-21
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dimdoor-21-evidence
git show 3edecf91b284a112f3f3e916e18b6fdaec2aa46c:docs/audit/c-reports/dimdoor-21/reproduce.py > /tmp/reproduce-dimdoor-21.py
python3 /tmp/reproduce-dimdoor-21.py --full-tests --fixtures-ref 3edecf91b284a112f3f3e916e18b6fdaec2aa46c --fix-ref 4b7a9f414bec7b23aa3453dfb7e54ba2e85c871b
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-escape.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `4b7a9f414bec7b23aa3453dfb7e54ba2e85c871b` (branch `task/dimdoor-21`), fixtures
  `3edecf91b284a112f3f3e916e18b6fdaec2aa46c` (branch `task/dimdoor-21-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: open
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_dimension_door() (borg-escape.c:569-621) takes the lowest borg_danger() grid of the in-bounds box with no check that the player can land there. The scan order makes the north-west-most zero-danger grid win, which on a rock level is unknown granite. effect_handler_TELEPORT_TO (effect-handler-general.c:2717-2843) never lands on an impassable or occupied grid: it searches outward from the target (has_teleport_destination_prereqs, effect-handler-general.c:132-157), so the Borg's score describes a grid it never reaches. The correction skips grids not known to be floor (or that are lava, or hold a known monster) before scoring.

Measured: one escape decision and the real Dimension Door it casts, the aimed grid, the landing grid, distance and line of sight to the dread, and the Borg's regional fear at the landing grid. Not measured: the following turns, hit points afterwards, or deaths; whether the landing in room A leads to further damage depends on the dread's next turns. The game has no range limit for TELEPORT_TO, so the issue's 'range' point needs no check; the box size is the Borg's own performance cap. Traps on the target are not checked (the game also refuses player traps); unknown grids are now excluded, so on a poorly explored level Dimension Door may find no target and the escape falls through to the next option, as it already does when no grid scores lower. Related but separate and not changed: borg_dimension_door() ignores the result of borg_spell(DIMENSION_DOOR) and returns true after queueing the targeting keys even if the cast is refused; borg_spell_okay_fail() is checked first, so this was not reached here.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
