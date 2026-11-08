# Four apples made Resist Poison potions worthless

This is **respois-184 / issue184**, an item-valuation defect in the C Borg.

The Borg judges its pack with a "power" score, and uses that score to decide
what to keep, sell, buy or drop. Carrying up to four Resist Poison potions
should add 300 each ("collect Resistance pots if not immune -- All
Classes" in `borg_power_inventory()`). That loop does not reset its counter
`k`, so it starts wherever the previous reward loop left it — the
low-calorie food count (capped at 15), or the Cure Serious Wounds count
when the Borg is cut or poisoned. With four or more apples, slime molds or
similar in the pack, Resist Poison potions are worth nothing.

The correction resets the counter, as the neighbouring reward loops do:

```c
k = 0;
for (; k < 4 && k < borg.trait[BI_ARESPOIS]; k++)
    value += 300L;
```

## What to watch

| Scenario | Original | Corrected |
| --- | --- | --- |
| Carrying 4 Apples | 3 Resist Poison potions add **0** power | add **900** |
| Control: no low-calorie food | 900 | byte-identical log |

Fixture: Native Human Mage, seed 184, level 25, depth 20, a lantern, 5
Rations of Food, 3 Resist Poison potions (and 4 Apples in the main case).
The potions' value is the production `borg_power()` with the stack minus
without it. The production junk pass (`borg_drop_junk`) then runs once; in
both versions it drops the apples (valued at 2), not the potions.

[replay.html](replay.html): choose a scenario, then **Show decision** and
**Show raw logs**.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/respois-184
git fetch https://github.com/diegoami/neo-angband-tracker.git task/respois-184-evidence
git show FETCH_HEAD:docs/audit/c-reports/respois-184/reproduce.py > /tmp/reproduce-respois-184.py
python3 /tmp/reproduce-respois-184.py --full-tests --fix-ref 5e8d4c5e6c1006ce4c90e21a973b0f95bb9fa0db
```

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `5e8d4c5e6` (branch
  `task/respois-184`), fixtures `af01d73cc` (branch
  `task/respois-184-evidence`); the native run executed at the fixtures
  pin, the later commit only adds these captured artifacts
- before: apples 0/1 (`potion_reward=0`); control 1/1 (900)
- after: apples 1/1 (900); control byte-identical
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Playwright Chromium headless, no console errors

## Scope and limits

The fixture measures the valuation only. The score feeds selling, buying,
junk and full-pack drop decisions; none of those was exercised to show a
potion actually sold, not bought or dropped, and in the recorded junk pass
the apples go first. Treat the practical effect as unmeasured; suggested
priority low. The warrior-only Resist Poison loop has a separate counter
slip (issue185), corrected separately.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`
(+ `profile/borg.txt`) are the generators. `before-*.log`, `after-*.log`,
`*-results.json`, `result.json`, `run-*.log`, `replay.html`,
`replay-data.json` and `receipt.json` (hashes of every file) are the
captured run.
