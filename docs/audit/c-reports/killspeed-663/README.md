# Danger and retreat comparisons use monster speed before Haste and Slow

This is **killspeed-663 / issue #663**, a C Borg defect
(diegoami/neo-angband-tracker#663).

The C tracker copies the monster's base mspeed into kill->speed on both tracking paths, and the danger calculation turns that value into an energy rate while the retreat logic compares it with the player's speed. The engine, before giving a monster energy, adds 10 for a hasted monster and subtracts twice the Slow effect level from a slowed one - so a visibly hasted monster outran the borg sooner than the danger and retreat comparisons predict, and the borg's own Slow spell bought no credit.

## What to watch

The monster speed the borg tracks (kill->speed, the energy input for the danger math and the retreat comparisons); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `574b618c3`) | Corrected (fix `91285793b`) |
| --- | --- | --- |
| A monster hasted when first seen (new-record path) | failed, tracked=110 | passed, tracked=120 |
| Haste arriving after the first sighting (existing-record path) | failed, tracked=110 | passed, tracked=120 |
| A slowed monster | failed, tracked=110 | passed, tracked=106 |
| The hasted eye goes out of sight | failed, tracked=110 | passed, tracked=120 |
| Control: no timed effect | passed, tracked=110 | byte-identical log |
| Control: the haste expires before the sighting | passed, tracked=110 | byte-identical log |

- **A monster hasted when first seen (new-record path).** A visible floating eye under Monster Haste stands three grids east; the production screen scan creates its record. The original tracks the base speed (110) although the engine grants the hasted eye 120 - the energy input for the danger math and the retreat gate is wrong from birth. The corrected borg tracks 120, and the eye's own-grid danger rises accordingly (200 to 400).

- **Haste arriving after the first sighting (existing-record path).** The eye is first seen plain (both track 110), then hasted; the refresh pass must pick the timed state up. The original keeps 110; the corrected borg tracks 120 on the existing-record path.

- **A slowed monster.** The eye under Monster Slow (one effect level): the engine takes 2 x level from its 110 base, net 106. The original tracks 110 - the borg's own Slow spell bought no credit. The corrected borg tracks 106.

- **The hasted eye goes out of sight.** The hasted eye is observed once (net 120) and then leaves sight. The corrected borg keeps the last defensible observation (120) instead of falling back to the base speed; the original never had it (110).

- **Control: no timed effect.** The eye at its base speed with no timed state: both variants track 110. The complete logs are byte-identical.

- **Control: the haste expires before the sighting.** The eye is seen with no timed effect (an expired haste reads exactly like none): both variants track 110. The complete logs are byte-identical.

## Conditions

Native Human Warrior, level 20 (experience forced), depth 5 in a synthetic open lit arena, a wooden torch as the only light, infravision 0. A visible stock floating eye (base mspeed 110) three grids east, awake and out of melee reach. The timed states are applied through the production mon_inc_timed(); the eye is observed by real production borg_update() passes - the first creates the kill record (borg_update_kill_new), a later pass refreshes it (borg_update_kill_old). The expected net speed is computed in the fixture from the live monster with the engine's own formula (mspeed + 10 hasted; mspeed - 2 x monster_effect_level(MON_TMD_SLOW) slowed), so the assertion does not hardcode numbers. The off-screen mode clears MFLAG_VISIBLE so the production scan can no longer read the monster.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario to see the original and corrected runs side by side on the dungeon map, drawn with the game's own symbols (the legend under the maps explains them), press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/killspeed-663
git fetch https://github.com/diegoami/neo-angband-tracker.git task/killspeed-663-evidence
git show a7360ad8248eb7a6ad89bd224fc3b976ecc77a34:docs/audit/c-reports/killspeed-663/reproduce.py > /tmp/reproduce-killspeed-663.py
python3 /tmp/reproduce-killspeed-663.py --full-tests --fixtures-ref a7360ad8248eb7a6ad89bd224fc3b976ecc77a34 --fix-ref 91285793b7bc08d866c71884a223635ef28ce473
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-kill.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix `91285793b7bc08d866c71884a223635ef28ce473` (branch `task/killspeed-663`), fixtures
  `a7360ad8248eb7a6ad89bd224fc3b976ecc77a34` (branch `task/killspeed-663-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: plain, expired
- full native gates: before 966/966 unit checks and 5/5 frontend tests;
  after 966/966 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The tracked monster speed is now the observed net speed the engine actually grants energy on: +10 under a visible Haste, -2 x effect level under a visible Slow, on both the new-record and the existing-record path, persisting as the last defensible observation when the monster goes out of sight. On the base commit the tracked speed stayed at the base mspeed in every one of those cases, so the danger energy and the kill->speed > BI_SPEED retreat and chooser gates priced a hasted pursuer as slower than it is. The no-timed-state and expired-haste readings are unchanged (byte-identical controls).

The fixture measures the tracked speed (the energy input) and reports the resulting own-grid danger from the production borg_danger; it does not replay an individual retreat or attack decision. The borg's speculative slow-spell modifier is untouched - the correction tracks the observed timed state, which is the input that modifier was missing. Haste is read from the live monster while visible (the same cheat the borg already applies to sleep, fear, confusion and stun); a haste applied entirely off-screen is not observable and stays out of scope.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
