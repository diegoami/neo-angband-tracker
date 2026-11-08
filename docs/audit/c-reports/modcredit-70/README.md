# Credit stat and speed bonuses in the Borg's item value guess

This is **modcredit-70 / issue #70**, a C Borg defect
(diegoami/neo-angband-tracker#70).

Behaviour change; owner decision (option A). The Borg guesses what each known item would fetch in a shop and uses that guess to decide what to sell first, what to junk and what is worth carrying. For wearables the guess is meant to add credit for stat bonuses (200 per point), speed (30000 per point), stealth, blows, shots and the other bonuses, but the whole credit block was skipped whenever the item's pval was zero, and in 4.2 that is every ring, amulet, armour and weapon (bonuses are modifiers, pval is set only for launchers, a flask of oil and a mushroom). The stat lines also tested stat numbers as object flags. So a Ring of Strength <+2> was guessed at its kind cost of 500, the same as a Ring of Strength <+1>, and the Cloak 'Colannon' (speed +2) at 21500, below an Amulet of ESP. When both items are spare, the Borg sold the bonus item first. The change removes the pval gate and credits the five stat modifiers directly (a negative stat modifier earns no credit). The guessed value of every wearable with modifiers changes; whole-game effects are not measured.

## What to watch

The item without the bonus sold first (1 = sold the plain item and kept the bonus item; 0 = sold the bonus item); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `46c6188ec`) |
| --- | --- | --- |
| Stat ring: Ring of Strength <+2> vs Ring of Soulkeeping | failed, sold_b=0 | passed, sold_b=1 |
| Speed item: Cloak 'Colannon' <+2> vs Amulet of ESP | failed, sold_b=0 | passed, sold_b=1 |
| Control: two rings without modifiers (Free Action vs Bodykeeping) | passed, sold_b=1 | byte-identical log |

- **Stat ring: Ring of Strength <+2> vs Ring of Soulkeeping.** Original: the Ring of Strength <+2> is guessed at 500 (no stat credit), below the Ring of Soulkeeping at 600, so the Borg sells the Ring of Strength. Corrected: 500 + 2 x 200 = 900, so it sells the Ring of Soulkeeping and keeps the Ring of Strength. The game's own values are 1116 (Strength <+2>) and 1050 (Soulkeeping).

- **Speed item: Cloak 'Colannon' <+2> vs Amulet of ESP.** Original: Colannon is guessed at 21500 (artifact cost 20000 + armour bonus 1500, no speed credit), below the Amulet of ESP at 25000, so the Borg sells Colannon. Corrected: 21500 + 2 x 30000 (speed) + 2 x 100 (stealth) = 81700, capped at 30000 for the sale order, so it sells the Amulet of ESP and keeps Colannon. The game's own values are 11550 (Colannon) and 7134 (ESP).

- **Control: two rings without modifiers (Free Action vs Bodykeeping).** Neither ring has a modifier, so the corrected code adds nothing: Free Action 1500, Bodykeeping 750 in both versions, and both sell the Ring of Bodykeeping. Logs byte-identical.

## Conditions

Native Human Warrior, seed 70, clevel 21, max depth 20, in town, 1000 gold, the starting equipment worn and the pack emptied except for two known items, all runes learned. 'statring': a Ring of Strength <+2> and a Ring of Soulkeeping. 'speed': the Cloak 'Colannon' [1,+15] <+2> (speed +2, stealth +2; the standard artifact made with copy_artifact_data()) and an Amulet of ESP. 'nomods': a Ring of Free Action and a Ring of Bodykeeping, neither with any modifier. Rings and amulets are made with object_prep() at level 40 with average rolls. Every store is emptied so no shop is full. The production borg_think_shop_sell_useless() chooses the sale (the cheaper item by the Borg's guess, capped at 30000, among items whose sale does not lower the Borg's power); the chosen item is then sold with the real do_cmd_sell() while standing in the chosen shop. The game's own valuation (object_value_real()) is printed for reference. Fixture interventions: the pack is emptied and the items are added directly, all runes are learned with player_learn_all_runes(), the stores are emptied directly, dynamic formulas, gold worship and money scumming are off, HP is set to maximum, and the player's grid is turned into the chosen shop's entrance before the sale.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/modcredit-70
git fetch https://github.com/diegoami/neo-angband-tracker.git task/modcredit-70-evidence
git show 42669672433cd274307be9716bec8926f7900394:docs/audit/c-reports/modcredit-70/reproduce.py > /tmp/reproduce-modcredit-70.py
python3 /tmp/reproduce-modcredit-70.py --full-tests --fixtures-ref 42669672433cd274307be9716bec8926f7900394 --fix-ref 46c6188eccb54bd0571e0a527145f7fd7ed756c3
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-item-analyze.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `46c6188eccb54bd0571e0a527145f7fd7ed756c3` (branch `task/modcredit-70`), fixtures
  `42669672433cd274307be9716bec8926f7900394` (branch `task/modcredit-70-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: nomods
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With the pval gate removed and the stat credits read from the modifiers, borg_object_value_known() credits a Ring of Strength <+2> (500 to 900) and the Cloak 'Colannon' with speed +2 (21500 to 81700). In two native sale scenarios the production borg_think_shop_sell_useless() then sells the plain item (Ring of Soulkeeping, Amulet of ESP) instead of the bonus item, and the real do_cmd_sell() carries the sale out; a control with two rings without modifiers is byte-identical; both full native gates pass on both builds.

Every wearable's guessed value changes, not only these items: all modifier credits (stats, stealth, searching, infravision, tunnelling, blows, shots, speed, light, might, moves, damage reduction) now apply to rings, amulets, armour, weapons and lights, so sale order, junk and drop choices change widely. Negative modifiers other than stats now lower the guess (as they already did for items with a pval): from the source arithmetic, an Amulet of Inertia (speed -1 or -2) goes from 10 to below zero, so the Borg treats it as worthless (it no longer tries to sell it and may junk it); the Ring of the Dog stays at 0 through its existing special case. Not measured: whole-game runs, overall survival or gold, and the decisions other than the two sale orders shown. Speed items whose base value is already 30000 or more (Ring of Speed, boots of Speed) keep the same sale order because the sale code caps the value at 30000. The cursed Ring of Escaping is guessed at 0 in both versions. The game's own valuation is printed for reference only; the Borg's guess is a separate heuristic.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
