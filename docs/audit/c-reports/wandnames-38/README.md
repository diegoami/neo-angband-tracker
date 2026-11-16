# Two wands were looked up under the wrong names, so the Borg aimed the wrong wand

This is **wandnames-38 / issue38** (CBUG-0022), a C Borg defect
(diegoami/neo-angband-tracker#38).

When the Borg starts it looks up, by name, each wand it knows how to use. Two lines were copies of the lines above them: the **Confuse Monster** wand was looked up as "Scare Monster", and the **Lightning Balls** wand as "Lightning Bolts". So the attack chooser offered a Lightning Bolts wand (a 6d6 beam, about 21 damage) as a 64-damage radius-2 ball, offered a Scare Monster wand as a confusion bolt, and never used a real Lightning Balls or Confuse Monster wand. Lightning Bolts wands are common from dungeon level 15; Confuse Monster wands are sold in the Magic shop. The correction looks the two wands up by their own names.

## What to watch

With a Lightning Bolts and a Frost Bolts wand, the Borg aims the Frost Bolts wand (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `f9f0384a6`) |
| --- | --- | --- |
| Lightning Bolts wand against Frost Bolts wand | failed: the Lightning Bolts wand is also scored as a ball (59) and **chosen** over Frost Bolts (49) | passed: ball score 0, **Frost Bolts chosen** |
| Real Lightning Balls wand | failed: not recognised, **no attack** | passed: scored 59 and **chosen** |
| Real Confuse Monster wand | failed: not recognised (score 0) | passed: scored as the confusion attack (−6 here, not zapped) |
| Scare Monster wand | failed: also scored as a confusion bolt (−6) | passed: only as a fear bolt |
| Control: Frost Bolts wand only | passed: Frost Bolts chosen | byte-identical log |

- **Lightning Bolts against Frost Bolts.** Original: the Lightning Bolts wand appears twice, as its own beam (16) and as a "Lightning Balls" attack worth 59, so the Borg chooses it and would fire a 21-average beam instead of the 54-average Frost Bolt. Corrected: the Lightning Bolts wand is only a beam and the Borg chooses the Frost Bolts wand (49).
- **Real Lightning Balls wand.** Original: no attack type matches it, so the Borg carries a 64-damage ball wand and never zaps it. Corrected: chosen (59).
- **Confuse Monster and Scare Monster wands.** These show recognition, not a changed zap: against this ogre the Borg scores both confusion and fear bolts below zero, so it zaps neither wand in either build. Originally a real Confuse Monster wand was never even scored and a Scare Monster wand was scored as a confusion bolt; after the correction each is scored as what it does.

## Conditions

Native Human Warrior, seed 38, character level 20, dungeon level 20, in a lit arena, every rune known, an awake **black ogre** (100 hit points, no electricity or cold immunity, can be confused) four grids east in a straight line. Fixture interventions: every starting pack item except food is removed so the wands under test are the only ranged attacks; the wands are made directly with 10 charges and are fully known; the ogre is entered into the Borg's monster list through the production `observe_kill_diff()`. The production `borg_calculate_attack_effectiveness()` scores each wand attack and the production `borg_attack()` chooses and queues the attack; the zap itself is not played.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/wandnames-38
git fetch https://github.com/diegoami/neo-angband-tracker.git task/wandnames-38-evidence
git show 460eddbd46b166e8e722d13b13811138a94f30c3:docs/audit/c-reports/wandnames-38/reproduce.py > /tmp/reproduce-wandnames-38.py
python3 /tmp/reproduce-wandnames-38.py --full-tests --fixtures-ref 460eddbd46b166e8e722d13b13811138a94f30c3 --fix-ref f9f0384a6dec8cc3a1e78d8210df8b004f594e8a
```

The runner asserts the correction is exactly the two expected lines in
`src/borg/borg-item-val.c` and nothing else, builds the original and corrected
native C game in a disposable worktree, runs every scenario, checks each
expected result and that the control log is byte-identical, runs both full
native gates and writes `replay.html`. Heavy steps wait on
`/tmp/angband-validation.lock`. The frontend tests need a UTF-8 locale
(for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `f9f0384a6dec8cc3a1e78d8210df8b004f594e8a` (branch `task/wandnames-38`), fixtures
  `460eddbd46b166e8e722d13b13811138a94f30c3` (branch `task/wandnames-38-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts.
- control byte-identical: frost-only
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- corrected source sha256 (`src/borg/borg-item-val.c`): `db6e929165674ae98d4b236e3fb7e3cb55017b761f9d871bbfd820c044483f26`
- replay: Playwright Chromium headless render check of the committed page
  (every option and visible button exercised), no console errors

## What this proves, and what it does not

The original attack chooser offered a Lightning Bolts wand as a 64-damage ball and chose it over a stronger Frost Bolts wand; it never used a real Lightning Balls wand; it scored a Scare Monster wand as a confusion bolt and never scored a real Confuse Monster wand. With the two names corrected, each wand is offered only as the attack it performs, and a Frost Bolts-only control is unchanged.

Not measured: the zap and its damage, fights, deaths, or how often the Borg carries these wands. The confusion and fear cases change only how the wands are scored; against this monster neither is zapped in either build. Two other copy slips noted in the original report (`sv_mush_cure_mind` looked up as "Clear Mind", `sv_potion_berserk` looked up twice) are not changed here.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
