# The Borg valued a Mind Blast at a quarter of its damage

This is **mindblast-591 / issue591**, a C Borg defect (diegoami/neo-angband-tracker#591), found by reading `borg_danger()` against our strategy manual and the game data.

A **Mind Blast** deals **8d8** damage (average 36, at most 64) and then confuses for 3+1d4 turns (`monster_spell.txt`, `MIND_BLAST`). The Borg's danger estimate valued it at the caster's spell power divided by two, plus one — 9 for Nár, 15 for a mind flayer or vampire, 21 for a beholder — and ignored the confusion. The correction values it at the 8d8 maximum, as the other spells are modelled, and adds the same small concern for the confusion that the plain Confusion spell gets when the Borg does not resist confusion. Casters: Nár (level 17), vampires, spirit nagas, mind flayers, silent watchers, master vampires, beholders and others.

## What to watch

The danger the Borg assigns to its grid; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `9a9fb3ce8`) |
| --- | --- | --- |
| Nár, the Dwarf four grids away | failed: danger **40** (his Wound spell; Mind Blast counted as 9) | passed: danger **74** (Mind Blast 64, plus 10 for the confusion) |
| Control: an illusionist (no Mind Blast) | passed: danger 600 | byte-identical log |

The control's danger is high because an illusionist can paralyse a Warrior without free action; this change does not touch it.

## Conditions

Native Human Warrior, seed 591, character level 20, 200 hit points, saving throw 39, no confusion protection, dungeon level 17, lit arena, an awake caster four grids east (out of melee reach). Fixture interventions: the caster enters the Borg's monster list through the production `observe_kill_diff()`. The production `borg_danger()` is read at the Borg's grid.

[replay.html](replay.html) is the offline page (download raw, open locally): choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/mindblast-591
git fetch https://github.com/diegoami/neo-angband-tracker.git task/mindblast-591-evidence
git show b40e88f2284d5abe2d00106bd1cf8751e2e3c85c:docs/audit/c-reports/mindblast-591/reproduce.py > /tmp/reproduce-mindblast-591.py
python3 /tmp/reproduce-mindblast-591.py --full-tests --fixtures-ref b40e88f2284d5abe2d00106bd1cf8751e2e3c85c --fix-ref 9a9fb3ce8fbfe2b01725e92a9ec04e5facb64af1
```

The runner asserts the correction is exactly `fix.patch` (`src/borg/borg-danger.c` only), builds the original and corrected native C game in a disposable worktree, runs both scenarios, checks each expected result and the byte-identical control, runs both full native gates and writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`; the frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `9a9fb3ce8fbfe2b01725e92a9ec04e5facb64af1` (branch `task/mindblast-591`), fixtures `b40e88f2284d5abe2d00106bd1cf8751e2e3c85c` (branch `task/mindblast-591-evidence`); later commits only add these artifacts.
- control byte-identical: control
- full native gates: before and after 965/965 unit checks and 5/5 frontend tests
- corrected source sha256 (`src/borg/borg-danger.c`): `1741ef92e2c4d9649855c8ae27fc105ba7fa55d5a14b6901e8be6a501fb606a5`
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

The Borg's danger from a Mind Blast caster now reflects the spell's real damage; for Nár it rises from 40 to 74 for a 200-hit-point Warrior. A caster without Mind Blast is unchanged.

Not measured: the caution decision that follows, fights or deaths. Brain Smash (12d15, modelled at its average 96 rather than its maximum 180) is related and noted on the issue but not changed here. Only Nár is exercised; other casters differ only in which of their spells dominates.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`) are the generators; `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file) are the captured run.
