# The borg's speed model misses Slow and under-rates Terror

This is **speedmodel-648 / issue #648**, a C Borg defect
(diegoami/neo-angband-tracker#648).

While a speed effect is active, the borg keeps a one-number model of how fast its character is (BI_SPEED, starting at the game's normal 110) and compares it with every monster's speed to decide when it can safely approach, wait, or trade blows. The model added +10 for haste or sprint and otherwise +5 for terror, and never applied the slow penalty at all - the engine applies +10 haste/sprint, -10 slow and +10 terror, independently. So a slowed character is planned at full speed, a terrified one at half the real bonus, and haste plus terror loses the terror bonus. The fixture sets each timed effect through the real engine function, recomputes the game's own speed and one production borg_notice(), and requires the two numbers to match.

## What to watch

The Borg's modelled speed score (the game's own speed is on the same log's SPEEDINFO line); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `574b618c3`) | Corrected (fix `4a2f98f68`) |
| --- | --- | --- |
| No timed effect (control) | passed, borg=110 | byte-identical log |
| Haste only (control) | passed, borg=120 | byte-identical log |
| Slowed | failed, borg=110 | passed, borg=100 |
| Terrified | failed, borg=115 | passed, borg=120 |
| Haste and terror together | failed, borg=120 | passed, borg=130 |
| Slow and terror together | failed, borg=115 | passed, borg=110 |

- **No timed effect (control).** No timed effect is active. Both variants report speed 110, the game's own value, and the complete logs are byte-identical.

- **Haste only (control).** Only haste is active. Both variants already add the engine's +10 and report 120; the complete logs are byte-identical.

- **Slowed.** The character is slowed. The game grants speed 100 (its -10 penalty); the original borg keeps planning at 110 - it believes it acts twice as fast as it does (energy 10 per turn budgeted where the game grants 5). The corrected borg reports 100.

- **Terrified.** Only terror is active. The engine adds +10 (speed 120); the original borg adds only 5 (115), so it thinks a monster at 120 is not faster. The corrected borg reports 120.

- **Haste and terror together.** Haste and terror are active at once. The engine adds +10 and +10 (130); the original borg's else-if drops the terror bonus entirely and reports 120. The corrected borg reports 130.

- **Slow and terror together.** Slow and terror are active at once. The engine nets -10 + 10 (110); the original borg ignores the slow penalty and adds a half terror bonus (115). The corrected borg reports 110.

## Conditions

Native Human Warrior, level 18 (experience forced), depth 5 in a synthetic open lit arena with no monsters, a wooden torch as the only light, infravision 0, unencumbered. The timed effects are set with the engine's own player_set_timed() (no message), the game's speed is recomputed with PU_BONUS, and one production borg_notice() derives the borg's model. No combat, movement or random placement consumes the RNG (fixed seed 648), so a scenario log is fully determined by its mode. Limits: the fixture measures the speed model itself, not a specific fight; the consumers of BI_SPEED (monster-speed comparisons in approach/wait decisions, energy-per-turn danger math, power valuation) are quoted from the source and not separately exercised.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario to see the original and corrected runs side by side on the dungeon map, drawn with the game's own symbols (the legend under the maps explains them), press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/speedmodel-648
git fetch https://github.com/diegoami/neo-angband-tracker.git task/speedmodel-648-evidence
git show 28cc1003576881e4b56a2a9a1b1d0408563e78b0:docs/audit/c-reports/speedmodel-648/reproduce.py > /tmp/reproduce-speedmodel-648.py
python3 /tmp/reproduce-speedmodel-648.py --full-tests --fixtures-ref 28cc1003576881e4b56a2a9a1b1d0408563e78b0 --fix-ref 4a2f98f684393e81041144245cf233354535e804
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-trait.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix `4a2f98f684393e81041144245cf233354535e804` (branch `task/speedmodel-648`), fixtures
  `28cc1003576881e4b56a2a9a1b1d0408563e78b0` (branch `task/speedmodel-648-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: none, fast
- full native gates: before 966/966 unit checks and 5/5 frontend tests;
  after 966/966 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The borg's one-number speed model diverges from the game's own speed exactly as described: unchanged with no effect (110) and with haste alone (120), wrong for slow (110 modelled where the game grants 100, with 10 energy budgeted per turn where the game grants 5), terror (115 modelled where the game grants 120), haste plus terror (120 where the game grants 130) and slow plus terror (115 where the game grants 110). With the correction every modelled value equals the game's, and the two unchanged controls are byte-identical. Both full native gates pass on both variants.

The fixture measures the speed model itself, not a specific fight. The consumers of BI_SPEED (monster-speed comparisons in the approach and wait checks, the energy-per-turn indexing in danger and pathing math, power valuation) are identified in the source and their inputs change with the model, but no individual combat decision is replayed here. Encumbrance, the speed curse and bloating adjust the same trait by their own rules and are untouched by this change.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
