# Only Rogues save up for the Black Market Rod of Teleport Other

This is **tporod-17 / issue #17**, a C Borg defect
(diegoami/neo-angband-tracker#17).

The Borg has a Black Market exception for a few items it should save up for. For the Rod of Teleport Other the comment says Mages and Rogues are excluded because they can cast Teleport Other, but the code reads 'not a Mage and IS a Rogue', so the exception applies to Rogues only. A level-30 Rogue who knows Teleport Other sets a 15000 gold target and stays in town resting for gold; a level-30 Warrior, who cannot cast it, does not. After the correction ('not a Mage and not a Rogue') the Warrior saves up and the Rogue does not. On its own the Borg still gives the rod no value (issue #560), so a level-12 Warrior with 16000 gold accepts the rod but does not buy it; the stacked run in stacked/ applies #560's correction and shows that Warrior buying the rod only with this correction. A Mage who knows the spell behaves exactly as before.

## What to watch

Sets a gold target to save up for the rod (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `d752bad27`) |
| --- | --- | --- |
| Warrior, level 30, 10000 gold | failed, saves_for_rod=0 | passed, saves_for_rod=1 |
| Rogue who knows Teleport Other, level 30, 10000 gold | failed, saves_for_rod=1 | passed, saves_for_rod=0 |
| Warrior, level 12, 16000 gold (needs #560) | failed, saves_for_rod=0 | failed, saves_for_rod=1 |
| Control: Mage who knows Teleport Other, level 30, 10000 gold | passed, saves_for_rod=0 | byte-identical log |

- **Warrior, level 30, 10000 gold.** Original: the Warrior is not on the exception, and the general Black Market rule rejects the rod (level under 35 needs 15000 gold); no gold target, no resting (0/1). Corrected: the exception applies; the Borg records a 15000 gold target and borg_money_scum() starts resting in town (first key 57344) (1/1). It cannot buy yet: 10000 gold is short of the price.

- **Rogue who knows Teleport Other, level 30, 10000 gold.** Original: the exception applies to the Rogue only; a 15000 gold target is recorded and the Rogue rests in town for a rod it does not need (0/1). Corrected: no gold target, no resting (1/1).

- **Warrior, level 12, 16000 gold (needs #560).** Original: the general Black Market rule wants 20000 gold below level 15; the rod is rejected. Corrected: the exception accepts the rod and records the 15000 target, but the shop step still does not buy it because the rod adds no power without #560 (buy_useful=0). Both fail the buy contract here; with #560's correction applied (stacked/), the corrected Borg buys the rod.

- **Control: Mage who knows Teleport Other, level 30, 10000 gold.** The changed line is evaluated for the Mage and excludes it in both versions; the general rule rejects the rod; no target, no resting. The logs are byte-identical.

## Conditions

Native Human, seed 17, starting kit off, character level by real experience gain (30, or 12 in warrior-buy), base Dexterity set to 18/120 so the Borg's anti-theft condition for saving up (adj_dex_safe + level > 90) holds for the level-30 characters, in town (depth 0), a worn lit Wooden Torch and 120 Rations of Food (the saving rule wants at least 100 food). Gold is 10000 (16000 in warrior-buy). The Rogue carries Arcane Control and the Mage Magical Defences, each with its spells studied through the game's spell_learn(); both books hold Teleport Other. Every store's stock is emptied and the Black Market is given one Rod of Teleport Other through store_carry() (price 15000; no normal store stocks this rod). The Borg reads inventory and stores with borg_find_all_items(); the production shop step borg_think_shop_buy_useful() runs, then the production town step borg_money_scum(), and the first queued key is recorded (57344 is the Borg's rest key). Only the decisions are recorded; the fixture does not walk to the shop, buy, or rest for gold. The default profile has borg_self_scum on.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tporod-17
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tporod-17-evidence
git show 92ca00fdc37014073ef3e65ef5f193e6124f39f9:docs/audit/c-reports/tporod-17/reproduce.py > /tmp/reproduce-tporod-17.py
python3 /tmp/reproduce-tporod-17.py --full-tests --fixtures-ref 92ca00fdc37014073ef3e65ef5f193e6124f39f9 --fix-ref d752bad27185ee7a709459b0e83186d9b0d16d7f
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-buy.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `d752bad27185ee7a709459b0e83186d9b0d16d7f` (branch `task/tporod-17`), fixtures
  `92ca00fdc37014073ef3e65ef5f193e6124f39f9` (branch `task/tporod-17-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: mage
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_good_buy() (borg-store-buy.c:127-133 at e473720ed) applied the Black Market Rod of Teleport Other exception with != CLASS_MAGE && == CLASS_ROGUE, so only Rogues, who can cast Teleport Other, saved up for it. The correction uses != for both classes, as the Recall and Speed rod exceptions beside it do. Measured: a level-30 Warrior now sets the 15000 gold target and a Rogue no longer does; the Mage control is byte-identical. The purchase depends on #560 (the rod's value); stacked/ shows a level-12 Warrior with 16000 gold buying it with #560 and this correction, and not with #560 alone.

Measured: the production shop and money-scum decisions for one Black Market offer. Not measured: the walk, the purchase and the resting itself, how long saving takes, how often the Black Market offers the rod, and fights or survival. Without #560 a Warrior that saves up for the rod still would not buy it (warrior-buy, and the #17 triage probe): this correction is meant to land with or after #560, and a bought rod is zapped only with #549's fix (#551). The fixture raises base Dexterity to 18/120 to meet the saving condition. The hunk is at L131 of borg-store-buy.c, apart from the #544/#553/#558 changes in the same file (L249-281, ~L516 and ~L638). No memory-safety aspect, so no sanitizer variant.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.

## Stacked run with #560 (stacked/)

The purchase needs both corrections. [stacked/](stacked/README.md) runs the same
fixture with #560's valuation correction applied in both phases: with #560 alone
the level-12 Warrior with 16000 gold buys nothing; with #560 and this correction
it chooses to buy the rod (`buy_useful=1 goal_shop=6 goal_ware=0`); the Rogue
stops saving up for it and the Mage control is byte-identical. Its own pinned
commands, logs, replay and receipt are in that directory.
