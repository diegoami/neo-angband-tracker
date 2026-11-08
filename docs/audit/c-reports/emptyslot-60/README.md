# A Borg with an empty pack thought its pack was full and would not buy food

This is **emptyslot-60 / issue #60**, a C Borg defect
(diegoami/neo-angband-tracker#60).

Before buying anything, or taking anything out of its home, the Borg asks for the first empty slot in its pack. That helper looked for the last used slot and returned the one after it; when the pack held nothing at all it found no used slot and answered 'no room'. A level-1 Warrior started without the starting kit, ate its only ration and stood in town starving with 591 gold and five Rations of Food in the General Store; the Borg's shop choice found 'no room' and chose no shop at all. The correction returns slot 0 for an empty pack; the Borg then sets out to buy the rations.

## What to watch

Sets out to buy food (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `adcb53459`) |
| --- | --- | --- |
| Empty pack, starving, rations on sale | failed, buys_food=0 | passed, buys_food=1 |
| Control: one Flask of Oil in the pack | passed, buys_food=1 | byte-identical log |

- **Empty pack, starving, rations on sale.** Original: the first-empty-slot helper returns -1 for the empty pack, so the immediate food purchase and the home check both give up and borg_choose_shop() returns false with no goal. Corrected: the helper returns slot 0 and the Borg chooses the General Store, ware 0 (Rations of Food).

- **Control: one Flask of Oil in the pack.** With one used slot both versions return slot 1 as the first empty slot and choose to buy the rations; the logs are identical.

## Conditions

Native Human Warrior, seed 60, character level 1, birth option 'Start with a kit of useful gear' off, so the game gives one Ration of Food (pack) and one Wooden Torch (wielded) and 591 gold. The ration is eaten with the real eat command (the fixture first sets the food counter to Weak so the command is accepted). In town (depth 0). Every store is emptied, then the General Store is given five Rations of Food (fixture stock). 'empty': nothing else, so the pack is empty. 'oil': the fixture puts one Flask of Oil in the pack, so exactly one slot is used. The Borg's items and stores are loaded with the production borg_find_all_items(); then the production borg_choose_shop() (the town decision that buys food immediately when the Borg has none) runs once and its goal is read back. This isolates the shop choice; it does not walk to the store or carry out the purchase.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/emptyslot-60
git fetch https://github.com/diegoami/neo-angband-tracker.git task/emptyslot-60-evidence
git show e550240591a3ba6c99ecf09ba8268667971ba52d:docs/audit/c-reports/emptyslot-60/reproduce.py > /tmp/reproduce-emptyslot-60.py
python3 /tmp/reproduce-emptyslot-60.py --full-tests --fixtures-ref e550240591a3ba6c99ecf09ba8268667971ba52d --fix-ref adcb53459ca9fabaa0c457231cd56dbd8b789698
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-inventory.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `adcb53459ca9fabaa0c457231cd56dbd8b789698` (branch `task/emptyslot-60`), fixtures
  `e550240591a3ba6c99ecf09ba8268667971ba52d` (branch `task/emptyslot-60-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: oil
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_first_empty_inventory_slot() returned -1 ('no room') for an empty pack. A starving level-1 Warrior with an empty pack, 591 gold and rations on sale chose no shop; with the correction it chooses the General Store rations. With one item in the pack both versions return slot 1 and buy the rations, byte-identically.

Not measured: the walk to the store, the purchase itself, or what the original Borg does next in a long run (it may starve). How often the pack is completely empty in play is not measured: it needs every carried item used, sold or worn, which is most likely just after a start without the starting kit, as here. The same helper is used by the home purchase, the home swap-weapon/armour purchases, wearing/removal code and borg_inventory_full(); only the shop choice was exercised. The fixture empties the stores, stocks five rations, sets the food counter before the real eat command, and (in 'oil') adds the flask.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
