# A fire vulnerability did not cancel the Borg's fire resistance

This is **vulnres-594 / issue594**, a C Borg defect (diegoami/neo-angband-tracker#594), found by reading the Borg's resistance traits against our strategy manual and the game code.

In the game a **vulnerability** — for example from the *burning up* (`RES_FIRE[-1]`), *chilled to the bone* or *steelskin* curses — takes away one level of the matching permanent resistance unless the player is immune (`calc_bonuses()` in `player-calcs.c`). So a resistance plus a vulnerability leaves **no** resistance. The Borg recorded the vulnerability (`BI_CRSFVULN` and its siblings) only to lower the value of its gear in `borg_power()`; its resistance trait (`BI_RFIRE`…) stayed set, so its danger estimate kept dividing that element's damage by three. The correction clears the permanent resistance trait when a vulnerability cancels it (fire, cold, electricity, acid); temporary resistance is still counted separately, as before.

## What to watch

The danger the Borg assigns to its grid with a young red dragon (fire breath) in range; the suite line checks that the Borg's fire resistance matches the game's.

| Scenario | Game's fire resistance | Original (base `42cd156ac`) | Corrected (fix `82fd25a54`) |
| --- | --- | --- | --- |
| Fire resistance + fire vulnerability | level **0** (none) | failed: Borg believes it resists; danger **72** | passed: Borg agrees; danger **134** |
| Control: fire resistance only | level 1 | passed: danger 72 | byte-identical log |

## Conditions

Native Human Warrior, seed 594, character level 35, 418 hit points, dungeon level 35, lit arena, an awake young red dragon three grids away. Worn: a Soft Leather Armour with fire resistance; in the vulnerability case also a Leather Shield with a fire vulnerability (`RES_FIRE[-1]`, as the *burning up* curse gives). Fixture interventions: the items are made directly; all runes known; the dragon enters the Borg's monster list through the production `observe_kill_diff()`. The game's own resistance level (`player->state.el_info[ELEM_FIRE].res_level`) and the production `borg_danger()` are read.

[replay.html](replay.html) is the offline page (download raw, open locally): choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/vulnres-594
git fetch https://github.com/diegoami/neo-angband-tracker.git task/vulnres-594-evidence
git show 6ec100d75c00ad1d0712491b0dd7d3e20a86a075:docs/audit/c-reports/vulnres-594/reproduce.py > /tmp/reproduce-vulnres-594.py
python3 /tmp/reproduce-vulnres-594.py --full-tests --fixtures-ref 6ec100d75c00ad1d0712491b0dd7d3e20a86a075 --fix-ref 82fd25a5477054a0727ab920701b7555898586b2
```

The runner asserts the correction is exactly `fix.patch` (`src/borg/borg-trait.c` only), builds the original and corrected native C game in a disposable worktree, runs both scenarios, checks each expected result and the byte-identical control, runs both full native gates and writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`; the frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `82fd25a5477054a0727ab920701b7555898586b2` (branch `task/vulnres-594`), fixtures `6ec100d75c00ad1d0712491b0dd7d3e20a86a075` (branch `task/vulnres-594-evidence`); later commits only add these artifacts.
- control byte-identical: control
- full native gates: before and after 965/965 unit checks and 5/5 frontend tests
- corrected source sha256 (`src/borg/borg-trait.c`): `ca454d64b877440157c6a6541251a84ac6b9817f463702a1e30d15a30245bde0`
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

With fire resistance and a fire vulnerability, the game computes no resistance, the original Borg still assumed one, and its fire danger was less than half the corrected value; with the correction the Borg's trait matches the game. Without a vulnerability nothing changes.

Not measured: decisions that follow, fights or deaths; the cold, electricity and acid lines (same shape, not exercised). Not changed here: with **no** resistance a vulnerability makes the game deal 4/3 damage (`adjust_dam()`), which `borg_danger` does not model — that is a separate, wider change (about a dozen element cases) recorded on the issue.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`) are the generators; `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file) are the captured run.
