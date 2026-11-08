# The Borg gave away an unidentified Cloak of Aman

This is **egosell-133 / issue #133**, a C Borg defect
(diegoami/neo-angband-tracker#133).

In town the Borg looks for pack items it can sell without losing anything. It is meant to keep an ego item whose random extra power (for example the hidden high resist on a Cloak of Aman) it has not learned yet, unless it carries a stack of them. That check looked up the ego of the cloak the Borg was wearing instead of the item being sold. A level-21 Warrior wearing a plain Cloak and carrying a Cloak of Aman whose random resist (nether) was still unknown therefore chose to sell the Cloak of Aman to the Black Market; carrying out that sale with the real store command handed the cloak over and only then revealed its resist. The correction asks about the item being sold; the Borg then keeps the cloak.

## What to watch

Sold the unidentified Cloak of Aman (1 = sold); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `8b5ab27a8`) |
| --- | --- | --- |
| Plain Cloak worn, unidentified Cloak of Aman carried | failed, sold_unidentified_aman=1 | passed, sold_unidentified_aman=0 |
| Control: Cloak of Aman worn, unidentified Cloak of Aman carried | passed, sold_unidentified_aman=0 | byte-identical log |

- **Plain Cloak worn, unidentified Cloak of Aman carried.** Original: the random-power check asks about the worn plain Cloak (no ego, no random power), so the Cloak of Aman counts as safe to sell; the Borg picks it for the Black Market (shop 6, item a) and the real sale hands it over ('You had a Cloak of Aman [1,+14] <+1> (a).'), learning the nether rune only as it goes. Corrected: the check asks about the Cloak of Aman itself and the Borg sells nothing.

- **Control: Cloak of Aman worn, unidentified Cloak of Aman carried.** The worn cloak and the carried one have the same random-power ego, so both versions give the same answer: nothing is sold. The changed check runs in both versions; the logs are byte-identical.

## Conditions

Native Human Warrior, seed 133, character level 21, max depth 20, in town standing on the Black Market entrance, birth_no_selling at its default (on), BORG_WORSHIPS_GOLD and BORG_MONEY_SCUM_AMOUNT off, static preparedness. The starting pack is emptied (equipment kept) and every store is emptied. The carried spare is a Cloak of Aman created first from the same RNG state in both modes; every rune is known except the spare's random high resist (nether), so its ego name is shown but it is not identified ({??}). 'plain-worn': a plain Cloak is worn. 'aman-worn': a Cloak of Aman is worn instead. The production borg_find_all_items(), borg_notice() and borg_power() load the state; the production borg_think_shop_sell_useless() (step 2 of the store logic) runs once and its goal is read back; if it chose the cloak, the real do_cmd_sell() runs on it. This isolates the sell choice and the sale; it does not play the walk through town or the Borg's other store steps.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/egosell-133
git fetch https://github.com/diegoami/neo-angband-tracker.git task/egosell-133-evidence
git show 0fed2a2d690c398f0de454637987b19f75865a1a:docs/audit/c-reports/egosell-133/reproduce.py > /tmp/reproduce-egosell-133.py
python3 /tmp/reproduce-egosell-133.py --full-tests --fixtures-ref 0fed2a2d690c398f0de454637987b19f75865a1a --fix-ref 8b5ab27a81e46d95a87f4aca100cf2beca213171
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-sell.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `8b5ab27a81e46d95a87f4aca100cf2beca213171` (branch `task/egosell-133`), fixtures
  `0fed2a2d690c398f0de454637987b19f75865a1a` (branch `task/egosell-133-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: aman-worn
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_good_sell() asked borg_ego_has_random_power() about the worn cloak's ego (borg_items[INVEN_OUTER]) instead of the item being sold. A level-21 Warrior wearing a plain Cloak sold an unidentified Cloak of Aman (hidden resist nether) to the Black Market; corrected, it keeps the cloak. With a Cloak of Aman worn both versions keep it and the logs are byte-identical.

Measured: the sell choice and the real sale of one cloak. Not measured: how often the Borg carries an unidentified random-power ego cloak or light next to a same-type duplicate (the route used here: a non-stacked item that still needs identifying is only offered for sale when another item of the same kind is carried or worn, and weapons and body armour never count as such duplicates, while cloaks and lights do), whether its other town steps (wearing to identify, home storage) would have dealt with the cloak first, and any later combat or survival effect of the lost resist. Under birth_no_selling (the default) the sale earns no gold; with selling on it would earn a price for an item whose value was not known. The opposite direction (wearing a random-power ego cloak makes the original keep an unidentified ego item without a random power) was not reproduced: an ego is only named once its fixed runes are known, so such an item usually no longer needs identifying.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
