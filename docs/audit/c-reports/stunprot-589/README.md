# The Borg treated sound resistance as protection from stun

This is **stunprot-589 / issue589**, a C Borg defect
(diegoami/neo-angband-tracker#589), found by reading the Borg's danger code against our strategy manual (`docs/strategy/04-resistances-afflictions.md`) and the game code.

Gravity, plasma and force breath, water balls and bolts, storms, plasma bolts and ice bolts can all stun. In the game (`src/project-player.c`) only **protection from stun** stops that; **sound resistance** stops only the stun of sound itself. The Borg's danger estimate adds a large penalty (+500, or +1000 heavily stunned) when an already-stunned Borg faces another stunning attack — but for all of these attacks it skipped the penalty whenever the Borg resisted sound, and it did not track protection from stun at all. So a stunned Borg with sound resistance saw a gravity hound as harmless to its stun, and a Borg protected from stun was needlessly afraid. The correction adds a protection-from-stun trait (`BI_PSTUN`, from the player's own flags and worn items), uses it for those attacks, and lets it also stop sound's stun.

## What to watch

The extra danger the Borg assigns to being stunned again by the gravity hound; the suite line is the scenario's own pass/fail check.

| Scenario (stunned Borg, gravity hound in range) | Original (base `42cd156ac`) | Corrected (fix `3088b91d9`) |
| --- | --- | --- |
| Sound resistance, no stun protection | failed: penalty **0**, danger 156 | passed: penalty 500, danger **656** |
| Stun protection, no sound resistance | failed: penalty **500**, danger 656 | passed: penalty 0, danger **156** |
| Both | passed: penalty 0, danger 156 | passed: penalty 0, danger 156 |
| Control: not stunned (sound resistance) | passed: penalty 0 | byte-identical log |

The Borg has 402 hit points here: the original estimate (156) is well under them; the corrected one (656) is above them, the range in which the Borg treats a position as potentially lethal.

## Conditions

Native Human Warrior, seed 589, character level 35, 402 hit points, dungeon level 35, lit arena, an awake **gravity hound** three grids east. Fixture interventions: one Soft Leather Armour is made directly with sound resistance and/or protection from stun and worn; all runes known; the stun timer is set to 10 (a light stun) except in the control; the hound enters the Borg's monster list through the production `observe_kill_diff()`. The production `borg_danger()` is read at the Borg's grid, then again with only the Borg's two "stunned" traits cleared; the difference is the stun penalty.

[replay.html](replay.html) is the offline page (download raw, open locally): choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stunprot-589
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stunprot-589-evidence
git show ba9ebcb174a61055436b9ae2e0a60adccd733c17:docs/audit/c-reports/stunprot-589/reproduce.py > /tmp/reproduce-stunprot-589.py
python3 /tmp/reproduce-stunprot-589.py --full-tests --fixtures-ref ba9ebcb174a61055436b9ae2e0a60adccd733c17 --fix-ref 3088b91d965ca483917377e4e1553329b7e292e8
```

The runner asserts the correction is exactly `fix.patch` (`git diff BASE FIX`, three files), builds the original and corrected native C game in a disposable worktree, runs all four scenarios, checks each expected result and the byte-identical control, runs both full native gates and writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`; the frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `3088b91d965ca483917377e4e1553329b7e292e8` (branch `task/stunprot-589`), fixtures `ba9ebcb174a61055436b9ae2e0a60adccd733c17` (branch `task/stunprot-589-evidence`); later commits only add these artifacts.
- control byte-identical: rsound-unstunned
- full native gates: before and after 965/965 unit checks and 5/5 frontend tests
- corrected source sha256: `borg-danger.c` `ffe1632e…6c518be9f2a`, `borg-trait.c` `df524094…44882e9ff71`, `borg-trait.h` `87cd8a47…6f6dd756b1` (full values in `result.json`)
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

For gravity breath, the Borg's stun penalty now follows protection from stun, as the game does: with sound resistance alone it applies (the original skipped it), with stun protection it does not (the original applied it). The same one-word substitution covers plasma and force breath, water ball and bolt, storm, plasma bolt and ice bolt (the other seven `borg_danger` cases), which share the gravity case's structure; only gravity is exercised here.

Not measured: the caution or escape decision that follows from the changed danger, fights, deaths, or how often a stunned Borg meets these attackers. The Borg does not value or seek protection-from-stun gear; this change only lets it recognise the protection it wears.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`) are the generators; `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file) are the captured run.
