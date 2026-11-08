# The Borg shot at a sleeping troll because its melee danger came out at 5%

This is **hitchance-63 / issue63**, a C Borg defect
(diegoami/neo-angband-tracker#63).

Before it attacks a sleeping monster from a distance, the Borg asks how dangerous that monster would be if it woke, and leaves hard sleeping monsters alone. That estimate scales a monster's melee damage by the chance that its blows hit. The chance was computed as 150 minus the armour term armour*75 minus 3*(level+power) where it should divide the armour term by the monster's to-hit. Any armour class of 2 or more therefore gave the 5% floor, and a higher monster level lowered the chance. A sleeping cave troll counted as harmless and the Borg shot at it. The correction divides armour by the to-hit, so the chance falls as armour rises and rises with the monster's level, and the check works.

## What to watch

The Borg shoots at the sleeping monster (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `5c7950a25`) |
| --- | --- | --- |
| Sleeping cave troll six grids away | failed, shoots=1 | passed, shoots=0 |
| Control: sleeping boil-covered wretch | passed, shoots=1 | byte-identical log |

- **Sleeping cave troll six grids away.** A level-15 Warrior at full hit points (177) with 10 armour sees a sleeping cave troll six grids east. The production ranged-attack scorer rates a bolt at it. Original: the partial melee danger of the troll is 0 (a 5% hit chance), so the check that avoids waking a hard sleeping monster does not trigger and the bolt scores 12. Corrected: the partial melee danger is 234 (full-damage danger is 162), well above half the Borg's avoidance (88), so the scorer refuses the attack.

- **Control: sleeping boil-covered wretch.** The same Warrior and a sleeping boil-covered wretch, a level-0 monster whose only blow does no damage. It goes through the same hit-chance lines, the danger is 0 before and after, and the bolt scores 12 in both.

## Conditions

Native Human Warrior, seed 63, character level 15, 177 hit points, 10 armour, depth 5, a lit open arena, a lit torch. The production borg_launch_bolt() scores a bolt of damage 12 (about a Magic Missile) at the monster; the fixture builds the target list for the one monster in view and sets the attacking flag exactly as borg_attack() does, and the monster is marked asleep in the game and in the Borg's monster list. Nothing is fired or played. All partial-damage callers share the corrected estimate; only this one decision is measured.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/hitchance-63
git fetch https://github.com/diegoami/neo-angband-tracker.git task/hitchance-63-evidence
git show d1ac3dce6c21906d049d8589e16ab43b1afe8390:docs/audit/c-reports/hitchance-63/reproduce.py > /tmp/reproduce-hitchance-63.py
python3 /tmp/reproduce-hitchance-63.py --full-tests --fixtures-ref d1ac3dce6c21906d049d8589e16ab43b1afe8390 --fix-ref 5c7950a25e7fb62eec1987f0fe8a4f43f4bf3984
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-danger.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `5c7950a25e7fb62eec1987f0fe8a4f43f4bf3984` (branch `task/hitchance-63`), fixtures
  `d1ac3dce6c21906d049d8589e16ab43b1afe8390` (branch `task/hitchance-63-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: wretch
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The original partial melee estimate gave the sleeping cave troll a danger of 0, so a level-15 Warrior's ranged-attack scorer rated a bolt at it at 12 and the troll would be woken. With armour divided by the monster's to-hit the estimate is 234, above half the Borg's avoidance, and the scorer refuses the attack. A control with a harmless sleeping monster is byte-identical.

Priority medium at most: this measures one decision (whether a bolt at a sleeping monster is scored) in one constructed scene. The corrected estimate includes the original's 50% bad-luck allowance, so for light armour it exceeds the full-damage figure (234 against 162 here). The other partial-damage callers (the defence and flow code) share the corrected estimate and their decisions are not measured. It does not measure a fight, a death, or how the Borg does over a whole game; the owner decision on #69 keeps borg_danger() on the full-damage path.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
