# The Borg underrated its Cure Serious Wounds potions

This is **healamount-592 / issue592**, a C Borg defect
(diegoami/neo-angband-tracker#592), found by reading `borg_heal()` against our C manual (HEAL-02) and the game data.

Before drinking a cure potion, the Borg checks that the potion will heal enough to cover the danger it faces, and that the heal is worth at least a third of that danger ("no rope-a-doping"). It estimated **Cure Light, Serious and Critical Wounds** as a percentage of the hit points lost — 15, 20 and 25 %, at least 15, 25 and 30 — and its comments still call them 2d10, 4d10 and 6d10, the 3.x amounts. In 4.2 these potions heal a **fixed 20, 40 and 60** (`object.txt` `HEAL_HP` dice with no percentage part; `effect_handler_HEAL_HP`). So at low hit points the Borg expected too little and left a potion in its pack that would have covered the danger. The correction uses the fixed amounts; the Potion of Healing estimate (300, or 35 % of the hit points lost) already matches the game and is unchanged.

## What to watch

The Borg drinks Cure Serious Wounds (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `65151df9e`) |
| --- | --- | --- |
| Danger 89, hit points 57/103 (danger 32 above current HP) | failed: no potion; "# Fleeing (failed to teleport)", "# Reading Scroll of Word of Recall." | passed: "# Quaffing Potions of Cure Serious Wounds.", "# Healing Level 2." |
| Control: danger 89, hit points 39/70 (50 above current HP) | passed: no potion (it cannot cover the danger) | byte-identical log |

In the changed case the potion's real 40 covers the danger (89 < 57 + 40) and is more than a third of it; the original's 25 fails both tests (89 ≥ 57 + 25, and 25 < 89 / 3), so it reads Word of Recall instead, an escape that takes effect only many turns later.

## Conditions

Native Human Warrior, seed 592, character level 10, dungeon level 10, lit room, a torch, three Cure Serious Wounds, a light cut, an awake black ogre adjacent. Fixture interventions: the danger D at the Borg's grid is read first; then current and maximum hit points are set so D is 32 above current hit points (50 in the control) with 45 % of the maximum lost (the Cure Serious Wounds band); the cut timer is set to 5, because a cut Borg skips the random "heal only sometimes" test (#364), so the amount test decides alone; the ogre enters the Borg's monster list through the production `observe_kill_diff()`. The production `borg_caution()` runs once.

[replay.html](replay.html) is the offline page (download raw, open locally): choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/healamount-592
git fetch https://github.com/diegoami/neo-angband-tracker.git task/healamount-592-evidence
git show 5c6a2aa4bc11f6dabe7bbb13fec7bd69fbe5a354:docs/audit/c-reports/healamount-592/reproduce.py > /tmp/reproduce-healamount-592.py
python3 /tmp/reproduce-healamount-592.py --full-tests --fixtures-ref 5c6a2aa4bc11f6dabe7bbb13fec7bd69fbe5a354 --fix-ref 65151df9e89079471faf18e7501e116f175238c3
```

The runner asserts the correction is exactly `fix.patch` (`git diff BASE FIX`, `src/borg/borg-caution.c` only), builds the original and corrected native C game in a disposable worktree, runs both scenarios, checks each expected result and the byte-identical control, runs both full native gates and writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`; the frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `65151df9e89079471faf18e7501e116f175238c3` (branch `task/healamount-592`), fixtures `5c6a2aa4bc11f6dabe7bbb13fec7bd69fbe5a354` (branch `task/healamount-592-evidence`); later commits only add these artifacts.
- control byte-identical: control
- full native gates: before and after 965/965 unit checks and 5/5 frontend tests
- corrected source sha256 (`src/borg/borg-caution.c`): `bff3d023198d10d3ca5fdc9f34fd88c1e93dddf6f870e4c054eba1ad1680b961`
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

At low hit points the original Borg underrated Cure Serious Wounds (25 instead of 40), failed both its heal tests, and read Word of Recall while carrying a potion that covered the danger; with the fixed amounts it drinks the potion. Danger beyond what the potion covers is unchanged.

Not measured: the fight or the recall that follow, deaths, how often play reaches this band, the Cure Light and Cure Critical rungs (same one-line change, not exercised here), and the high-hit-point direction, where the old percentages overrated the potions (e.g. Cure Light Wounds expected to heal 15 % of a large loss). The random skip test of #364 still applies to an uncut Borg; the fixture's cut bypasses it so this change can be seen alone.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`) are the generators; `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file) are the captured run.
