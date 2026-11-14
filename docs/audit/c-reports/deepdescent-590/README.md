# About to die, the Borg read Deep Descent, which takes effect turns later

This is **deepdescent-590 / issue590**, a C Borg defect (diegoami/neo-angband-tracker#590), found by reading the Borg's escape code against our C manual (ESC-04) and the game code.

When the Borg is about to die it runs down a list of escapes and stops at the first one that works. One entry, `borg_teleport_off_level()`, falls back to **Deep Descent** when the Borg has no Teleport Level, and the list treats that as a successful flight. But in the game Deep Descent only acts after **3+1d4 player turns** (`effect_handler_DEEP_DESCENT`) and then drops the player to a level **below the deepest one reached** (`game-world.c`, `dungeon_get_next_level(player, player->max_depth, ...)`). So the Borg spent its last turn without moving, and the escapes after that entry — retries at a higher fail rate, Teleport Level by spell, **Phase Door** — were never tried. The correction lets only the one caller that wants a delayed dive (starving on dungeon level 1 with no known stairs) use Deep Descent; the three danger escapes use Teleport Level only.

## What to watch

The Borg reads Deep Descent as its emergency escape (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `e8c8cae0d`) |
| --- | --- | --- |
| About to die, carrying Deep Descent and Phase Door | failed: "# Reading Scroll of Deep Descent.", "# Danger Level 1." | passed: "# Safe to Phase. scary squares: 0", "# Reading Scrolls of Phase Door." |
| Control: Teleport Level also carried | passed: "# Reading Scroll of Teleport Level." | byte-identical log |

## Conditions

Native Human Warrior, seed 590, character level 10, 44 of 119 hit points, dungeon level 10, lit room, an awake black ogre adjacent (danger 89, more than 1.5 times the Borg's avoidance of 44, so the "about to die" escape — Danger Level 1 — applies). Carried escapes: one Scroll of Deep Descent and five Scrolls of Phase Door (plus one Scroll of Teleport Level in the control); no teleport, no potions. Fixture interventions: the danger is read first and hit points are set to half of it; the scrolls are made directly; the ogre enters the Borg's monster list through the production `observe_kill_diff()`. The production `borg_caution()` runs once; the read is not played.

[replay.html](replay.html) is the offline page (download raw, open locally): choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/deepdescent-590
git fetch https://github.com/diegoami/neo-angband-tracker.git task/deepdescent-590-evidence
git show 860efeb9007d4d90e93ad2ad9aa2d3f8c8815292:docs/audit/c-reports/deepdescent-590/reproduce.py > /tmp/reproduce-deepdescent-590.py
python3 /tmp/reproduce-deepdescent-590.py --full-tests --fixtures-ref 860efeb9007d4d90e93ad2ad9aa2d3f8c8815292 --fix-ref e8c8cae0df448c04259264e6427d28abf65605a1
```

The runner asserts the correction is exactly `fix.patch` (`src/borg/borg-escape.c` only), builds the original and corrected native C game in a disposable worktree, runs both scenarios, checks each expected result and the byte-identical control, runs both full native gates and writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`; the frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `e8c8cae0df448c04259264e6427d28abf65605a1` (branch `task/deepdescent-590`), fixtures `860efeb9007d4d90e93ad2ad9aa2d3f8c8815292` (branch `task/deepdescent-590-evidence`); later commits only add these artifacts.
- control byte-identical: control
- full native gates: before and after 965/965 unit checks and 5/5 frontend tests
- corrected source sha256 (`src/borg/borg-escape.c`): `6242f2b1d659dfc32a83811e87ea9d20078843657aab66ecb9965d7bd0d033ee`
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

About to die, the original Borg spent its turn reading a Deep Descent that would act only 4–7 turns later and counted itself as fled; the corrected Borg skips it and reads Phase Door, an immediate escape. With Teleport Level carried, nothing changes.

Not measured: whether the Phase Door saves the Borg, the fight, deaths, or how often the Borg carries Deep Descent without Teleport Level (it finds the scrolls; it does not buy them). The Danger Level 3.4 and 8 escapes get the same change but are not exercised here. The starving-on-level-1 exit keeps Deep Descent by design.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`) are the generators; `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file) are the captured run.
