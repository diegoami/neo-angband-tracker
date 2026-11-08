# Resist Cold potions cancelled a warrior's Resist Poison reward

This is **respoiswar-185 / issue185**, an item-valuation defect in the C
Borg.

Warriors past depth 20 get an extra "power" reward for carrying up to four
potions each of Resist Heat, Resist Cold and Resist Poison ("Reward
Resistance Potions for Warriors" in `borg_power_inventory()`). The counter
`k` is reset before the heat and cold loops but not before the poison loop,
which therefore starts at the Resist Cold count (capped at 4). A warrior
with four Resist Cold potions gets no warrior reward for its Resist Poison
potions.

The correction resets the counter before the poison loop too (`k = 0;`).

## What to watch

| Scenario | Original | Corrected |
| --- | --- | --- |
| Also carrying 4 Resist Cold potions | 3 Resist Poison potions add **900** | add **2400** |
| Control: no Resist Cold potions | 2400 | byte-identical log |

(2400 = 3 × 300 from the all-classes reward + 3 × 500 from the warrior
reward.)

Fixture: Native Human Warrior, seed 185, level 25, maximum depth 25 (on
depth 20), a lantern, 5 Rations of Food, 3 Resist Poison potions (and 4
Resist Cold potions in the main case). The potions' value is the production
`borg_power()` with the stack minus without it; the production junk pass
then runs once and drops nothing in either version.

[replay.html](replay.html): choose a scenario, then **Show decision** and
**Show raw logs**.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/respoiswar-185
git fetch https://github.com/diegoami/neo-angband-tracker.git task/respoiswar-185-evidence
git show FETCH_HEAD:docs/audit/c-reports/respoiswar-185/reproduce.py > /tmp/reproduce-respoiswar-185.py
python3 /tmp/reproduce-respoiswar-185.py --full-tests --fix-ref c056daabf55e6a1713e68381ef4a72ec6fd349b9
```

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `c056daabf` (branch
  `task/respoiswar-185`), fixtures `6f130a4ee` (branch
  `task/respoiswar-185-evidence`); the native run executed at the fixtures
  pin, the later commit only adds these captured artifacts
- before: cold 0/1 (`potion_reward=900`); control 1/1 (2400)
- after: cold 1/1 (2400); control byte-identical
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Playwright Chromium headless, no console errors

## Scope and limits

The fixture measures the valuation only; selling, buying and full-pack drop
decisions were not exercised, so the practical effect is unmeasured
(suggested priority low). The all-classes Resist Poison loop has a
separate counter slip (issue184), corrected separately.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`
(+ `profile/borg.txt`) are the generators. `before-*.log`, `after-*.log`,
`*-results.json`, `result.json`, `run-*.log`, `replay.html`,
`replay-data.json` and `receipt.json` (hashes of every file) are the
captured run.
