# A Necromancer Borg never cast Crush on a weak monster shooting at it from range

This is **crushv2-66 / issue #66**, a C Borg defect
(diegoami/neo-angband-tracker#66).

Crush kills every monster in view with less than four times the caster's level in hit points. The Borg values Crush by how much it lowers the danger at its grid. The danger code has two Crush adjustments, one for melee danger and one for ranged danger, but the ranged one cleared the melee value again instead of the ranged value. A monster that would die to Crush therefore kept all of its ranged danger, and Crush looked useless against anything that attacks from a distance. A level-26 Necromancer whose only attack spell is Crush, with an awake blacklock mage (36 hit points, under the limit of 104) five steps away, saw danger 936 both with and without Crush and did nothing. With the ranged value cleared, Crush removes all 936 danger, the Borg casts it, and the real cast kills the mage. The same holds for a cave orc that shoots arrows.

## What to watch

Cast Crush (1 = the Borg queued Crush and the real cast ran); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `9757b1617`) |
| --- | --- | --- |
| Blacklock mage (36 hp, spells) five steps away | failed, cast_crush=0 | passed, cast_crush=1 |
| Cave orc (50 hp, innate arrows) five steps away | failed, cast_crush=0 | passed, cast_crush=1 |
| Control: ogre (60 hp, no ranged attack) five steps away | passed, cast_crush=0 | byte-identical log |
| Control: dark dwarven lord (132 hp, too strong to crush) | passed, cast_crush=0 | byte-identical log |

- **Blacklock mage (36 hp, spells) five steps away.** Original: danger 936 with and without Crush, reduction 0, no attack chosen. Corrected: Crush clears the mage's ranged danger (936 to 0); the Borg queues 'mbb', the real cast kills the mage.

- **Cave orc (50 hp, innate arrows) five steps away.** Original: danger 48 with and without Crush, no attack. Corrected: Crush clears the arrow danger (48 to 0); the Borg casts Crush and the orc dies.

- **Control: ogre (60 hp, no ranged attack) five steps away.** The changed line runs (the ogre is crushable and in view) but its ranged danger is already 0; both versions see danger 0, choose no attack, and the logs are byte-identical.

- **Control: dark dwarven lord (132 hp, too strong to crush).** Above the 104 hit-point limit Crush would not kill it; both versions keep danger 720, choose no attack, and the logs are byte-identical.

## Conditions

Native Human Necromancer, seed 66, level 26, depth 20, 183 hit points, carrying [Dark Rituals] with only Crush learned (no other attack spell, no missiles), in a synthetic 40x40 arena. The Borg's map is copied from the game cave and every grid is marked in view; one monster is placed five steps south, made aware and tracked by the Borg as awake. The production borg_attack(false) runs once; the keys it queues are resolved to the book and spell and executed with the real CMD_CAST command. The danger lines are diagnostic calls of borg_danger() with and without the Crush flag, the same calls borg_attack_aux_crush() makes. Cases: blacklock mage (ranged spells), cave orc (innate arrows), ogre (melee only, control), dark dwarven lord (132 hit points, too strong to crush, control).

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/crushv2-66
git fetch https://github.com/diegoami/neo-angband-tracker.git task/crushv2-66-evidence
git show 708efbc836e42549271cf51fcd12fcc6394266a0:docs/audit/c-reports/crushv2-66/reproduce.py > /tmp/reproduce-crushv2-66.py
python3 /tmp/reproduce-crushv2-66.py --full-tests --fixtures-ref 708efbc836e42549271cf51fcd12fcc6394266a0 --fix-ref 9757b161717b68d6c2d9911c46632a817856af51
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-danger.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `9757b161717b68d6c2d9911c46632a817856af51` (branch `task/crushv2-66`), fixtures
  `708efbc836e42549271cf51fcd12fcc6394266a0` (branch `task/crushv2-66-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: melee, tough
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The ranged-danger Crush adjustment in borg_danger_one_kill() cleared v1 (melee danger, already cleared by the first Crush block) instead of v2 (ranged danger), so Crush never reduced the danger of a crushable monster that attacks from range. Clearing v2 makes a level-26 Necromancer cast Crush on a blacklock mage (danger 936 to 0) and a cave orc archer (48 to 0) five steps away, and the real cast kills each; the ogre (no ranged danger) and dark dwarven lord (too strong) controls are byte-identical.

Synthetic arena with every grid marked in view and only Crush learned, so Crush is the one candidate attack; with other spells known the Borg compares Crush's danger reduction against their damage and may still pick another attack. Measured: the danger values, the attack decision and the real cast's result in one turn. Not measured: whole-game survival or kill rates. The first Crush block already clears v1 under the same condition, so the original second block changed nothing.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
