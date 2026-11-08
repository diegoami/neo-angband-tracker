# Say that grid danger always uses the full damage on purpose

This is **dangerfull-69 / issue #69**, a C Borg cleanup with no behaviour change (owner decision)
(diegoami/neo-angband-tracker#69).

Comment-only cleanup, no behaviour change; owner decision (option A). borg_danger() answers 'how dangerous is this grid?' for almost every Borg decision. Nearly all callers pass full_damage = false, but the function sets it to true before adding up the monsters, so every caller gets the worst-case estimate. The owner decided the override is deliberate: the reduced estimate it would unmask rests on a hit-chance formula that is tracked separately (#63). The change adds a comment saying the override is deliberate and that reduced estimates come only from direct calls to borg_danger_one_kill(). Nothing became dead code because of the override (those direct calls still use the reduced path), so no code is removed; the compiled object is identical.

## What to watch

Borg_danger() next to the monster, as callers ask for it (full_damage false); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Changed (fix `8af235045`) |
| --- | --- | --- |
| Control: awake cave bear, 3 grids away | passed, adj=20 | byte-identical log |
| Control: awake wolf, 2 grids away | passed, adj=48 | byte-identical log |
| Control: cave orc with arrows, 3 grids away | passed, adj=22 | byte-identical log |
| Control: kobold shaman (spells), 3 grids away | passed, adj=22 | byte-identical log |

- **Control: awake cave bear, 3 grids away.** borg_danger() gives 0 at the player's grid and 20 next to the bear whether full_damage is false or true; a direct borg_danger_one_kill() call gives 0 (reduced) and 20 (full). Logs byte-identical.

- **Control: awake wolf, 2 grids away.** 16 at the player's grid, 48 next to the wolf, for both argument values; the direct call gives 0 and 48. Logs byte-identical.

- **Control: cave orc with arrows, 3 grids away.** 13 and 22 for both argument values; the direct call gives 5 and 22. Logs byte-identical.

- **Control: kobold shaman (spells), 3 grids away.** 22 and 22 for both argument values; the direct call gives 8 and 22. Logs byte-identical.

## Conditions

Native Human Warrior at dungeon level 10, seed 69, 60% HP, a torch and rations, in a lit, open 24x40 arena with one monster due east, observed through the Borg's own observe_kill_diff() and tracked as awake. The production borg_danger() runs at the player's grid and at the grid next to the monster, with full_damage false (as its callers pass it) and true, plus once with c=4 at the player's grid; borg_danger_one_kill() is called directly with both values at the grid next to the monster for reference. 'bear': cave bear 3 grids away. 'wolf': wolf (+10 speed) 2 grids away. 'orc': cave orc (arrows) 3 grids away. 'shaman': kobold shaman (spells) 3 grids away. The scenario check requires borg_danger() to give the same answer for false and true. Fixture interventions: the arena and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_VIEW, borg.avoidance is set to current HP, and only borg_kills[k].awake is set by hand.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dangerfull-69
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dangerfull-69-evidence
git show 7b06c733a9db29c8ad2b3608c0e9e6065c467824:docs/audit/c-reports/dangerfull-69/reproduce.py > /tmp/reproduce-dangerfull-69.py
python3 /tmp/reproduce-dangerfull-69.py --full-tests --fixtures-ref 7b06c733a9db29c8ad2b3608c0e9e6065c467824 --fix-ref 8af23504525b7577ddc74205c7aec03263cc001e
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
change is exactly the expected change in `src/borg/borg-danger.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `8af23504525b7577ddc74205c7aec03263cc001e` (branch `task/dangerfull-69`), fixtures
  `7b06c733a9db29c8ad2b3608c0e9e6065c467824` (branch `task/dangerfull-69-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: bear, wolf, orc, shaman
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## Compiled object unchanged

The change is a comment, so the runner also disassembles the compiled
`borg-danger.c` object (`objdump -d CMakeFiles/OurCoreLib.dir/src/borg/borg-danger.c.o`)
after each build and requires the two disassemblies to be byte-identical.
They are: `before-objdump-borg-danger.c.log` and
`after-objdump-borg-danger.c.log` are committed, and `result.json` records
their sha256 (`adb201ad4b75c09e1c934f804dd44228e411f0dedac9799a964cd7884e3d7b9b`)
under `identical_objects`.

## Why no code is removed

The owner decision allowed removing code that is provably dead because of
the override. None is: the reduced-damage paths in `borg_danger_one_kill()`
and `borg_danger_physical()` are still reached by the direct
`borg_danger_one_kill(..., false)` calls in `borg-fight-attack.c` (lines
1306 and 1328 at the base), and the `ONEKILL` lines in every scenario log
show that path giving lower values. The `full_damage` parameter of
`borg_danger()` itself is kept: dropping it would change the signature and
all of its call sites, which is an interface change rather than a dead-code
removal.

## What this proves, and what it does not

The change is a comment. The pinned runner asserts the exact hunk, then shows that the compiled borg-danger.c object disassembles byte-identically (objdump -d) before and after, and that four native borg_danger() scenarios (melee, fast melee, archer, spellcaster) give byte-identical logs. Each scenario also records what the comment states: borg_danger() returns the same value for full_damage false and true, while a direct borg_danger_one_kill() call still gives a lower reduced estimate. Both full native gates pass on both builds.

This is a cleanup with no failing mode: the scenarios and the object comparison show unchanged behaviour, they cannot show a difference. No code was removed: the override makes nothing provably dead, because the reduced paths in borg_danger_one_kill() and borg_danger_physical() are still reached by the direct calls in borg-fight-attack.c (lines 1306 and 1328 at the base). The full_damage parameter of borg_danger() is kept; removing it would touch 64 call sites and is not a dead-code removal. The hit-chance formula behind the reduced estimate is tracked separately as #63.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
