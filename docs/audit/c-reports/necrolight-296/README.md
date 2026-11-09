# A Necromancer in a lit room thought its spell was safe

This is **necrolight-296 / issue #296**, a C Borg defect
(diegoami/neo-angband-tracker#296).

Necromancers cast worse in the light: whenever their square is lit, the game adds 25 points to every spell's failure chance. The Borg estimates that failure chance before deciding to cast, and it refuses an attack spell whose estimate is over its limit (25% for Vampire Strike against an ordinary monster). Its estimate added the light penalty only when the Borg's own torch lit the square, and Necromancers normally carry no torch. A level-26 Necromancer standing in a permanently lit room facing a kobold therefore estimated 8% failure for Vampire Strike while the game used 33%, and cast it anyway. The correction uses the same lit-square test as the game: the estimate becomes 33% and the Borg does not cast the spell.

## What to watch

Casts Vampire Strike (1 = yes) with the game's failure chance above the Borg's 25% limit; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `ea838de67`) |
| --- | --- | --- |
| Torchless Necromancer on a permanently lit square | failed, cast_spell=1 | passed, cast_spell=0 |
| Control: the same Necromancer, its own square unlit | passed, cast_spell=1 | byte-identical log |
| Control: a Mage on the lit square | passed, cast_spell=1 | byte-identical log |

- **Torchless Necromancer on a permanently lit square.** Original: the Borg estimates 8% failure while the game's spell_chance() is 33%; the estimate passes the 25% limit and the Borg casts Vampire Strike (keys m b b). Corrected: the estimate is 33%, equal to the game's; the Borg does not cast.

- **Control: the same Necromancer, its own square unlit.** No light on the Borg's square: the game uses 8%, both versions estimate 8% and cast Vampire Strike. Logs are byte-identical.

- **Control: a Mage on the lit square.** Mages have no light penalty: the game and both versions use 2% for Magic Missile and the Borg casts it. Logs are byte-identical.

## Conditions

Native Human Necromancer, seed 296, level 26, depth 30, eight intelligence gains, [Fear and Torment] carried with only Vampire Strike learned, full mana (118), no light source. A 40x40 arena whose floor is permanently lit (SQUARE_GLOW); one awake Kobold five squares south. The fixture fills the Borg's map from the game cave and observes the kobold; the Borg's own light map is not touched (no light is carried, so it would be empty). The production borg_attack(false) runs once in acting mode; queued keys are read back. The game's spell_chance() and the Borg's borg_spell_fail_rate() are printed for the same spell. Controls: the same Necromancer with only its own square unlit, and a Human Mage (starting torch) on the lit arena casting Magic Missile. The cast is not executed: this measures the estimate and the decision, not damage, failed-cast frequency or survival.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/necrolight-296
git fetch https://github.com/diegoami/neo-angband-tracker.git task/necrolight-296-evidence
git show 742e04ed8358e566933902faf63aaca36b91ff1b:docs/audit/c-reports/necrolight-296/reproduce.py > /tmp/reproduce-necrolight-296.py
python3 /tmp/reproduce-necrolight-296.py --full-tests --fixtures-ref 742e04ed8358e566933902faf63aaca36b91ff1b --fix-ref ea838de6782149c83300dccdfc06d15c099fbd86
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-magic.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `ea838de6782149c83300dccdfc06d15c099fbd86` (branch `task/necrolight-296`), fixtures
  `742e04ed8358e566933902faf63aaca36b91ff1b` (branch `task/necrolight-296-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: dark, mage
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_spell_fail_rate() adds the Necromancer unlight penalty only when BORG_LIGHT (the Borg's own light source) marks its square, while the game's spell_chance() adds it whenever square_islit() holds. A torchless level-26 Necromancer on a permanently lit square estimated 8% for Vampire Strike while the game used 33%, passed the 25% borg_spell_okay_fail() limit and cast it. Using square_islit() makes the estimate 33% and the cast is declined; unlit and non-Necromancer controls are byte-identical.

The fixture measures the failure estimate and the attack decision only. It does not execute the cast, measure how often a cast fails in play, mana or turns lost, damage taken or survival. Permanent light is set by the fixture on an arena floor; in play the same state arises in lit rooms. The fix does not add the game's low-mana penalty, which the Borg's estimate also omits (a separate matter; the fixture runs at full mana).

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
