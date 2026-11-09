# Explain why the rest check rates a monster at its own grid

This is **restcomment-486 / issue #486**, a C Borg cleanup with no behaviour change (owner decision)
(diegoami/neo-angband-tracker#486).

Comment-only cleanup, no behaviour change; owner decision. Before resting, borg_check_rest() refuses when a monster within four grids is too dangerous. It asks for the danger at the monster's own grid, not at the resting grid. The triage measured that this is what keeps the check useful: at the resting grid a melee monster three or four grids away scores 0, while at its own grid it is scored as if adjacent, which is the danger it poses once it closes in. The owner decided to keep the behaviour and say so in the code. The change replaces the comment 'one call for dangers' with one that explains the choice; no code changes, and the compiled object is identical.

## What to watch

Borg_check_rest() answer (1 = safe to rest here, 0 = refused); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Changed (fix `622f62ddf`) |
| --- | --- | --- |
| Control: awake cave bear 3 grids away, clevel 10 | passed, rest=0 | byte-identical log |
| Control: the same bear, clevel 20 | passed, rest=1 | byte-identical log |
| Control: the bear asleep | passed, rest=0 | byte-identical log |
| Control: awake wolf (+10 speed) 2 grids away | passed, rest=0 | byte-identical log |
| Control: awake cave orc with arrows 3 grids away | passed, rest=0 | byte-identical log |

- **Control: awake cave bear 3 grids away, clevel 10.** Danger 40 at the bear's grid against a threshold of 22: both versions refuse to rest. Logs byte-identical.

- **Control: the same bear, clevel 20.** Threshold 43 is above the danger of 40: both versions allow the rest. Logs byte-identical.

- **Control: the bear asleep.** Danger 54 at the bear's grid: both versions refuse. Logs byte-identical.

- **Control: awake wolf (+10 speed) 2 grids away.** Danger 48 at the wolf's grid: both versions refuse. Logs byte-identical.

- **Control: awake cave orc with arrows 3 grids away.** Danger 52 either way, and the orc has a ranged attack in line of sight: both versions refuse. Logs byte-identical.

## Conditions

Native Human Warrior at dungeon level 10, seed 486, 60% HP, a torch and rations, in a lit, open 24x40 arena with one monster due east, observed through the Borg's own observe_kill_diff(); borg.avoidance is set to current HP, so the refusal threshold is avoidance/3. The production borg_check_rest() runs at the player's grid; the observer also prints borg_danger_one_kill() at both grids for reference. 'bear': an awake cave bear 3 grids away, clevel 10. 'bear-l20': the same at clevel 20. 'asleep': the bear asleep. 'wolf': an awake wolf (+10 speed) 2 grids away. 'orc': an awake cave orc (has arrows) 3 grids away. Fixture interventions: the arena and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_VIEW, borg.avoidance is set to current HP, and only borg_kills[k].awake is set by hand.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/restcomment-486
git fetch https://github.com/diegoami/neo-angband-tracker.git task/restcomment-486-evidence
git show 9b04516f5e592f04f2863e88898a5328b8892cb0:docs/audit/c-reports/restcomment-486/reproduce.py > /tmp/reproduce-restcomment-486.py
python3 /tmp/reproduce-restcomment-486.py --full-tests --fixtures-ref 9b04516f5e592f04f2863e88898a5328b8892cb0 --fix-ref 622f62ddfcc27f04eed3cdee92321fdee74a1ea3
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
change is exactly the expected change in `src/borg/borg-flow-misc.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `622f62ddfcc27f04eed3cdee92321fdee74a1ea3` (branch `task/restcomment-486`), fixtures
  `9b04516f5e592f04f2863e88898a5328b8892cb0` (branch `task/restcomment-486-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: bear, bear-l20, asleep, wolf, orc
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## Compiled object unchanged

The change is a comment, so the runner also disassembles the compiled
`borg-flow-misc.c` object (`objdump -d CMakeFiles/OurCoreLib.dir/src/borg/borg-flow-misc.c.o`)
after each build and requires the two disassemblies to be byte-identical.
They are: `before-objdump-borg-flow-misc.c.log` and
`after-objdump-borg-flow-misc.c.log` are committed, and `result.json`
records their sha256 (`3292be06891b2ea7bdae581e8e850bf12416e35465fba43f621dc44ec594a5ca`)
under `identical_objects`.

The fixtures pin `9b04516f5` is the generator commit `01dc4c14f` plus the
profile copy of `borg.txt` (byte-identical to the `borg.txt` beside it),
which the first commit left out.

## What this proves, and what it does not

The change is a comment. The pinned runner asserts the exact hunk, then shows that the compiled borg-flow-misc.c object disassembles byte-identically (objdump -d) before and after, and that five native borg_check_rest() scenarios (refuse and accept, awake and asleep, melee and ranged monsters) give byte-identical logs. Both full native gates pass on both builds.

This is a cleanup with no failing mode: the scenarios and the object comparison show unchanged behaviour, they cannot show a difference. The comment states the reasoning the triage measured (the danger at the resting grid is 0 for a melee monster three or four grids away); whether the rest check should use a different design, such as the resting-grid danger plus a movement allowance, is out of scope.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
