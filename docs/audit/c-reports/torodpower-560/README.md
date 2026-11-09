# The Borg gives a Rod of Teleport Other no value and never buys one

This is **torodpower-560 / issue #560**, a C Borg defect
(diegoami/neo-angband-tracker#560).

The Borg values Teleport Other highly, especially for a Warrior, who cannot cast it: its power score rises by 60000 for the first charge of a Wand of Teleport Other. But only the wand and the spell count. A Rod of Teleport Other adds nothing to the Borg's Teleport Other count, so its power score does not move for the rod and the shop step never buys one. A level-30 Warrior with 20000 gold standing in front of a Black Market that offers the rod for 15000 leaves it there. After the correction each carried rod counts like one wand charge: the same Warrior sees its power rise by 60000 and the production shop step chooses to buy the rod. A Warrior who already carries Wands of Teleport Other with 16 charges, and a Mage who knows the Teleport Other spell, gain nothing from the rod and behave exactly as before.

## What to watch

Chooses to buy the Black Market Rod of Teleport Other (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `fc444ab17`) |
| --- | --- | --- |
| Warrior with 20000 gold, no Teleport Other | failed, buys_rod=0 | passed, buys_rod=1 |
| Control: Warrior already carrying Wands of Teleport Other (16 charges) | passed, buys_rod=0 | byte-identical log |
| Control: Mage who knows Teleport Other | passed, buys_rod=0 | byte-identical log |

- **Warrior with 20000 gold, no Teleport Other.** Original: Teleport Other count 0 with the rod too, power 805500 with and without it, so borg_think_shop_buy_useful() chooses nothing (buy_useful=0). Corrected: the rod counts as 1, power rises to 865500 (+60000: 5000 for having Teleport Other, 50000 more for a Warrior, 5000 for the charge), and the shop step chooses Black Market ware 0, the rod (buy_useful=1 goal_shop=6 goal_ware=0).

- **Control: Warrior already carrying Wands of Teleport Other (16 charges).** The Warrior's count is already 16 from the wand charges. The corrected code counts the rod as one more (the changed line runs while the shop step evaluates the rod), but the reward per charge stops at 15, so power stays 935500 and neither version buys the rod. The logs are byte-identical.

- **Control: Mage who knows Teleport Other.** The Mage's count is already 1000 from the spell. The rod adds one in the corrected code (the changed line runs), power stays 1516641 and neither version buys the rod. The logs are byte-identical.

## Conditions

Native Human, seed 560, character level 30 by real experience gain, 20000 gold, in town (depth 0). The starting kit is off; the character carries a lit Wooden Torch (worn) and 5 Rations of Food, plus per case: nothing else (Warrior), two Wands of Teleport Other with 16 charges in all (Warrior, wand control; object.txt gives 6+d5 charges per wand), or Magical Defences with its spells studied through the game's spell_learn() (Mage control). Every store's stock is emptied and the Black Market is given one Rod of Teleport Other through store_carry() (price 15000); no normal store stocks this rod (store.txt). The Borg reads inventory and stores with borg_find_all_items(), then the production shop step borg_think_shop_buy_useful() runs. A TRACK line reports, as a diagnostic, the Borg's power with and without the rod in an empty pack slot (the same evaluation the shop step makes). Only the purchase decision is recorded; the fixture does not walk to the shop or carry out the buy command.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/torodpower-560
git fetch https://github.com/diegoami/neo-angband-tracker.git task/torodpower-560-evidence
git show 12a59c5c4d20ec9b66e2d151f9646a8c7937b7b6:docs/audit/c-reports/torodpower-560/reproduce.py > /tmp/reproduce-torodpower-560.py
python3 /tmp/reproduce-torodpower-560.py --full-tests --fixtures-ref 12a59c5c4d20ec9b66e2d151f9646a8c7937b7b6 --fix-ref fc444ab17e8dee8ccdbb908f4e3bb760114089b8
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-trait.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `fc444ab17e8dee8ccdbb908f4e3bb760114089b8` (branch `task/torodpower-560`), fixtures
  `12a59c5c4d20ec9b66e2d151f9646a8c7937b7b6` (branch `task/torodpower-560-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: wand, mage
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_notice_inventory() (borg-trait.c:2401-2440 at e473720ed) filled BI_ATPORTOTHER only from a Wand of Teleport Other's charges (:2454) and the Teleport Other spell (:2718-2719); the rod case was missing, so borg_power() (borg-power.c:1644-1653) never rewarded the rod and borg_think_shop_buy_useful() rejected it (p <= b_p). The correction adds each carried Rod of Teleport Other to BI_ATPORTOTHER as one, like one wand charge. A Warrior with gold now buys the rod at the Black Market; a Warrior with 16 wand charges and a Mage with the spell are byte-identical controls that run the changed line.

Measured: one production shop decision (borg_think_shop_buy_useful()) for three level-30 characters in front of a Black Market that offers the rod. Not measured: walking to the shop and the real purchase, how often the Black Market offers this rod (no normal store stocks it), and fights or survival. Whether the rod is used after buying depends on #549 (draft fix #551, which zaps the rod in the Teleport Other defence); without it a bought rod is carried but not zapped. The Black Market class gate of #17 is unchanged here: the Warrior in this fixture buys under the Black Market's general rich-Borg rule (level under 35 needs at least 15000 gold). The rod is counted whether or not it is currently charging, as other rods are; there is no device-skill check, matching the wand. BI_ATPORTOTHER is also read by borg-junk.c:633 (keeping Teleport Other wands for a Warrior with 8 or fewer charges); a carried rod now adds one to that count. No memory-safety aspect, so no sanitizer variant.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
