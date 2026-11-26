# A live monster can round down to 0 tracked power and lose its attack value

This is **killpower-662 / issue #662**, a C Borg defect
(diegoami/neo-angband-tracker#662).

When the Borg tracks a monster it cheats the monster's true hit points from the game and stores an estimate called power, which drives danger math and attack scoring; damage against ordinary monsters is capped at twice that value. Both tracking paths - the first observation of a monster and the refresh of a known one - estimate the hit points by first rounding them into a percentage (100 * hp / maxhp) and then rounding again when converting back (maxhp * pct / 100). Small remainders are lost twice: a floating eye at 1 of its hit points becomes 9% and then 0 tracked power, so the Borg reads a living monster as having no health worth attacking - the recorded level-1 mage stopped shooting a wounded floating eye, stepped next to it, and was paralysed to death. The fixture places a visible floating eye in a lit arena, sets its true hp, runs two production borg_update() passes (so both tracking paths run) and requires the tracked power to equal the monster's true hp.

## What to watch

The Borg's tracked power (hit points) for the living monster; the game's true hp is on each log's KILLINFO line; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `574b618c3`) | Corrected (fix `f87b535af`) |
| --- | --- | --- |
| Eye at full health (control) | passed, borg_power=12 | byte-identical log |
| The 1-hp boundary | failed, borg_power=0 | passed, borg_power=1 |
| Two hp, just above the edge | failed, borg_power=1 | passed, borg_power=2 |
| Exactly 0 hp while alive (control) | passed, borg_power=0 | byte-identical log |

- **Eye at full health (control).** The floating eye at full health. Both variants track its power correctly; the complete log is byte-identical.

- **The 1-hp boundary.** The eye at 1 of its hit points. The original borg rounds the percentage down (9%) and then rounds again, tracking power 0 - the eye reads as having no health worth attacking, and the attack-damage cap (twice the tracked power) drops to 0. The corrected borg tracks power 1 and a cap of 2.

- **Two hp, just above the edge.** The eye at 2 hp. The original borg tracks power 1 where the eye has 2; the corrected borg tracks 2 (cap 4).

- **Exactly 0 hp while alive (control).** The engine kills a monster only below 0 hp, so a monster standing at exactly 0 hp is alive and is tracked as 0 in both variants - the faithful reading. The complete log is byte-identical.

## Conditions

Native Human Warrior, level 20 (experience forced), depth 5 in a synthetic open lit arena, a wooden torch as the only light, infravision 0. The floating eye is the stock race (maxhp rolled per instance) one grid east, awake and visible; its true hp is set by the mode before the first observation. Two production borg_update() passes run: the first creates the kill record (new-record path), the second refreshes it (existing-record path). No combat, movement or random placement consumes the RNG (fixed seed 662); a scenario log is fully determined by its mode. Limits: the fixture measures the tracked power on both tracking paths; the attack score, danger and the walk-closer decision that consume power are identified in the source and their inputs change with it, but no individual attack is replayed here (the recorded paralysis death is the Rust port's motivating case, quoted on the issue).

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario to see the original and corrected runs side by side on the dungeon map, drawn with the game's own symbols (the legend under the maps explains them), press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/killpower-662
git fetch https://github.com/diegoami/neo-angband-tracker.git task/killpower-662-evidence
git show a41612c45ab405962cad449984b962c6342ee196:docs/audit/c-reports/killpower-662/reproduce.py > /tmp/reproduce-killpower-662.py
python3 /tmp/reproduce-killpower-662.py --full-tests --fixtures-ref a41612c45ab405962cad449984b962c6342ee196 --fix-ref f87b535af3f35649704641dee887a0f161bb34b7
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-kill.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix `f87b535af3f35649704641dee887a0f161bb34b7` (branch `task/killpower-662`), fixtures
  `a41612c45ab405962cad449984b962c6342ee196` (branch `task/killpower-662-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: full, zero_alive
- full native gates: before 966/966 unit checks and 5/5 frontend tests;
  after 966/966 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

On both monster-tracking paths the borg's tracked power for a living monster no longer matches its true hp exactly when a small remainder is lost twice: at 1 hp the original tracks 0 (the attack-damage cap collapses to 0 - the eye reads as having no health worth attacking) and at 2 hp it tracks 1. With the correction the tracked power equals the true hp (1 and 2, attack caps 2 and 4), and the full-health and exactly-0-hp-alive controls are byte-identical - exactly 0 hp on a living monster stays 0, the faithful reading of the engine's hp < 0 death test. Both full native gates pass on both variants.

The fixture measures the tracked power on both tracking paths (new record and refreshed record). The attack score, danger math and the walk-closer decision that consume power are identified in the source and their inputs change with the correction, but no individual attack decision is replayed here; the recorded paralysis death is the Rust port's motivating case, quoted on the issue. The stock floating eye's maxhp is rolled per instance (12 in the fixture); the rounding loses the same remainders for any maxhp, including the recorded 1-of-9 case (11% then 0).

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
