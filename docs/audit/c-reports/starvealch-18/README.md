# A Borg with no food in town never bought Remove Hunger at the Alchemist

This is **starvealch-18 / issue #18**, a C Borg defect
(diegoami/neo-angband-tracker#18).

When the Borg has no food left and is in town, it only looks for something to buy in the General Store (and its Home). If the General Store has no food, it buys nothing, even with gold in hand and Scrolls of Remove Hunger on sale at the Alchemist, the only shop that sells them. The code meant to allow Remove Hunger (its starving check names the scroll), but the Alchemist was never looked at, the check was miswritten so that it let any scroll through instead of only Remove Hunger, and the Borg gave a Remove Hunger scroll no value as food. A level-1 Warrior with no food, 500 gold, only Flasks of Oil in the General Store and Remove Hunger at the Alchemist chooses no shop at all. With the correction it sets out to buy Remove Hunger at the Alchemist. With rations in the General Store it still buys rations, and a Borg that carries food still buys Cure Light Wounds, exactly as before.

## What to watch

Sets out to buy Remove Hunger (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `0fd9bc4bd`) |
| --- | --- | --- |
| No food carried, General Store has only oil | failed, buys_remove_hunger=0 | passed, buys_remove_hunger=1 |
| Control: no food carried, General Store has rations | passed, buys_remove_hunger=0 | byte-identical log |
| Control: carrying rations, not hungry | passed, buys_remove_hunger=0 | byte-identical log |

- **No food carried, General Store has only oil.** Original: with no food only the General Store and Home are weighed; the oil fails the starving check, so borg_choose_shop() returns false with no goal. Corrected: the Alchemist is weighed too, Remove Hunger passes the starving check and now counts as food, and the Borg chooses the Alchemist's Scrolls of Remove Hunger (store 4, ware 1).

- **Control: no food carried, General Store has rations.** Both versions choose the General Store's Rations of Food. The corrected Borg also weighs Remove Hunger at the Alchemist, but rations score higher; the logs are identical.

- **Control: carrying rations, not hungry.** With food carried, the store filter and starving check do not apply and Remove Hunger is not counted as food; both versions choose the Alchemist's Cure Light Wounds and the logs are identical.

## Conditions

Native Human Warrior, seed 18, character level 1, standard starting kit (wielded Wooden Torch, armour and weapon). The fixture removes the kit's Scroll of Word of Recall and Potion of Berserk Strength (otherwise the Borg first takes the scroll home and sells the potion, which ends the shop choice before any purchase is weighed) and sets the gold to 500. In the two no-food modes the kit's Rations of Food are removed as well and the character is made Hungry with the game's player_set_timed(TMD_FOOD, PY_FOOD_HUNGRY - 1); in 'fed' the three rations stay and nutrition is untouched. In town (depth 0). Every store is emptied, then the Alchemist (store 4) gets five Scrolls of Remove Hunger and five Potions of Cure Light Wounds and the General Store (store 0) five Flasks of Oil, plus five Rations of Food in 'general_food' only (fixture stock; emptying the General Store's staple rations is an intervention that normal rules do not produce, see the limits). The Borg's items and stores are loaded with the production borg_find_all_items(); then the production borg_choose_shop() runs once and its goal is read back. This isolates the shop choice; it does not walk to the store or carry out the purchase.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/starvealch-18
git fetch https://github.com/diegoami/neo-angband-tracker.git task/starvealch-18-evidence
git show b8132a88b5f40049026f3d79d15e73ed7b86b48a:docs/audit/c-reports/starvealch-18/reproduce.py > /tmp/reproduce-starvealch-18.py
python3 /tmp/reproduce-starvealch-18.py --full-tests --fixtures-ref b8132a88b5f40049026f3d79d15e73ed7b86b48a --fix-ref 0fd9bc4bd6f069e0cbab664e7541e3202764a9a4
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-buy.c, src/borg/borg-trait.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `0fd9bc4bd6f069e0cbab664e7541e3202764a9a4` (branch `task/starvealch-18`), fixtures
  `b8132a88b5f40049026f3d79d15e73ed7b86b48a` (branch `task/starvealch-18-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: general_food, fed
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With no food, borg_think_shop_buy_useful() weighed only the General Store and Home, its starving check let any scroll through instead of only Remove Hunger, and Remove Hunger scrolls were never counted as food. A Hungry level-1 Warrior with no food, 500 gold, only oil in the General Store and Remove Hunger at the Alchemist chose no shop; with the correction it chooses Remove Hunger at the Alchemist. With rations in the General Store, or with food carried, both versions choose the same purchase, byte-identically.

Reachability: in the stock 4.2 data the General Store always stocks Rations of Food (store.txt `always:food:Ration of Food`; store.c restocks staples to a full stack and store_sale_should_reduce_stock() does not reduce a staple when it is bought), so a General Store with no food, as built here by emptying the stores, needs modified store data; the corrected path is a safety net for that case and for game data without the staple, not a situation measured in an unmodified game. Not measured: the walk to the store, the purchase, reading the scroll, or whether the original Borg would later starve (it may find food in the dungeon or once the General Store restocks); no death or starvation is claimed. The corrected starving check (food or Remove Hunger only) is not separately observable in this fixture: the only other Alchemist ware offered, Cure Light Wounds, is blocked by both versions of the check, and scrolls such as Phase Door that the original check would let through were not stocked. Counting Remove Hunger as food only while the Borg has no other food also changes other BI_FOOD checks for a Borg whose only food is Remove Hunger scrolls (for example it no longer counts as out of food); those paths were not measured. The fixture removes kit items, sets the gold and hunger, and stocks the stores.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
