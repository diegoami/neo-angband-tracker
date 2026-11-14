# With only teleport scrolls, the Borg destroyed the area instead of teleporting

This is **tportcheck-593 / issue593**, a C Borg defect (diegoami/neo-angband-tracker#593), found during the strategy-manual comparison (C-FIX-POSTING §8.1).

Before casting Destruction or Teleport Level in a dangerous fight, the Borg asks `borg_caution_teleport()` whether a plain teleport would land somewhere safe; if so, the defence steps aside and the Borg teleports instead (the code's comment: otherwise "too often he will die out-of-the-frying-pan-into-the-fire"). That check returned at once unless the Borg had **both** a teleport scroll or spell (`BI_ATELEPORT`) **and** a teleport staff or activation (`BI_AESCAPE`) — but its callers ask it when the Borg has either one. So a Borg with only Teleportation scrolls (or only a staff) never ran the check and used Destruction, wiping out the area and everything on its floor, even with a safe landing available. The correction runs the check with either kind of teleport (`||` → `&&` in the "must have the ability" test).

## What to watch

The Borg uses Destruction instead of teleporting (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `edc4dcb3f`) |
| --- | --- | --- |
| Teleportation scrolls only | failed: landing check not run; "# Performing defense type 22 with value 2000", "# Using Staff of *Destruction* (5 charges)." | passed: safe landing found, no defence taken, no Destruction |
| Control: a Staff of Teleportation as well | passed: safe landing found, no Destruction | byte-identical log |

## Conditions

Native Human Warrior, seed 593, character level 40, 300 of 466 hit points, dungeon level 40, a fully known lit 66×198 arena (so teleport landings 50–100 squares away can be checked), an awake ancient blue dragon adjacent (danger 2000). Carried: five Scrolls of Teleportation and a Staff of \*Destruction\* with 5 charges (plus a Staff of Teleportation in the control). Fixture interventions: the map is built directly; hit points set to 300 (the Destruction defence needs at least 275, and the danger must exceed the Borg's avoidance); the items are made directly; the dragon enters the Borg's monster list through the production `observe_kill_diff()`. The production `borg_caution_teleport(75, 2)` and `borg_defend()` run once; nothing is played.

[replay.html](replay.html) is the offline page (download raw, open locally): choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tportcheck-593
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tportcheck-593-evidence
git show bcc2c86aef432b1aaa6824b02f41fea84bc23b3b:docs/audit/c-reports/tportcheck-593/reproduce.py > /tmp/reproduce-tportcheck-593.py
python3 /tmp/reproduce-tportcheck-593.py --full-tests --fixtures-ref bcc2c86aef432b1aaa6824b02f41fea84bc23b3b --fix-ref edc4dcb3ff4c869b1fd366dd87330751a0de4d9c
```

The runner asserts the correction is exactly `fix.patch` (`src/borg/borg-escape.c` only), builds the original and corrected native C game in a disposable worktree, runs both scenarios, checks each expected result and the byte-identical control, runs both full native gates and writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`; the frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `edc4dcb3ff4c869b1fd366dd87330751a0de4d9c` (branch `task/tportcheck-593`), fixtures `bcc2c86aef432b1aaa6824b02f41fea84bc23b3b` (branch `task/tportcheck-593-evidence`); later commits only add these artifacts.
- control byte-identical: control
- full native gates: before and after 965/965 unit checks and 5/5 frontend tests
- corrected source sha256 (`src/borg/borg-escape.c`): `2c908a89ac7515f8a1b55818ebc0e19fa703ea5c288857b6bc7801f81b390d0a`
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

With teleport scrolls only, the original Borg skipped its own landing check and used a Staff of \*Destruction\* next to an ancient dragon; with the correction the check runs, finds a safe landing, and the Destruction defence steps aside. With both kinds of teleport, nothing changes.

Not measured: the escape that follows (here the escape code would read a Teleportation scroll), the fight, deaths, loot lost to Destruction, or the Teleport Level defence and staff-only case, which take the same path but are not exercised here. Whether teleporting or destroying is better in a given fight is the existing design's judgement (its landing check); this change only lets that judgement run.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`) are the generators; `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file) are the captured run.
