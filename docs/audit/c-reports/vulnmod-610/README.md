# The Borg did not model an unresisted elemental vulnerability

This is **vulnmod-610**, a C Borg defect (diegoami/neo-angband-tracker#610). It was split from #594 and found by reading the Borg's danger code against the game's `adjust_dam()`.

**The game.** A **vulnerability** (for example from the *burning up* curse, `RES_FIRE[-1]`) works like this:
- With no permanent resistance to that element, it puts the resistance level at **-1**, and `adjust_dam()` in `project-player.c` multiplies the damage by **4/3**.
- Temporary resistance (for example from a Resist Heat potion) then only brings the level back to **0**: no reduction (`calc_bonuses()` in `player-calcs.c`).

**The Borg.** `borg_danger()` divided by three for permanent and temporary resistance, but never multiplied for a vulnerability. So it underrated fire, cold, electricity and acid attacks, and with temporary resistance it counted a reduction the game does not give.

**The correction:**
- It records the unresisted vulnerability as `BI_VFIRE`, `BI_VELEC`, `BI_VCOLD` and `BI_VACID`. This happens where #594 already weighs the vulnerability against the permanent resistance.
- It applies the game's rule in the 17 element cases of `borg_danger()` (melee, breath, ball, bolt and beam): ×4/3 without temporary resistance, no reduction with it.
- It is stacked on the #594 correction (`82fd25a54`), which is the base here. Upstream base: angband/master `42cd156ac`.

## What to watch

The danger the Borg assigns to its grid with a young red dragon (fire breath) in range. Each run first reads a **reference** with no fire resistance, no vulnerability and no temporary resistance (game level 0), then the scenario.

| Scenario | Game's fire level | Original (base `82fd25a54`) | Corrected (fix `16a8ac653`) |
| --- | --- | --- | --- |
| Fire vulnerability, no resistance | **-1** (×4/3) | failed: danger **137** = reference 137 | passed: danger **169** > reference 137 |
| Vulnerability + temporary fire resistance | **0** (no reduction) | failed: danger **73** < reference 137 | passed: danger **137** = reference |
| Control: resistance cancelled by the vulnerability (as #594) | 0 | passed: 137 = 137 | byte-identical log |

The 169 is not 137 × 4/3, because the danger also counts the dragon's other attacks; only the fire part is scaled.

## Conditions

- Character: native Human Warrior, seed 610, character level 35, 427 hit points, dungeon level 35, lit arena.
- Monster: an awake young red dragon three grids away.
- Worn: a Soft Leather Armour and a Leather Shield. The fixture sets their fire resistance levels and the temporary fire resistance timer (`TMD_OPP_FIRE`) for each reading.
- Fixture interventions: the items are made directly, and all runes are known. The dragon enters the Borg's monster list through the production `observe_kill_diff()`.
- Readings: the game's own resistance level (`player->state.el_info[ELEM_FIRE].res_level`) and the production `borg_danger()`.

[replay.html](replay.html) is the offline page (download it raw and open it locally). Choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/vulnmod-610
git fetch https://github.com/diegoami/neo-angband-tracker.git task/vulnmod-610-evidence
git show 91685999bb307a8f32329b4ea5628ea535d0a0e4:docs/audit/c-reports/vulnmod-610/reproduce.py > /tmp/reproduce-vulnmod-610.py
python3 /tmp/reproduce-vulnmod-610.py --full-tests --fixtures-ref 91685999bb307a8f32329b4ea5628ea535d0a0e4 --fix-ref 16a8ac653bc4f6e9564f7b914358a1ebe8a16e1f
```

The runner:
- asserts that the correction over the #594 base is exactly `fix.patch` (`src/borg/borg-danger.c`, `src/borg/borg-trait.c`, `src/borg/borg-trait.h`);
- builds the original and corrected native C game in a disposable worktree;
- runs the three scenarios and checks each expected result and the byte-identical control;
- runs both full native gates and writes `replay.html`.

Heavy steps wait on `/tmp/angband-validation.lock`. The frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `82fd25a5477054a0727ab920701b7555898586b2` (the #594 correction, branch `task/vulnres-594`, on angband/master `42cd156ac`)
- fix `16a8ac653bc4f6e9564f7b914358a1ebe8a16e1f` (branch `task/vulnmod-610`)
- fixtures `91685999bb307a8f32329b4ea5628ea535d0a0e4` (branch `task/vulnmod-610-evidence`); later commits only add these artifacts
- control byte-identical: control
- full native gates: 965/965 unit checks and 5/5 frontend tests, before and after
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

In the measured fire cases the corrected danger follows the game's resistance level:
- level -1: higher than with no resistance;
- level 0: equal to no resistance.

The original ignored the vulnerability, and with temporary resistance it nearly halved the danger.

Not measured:
- the cold, electricity and acid lines (same shape, read only);
- melee, ball, bolt and beam (the dragon's fire breath dominates here);
- decisions that follow, fights or deaths.

Not changed: the cold melee case tests temporary **acid** resistance, a separate slip filed as #616 and left as it is here.

## Files

- Generators: `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`).
- Captured run: `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file).
