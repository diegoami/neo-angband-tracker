# A Druid at full health drank Healing instead of casting Remove Hunger

This is **eathurt-80 / issue80**, a C Borg defect
(diegoami/neo-angband-tracker#80).

When the Borg is weak from hunger and has no food, borg_eat_food_any() falls back to cure potions, which also nourish a little (one percent of a full stomach each). The comment says it does this only 'when hurting', but the test was current hit points <= maximum hit points, which is always true. The caller tries this fallback before the Remove Hunger spell, so a level-11 Druid at full health, with Remove Hunger available and two Potions of Healing, drank a Potion of Healing. The correction tests current < maximum, and the Druid casts Remove Hunger.

## What to watch

Cast Remove Hunger instead of drinking a potion (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `8ae589ac5`) |
| --- | --- | --- |
| 69 of 69 hit points | failed, cast_remove_hunger=0 | passed, cast_remove_hunger=1 |
| Control: 64 of 69 hit points | passed, cast_remove_hunger=0 | byte-identical log |

- **69 of 69 hit points.** Original: 'Quaffing Potions of Healing.' (key q) at full hit points. Corrected: 'Casting Remove Hunger (0,2).' (key m).

- **Control: 64 of 69 hit points.** The Druid is hurt, so both versions drink the Potion of Healing; the logs are identical.

## Conditions

Native Human Druid, seed 80, level 11, depth 5, a lit 38x22 room with no monsters. The starting rations are removed (no food in the pack). The Druid carries its starting [Lesser Charms] and two Potions of Healing, and has learned Remove Hunger and cast it before (fixture sets the learned and worked spell flags); 18 of 18 mana. Food is set just below the Weak threshold. Main case 69 of 69 hit points; control 64 of 69 (hurt). The production borg_caution() runs once and the first queued key is read back. This isolates the starvation step; it does not play out later turns.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/eathurt-80
git fetch https://github.com/diegoami/neo-angband-tracker.git task/eathurt-80-evidence
git show 84ec69bca227897e7846c9189b5bf9c80205f79c:docs/audit/c-reports/eathurt-80/reproduce.py > /tmp/reproduce-eathurt-80.py
python3 /tmp/reproduce-eathurt-80.py --full-tests --fixtures-ref 84ec69bca227897e7846c9189b5bf9c80205f79c --fix-ref 8ae589ac5719e647ecc6c4da887268f64b8bcb46
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-item-use.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `8ae589ac5719e647ecc6c4da887268f64b8bcb46` (branch `task/eathurt-80`), fixtures
  `84ec69bca227897e7846c9189b5bf9c80205f79c` (branch `task/eathurt-80-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: hurt
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_eat_food_any() drinks cure potions for food 'when hurting', tested as CURHP <= MAXHP, which is always true. A weak level-11 Druid at full health with Remove Hunger and two Potions of Healing drinks a Potion of Healing (one percent of nourishment) instead of casting Remove Hunger. With CURHP < MAXHP it casts the spell; the hurt control is byte-identical.



## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
