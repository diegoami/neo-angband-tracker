# A poisoned or bleeding Borg in town only looked in the Bookseller and bought no cure

This is **urgentshop-23 / issue #23**, a C Borg defect
(diegoami/neo-angband-tracker#23).

When the Borg is poisoned or bleeding in town it is meant to go straight to the shop that sells cures. The code still uses store number 3 for that, which was the Temple in older Angband; in 4.2 store 3 is the Bookseller and the cure potions are in the Alchemy shop (store 4). While poisoned or bleeding, every purchase the Borg weighs is limited to that one store. A level-1 Warrior in town with 500 gold and Potions of Cure Light Wounds on sale in the Alchemy shop buys them when healthy, but when poisoned or bleeding it chooses no shop at all. With the correction it sets out to buy the potions in both cases.

## What to watch

Sets out to buy Cure Light Wounds (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `09c28cd59`) |
| --- | --- | --- |
| Poisoned, Cure Light Wounds on sale | failed, buys_cure=0 | passed, buys_cure=1 |
| Bleeding, Cure Light Wounds on sale | failed, buys_cure=0 | passed, buys_cure=1 |
| Control: healthy, same stock | passed, buys_cure=1 | byte-identical log |

- **Poisoned, Cure Light Wounds on sale.** Original: the immediate-purchase step and step 3 only look at store 3 (the Bookseller), where nothing helps a Warrior, so borg_choose_shop() returns false with no goal. Corrected: they look at store 4 and the Borg chooses the Alchemy shop, ware 0 (Potions of Cure Light Wounds).

- **Bleeding, Cure Light Wounds on sale.** Same as poisoned: the original chooses no shop; the corrected Borg chooses the Alchemy shop's Cure Light Wounds, which reduce cuts.

- **Control: healthy, same stock.** Not poisoned or bleeding, the store filter does not apply and both versions weigh every store and choose the Alchemy shop's Cure Light Wounds in step 3; the logs are identical.

## Conditions

Native Human Warrior, seed 23, character level 1, standard starting kit (two Rations of Food, a wielded Wooden Torch, armour and weapon). The fixture removes the kit's Scroll of Word of Recall and Potion of Berserk Strength (otherwise the Borg first takes the scroll home or sells the potion, which ends the shop choice before any purchase is weighed) and sets the gold to 500. In town (depth 0). Every store is emptied, then the Alchemy shop is given five Potions of Cure Light Wounds and the Bookseller one [First Spells] (fixture stock, identical in every mode). 'poisoned' and 'bleeding' apply 10 turns of poison or a cut with the game's player_inc_timed(); 'healthy' applies nothing. The Borg's items and stores are loaded with the production borg_find_all_items(); then the production borg_choose_shop() runs once and its goal is read back. This isolates the shop choice; it does not walk to the store or carry out the purchase.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/urgentshop-23
git fetch https://github.com/diegoami/neo-angband-tracker.git task/urgentshop-23-evidence
git show dbd9ddf1bddabb4c2b3d35099f9d0efd065ae2a3:docs/audit/c-reports/urgentshop-23/reproduce.py > /tmp/reproduce-urgentshop-23.py
python3 /tmp/reproduce-urgentshop-23.py --full-tests --fixtures-ref dbd9ddf1bddabb4c2b3d35099f9d0efd065ae2a3 --fix-ref 09c28cd59c948fa03fb49349d30e7236b6d067ed
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-buy.c, src/borg/borg-think-store.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `09c28cd59c948fa03fb49349d30e7236b6d067ed` (branch `task/urgentshop-23`), fixtures
  `dbd9ddf1bddabb4c2b3d35099f9d0efd065ae2a3` (branch `task/urgentshop-23-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: healthy
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The poison/cut shop filter in borg_think_shop_buy_useful() and the forced shop in borg_choose_shop() used store index 3, the Bookseller in 4.2 (formerly the Temple). A poisoned or bleeding level-1 Warrior with 500 gold and Cure Light Wounds on sale in the Alchemy shop chose no shop; with index 4 it chooses the potions. A healthy Borg chooses the same potions in both versions, byte-identically.

Not measured: the walk to the store, the purchase itself, or whether the original Borg later recovers (poison and cuts wear off, and it may buy once healed). Cure Light Wounds heals hit points and reduces cuts but does not cure poison; the Alchemy shop also stocks Cure Serious Wounds and Neutralize Poison, which were not offered here. The changed forced-shop line in borg_choose_shop() is overwritten whenever a purchase is chosen and is only kept on a stale-ware path the issue also describes; that path was not exercised and is not claimed. The fixture removes two kit items, sets the gold, stocks the stores and applies the timed effect.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
