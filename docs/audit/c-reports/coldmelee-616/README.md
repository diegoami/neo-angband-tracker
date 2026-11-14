# Cold blows were weighed against temporary acid resistance

This is **coldmelee-616**, a C Borg defect (diegoami/neo-angband-tracker#616). It was found while working on #610.

**The slip.** `borg_danger_physical()` divides the damage of an elemental monster blow by three for each resistance to its element. The `MONBLOW_COLD` case tested `borg.temp.res_acid` instead of `borg.temp.res_cold`, a copy slip: the acid, electricity and fire blows directly above test their own element. As a result:
- the Borg **overrated** cold blows while it resisted cold (Resist Cold, or Resist Heat & Cold);
- it **underrated** them while it resisted only acid.

**The correction** changes one word in `src/borg/borg-danger.c`: `res_acid` → `res_cold`.

## What to watch

The danger the Borg assigns to its grid with a frost giant next to it. Each run first reads a **reference** with no temporary resistance (danger 160), then the scenario.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `28d478129`) |
| --- | --- | --- |
| Temporary cold resistance | failed: danger **160** = reference | passed: danger **56** < reference |
| Temporary acid resistance only | failed: danger **56** < reference | passed: danger **160** = reference |
| Control: temporary fire resistance | passed: 160 = 160 | byte-identical log |

## Conditions

- Character: native Human Warrior, seed 616, character level 30, 351 hit points, dungeon level 28, wearing a Soft Leather Armour.
- Arena: lit, with the shipped `src/borg/borg.txt` of the base revision.
- Monster: an awake frost giant next to the Borg (two `HIT:COLD:5d8` blows, plus a boulder throw).
- Fixture interventions: the timed resistances (`TMD_OPP_COLD`, `TMD_OPP_ACID`, `TMD_OPP_FIRE`) are set directly. The giant enters the Borg's monster list through the production `observe_kill_diff()`.
- Reading: the production `borg_danger()`.

[replay.html](replay.html) is the offline page (download it raw and open it locally). Choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/coldmelee-616
git fetch https://github.com/diegoami/neo-angband-tracker.git task/coldmelee-616-evidence
git show d6d93e3be21d919ffc71635c1fd05cc7fea4f8b2:docs/audit/c-reports/coldmelee-616/reproduce.py > /tmp/reproduce-coldmelee-616.py
python3 /tmp/reproduce-coldmelee-616.py --full-tests --fixtures-ref d6d93e3be21d919ffc71635c1fd05cc7fea4f8b2 --fix-ref 28d478129862086b77f537dca4ff2e8f9235bcf7
```

The runner:
- asserts that the correction is exactly `fix.patch` (`src/borg/borg-danger.c` only);
- builds the original and corrected native C game in a disposable worktree;
- runs the three scenarios and checks each expected result and the byte-identical control;
- runs both full native gates and writes `replay.html`.

Heavy steps wait on `/tmp/angband-validation.lock`. The frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master)
- fix `28d478129862086b77f537dca4ff2e8f9235bcf7` (branch `task/coldmelee-616`)
- fixtures `d6d93e3be21d919ffc71635c1fd05cc7fea4f8b2` (branch `task/coldmelee-616-evidence`); later commits only add these artifacts
- byte-identical control: control
- full native gates: 965/965 unit checks and 5/5 frontend tests, before and after
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

With the correction, temporary cold resistance lowers the danger of cold blows and temporary acid resistance does not. The original had it the other way round.

Not measured: the decisions that follow (for example whether the Borg casts or quaffs a cold resistance, which `borg_defend()` values with this danger), fights or deaths.

## Files

- Generators: `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`).
- Captured run: `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file).
