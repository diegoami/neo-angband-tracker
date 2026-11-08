# A full pack stopped the Borg from wearing its other planned upgrades

This is **wearskip-85 / issue85**, a C Borg defect
(diegoami/neo-angband-tracker#85).

At home the Borg plans its best set of equipment and then makes the changes one step at a time. When the next change needs a free pack slot and the pack is full, it is meant to skip that change and try the following one. The skip counter was reset on every pass of the search, so instead every remaining change was skipped and the plan was thrown away. A level-21 Warrior with exactly one free pack slot, a stack of two Tulwars, Metal Scale Mail and a Cloak in the pack switched to a Tulwar (which filled the pack) and then stopped: the armour change needed a free slot, and the cloak, which could have gone on at once and freed that slot, was never tried. The correction keeps the counter across the search; the Borg puts on the cloak, then swaps the armour.

## What to watch

Wearing the Cloak at the end (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `5d73e0c21`) |
| --- | --- | --- |
| Two stacked Tulwars, one free slot, Cloak in pack | failed, wears_cloak=0 | passed, wears_cloak=1 |
| Control: one Tulwar, two free slots | passed, wears_cloak=1 | byte-identical log |
| Control: full pack and nothing else to try | passed, wears_cloak=0 | byte-identical log |

- **Two stacked Tulwars, one free slot, Cloak in pack.** Original: the Dagger comes off and a Tulwar goes on (2 actions); the pack is now full, the armour change is skipped, the cloak is skipped too and the plan is discarded, so the Borg ends in Soft Leather Armour with no cloak. Corrected: the armour change is skipped once, the Cloak goes on (freeing a slot), then the armour is swapped for Metal Scale Mail (5 actions).

- **Control: one Tulwar, two free slots.** The pack never fills, so no change is skipped; both versions make the same five changes and the logs are identical.

- **Control: full pack and nothing else to try.** The armour change is skipped (the skip path runs in both versions) and no other change remains; both versions stop after the Tulwar and discard the plan. The logs are identical.

## Conditions

Native Human Warrior, seed 85, character level 21, in town (depth 0); the starting kit is discarded. Worn: a Wooden Torch, a Dagger and Soft Leather Armour; the cloak slot is empty. Pack: Tulwars, Metal Scale Mail, a Cloak, five Rations of Food and distinct potions that fill the pack to exactly one free slot. All runes known; every store, including the home, is emptied. The fixture calls the production borg_clear_best() as the Borg does on arriving in town, sets shop_num to the home, then repeats the production borg_best_stuff() (the first step of borg_think_store()) until it declines to act. Each queued command is checked (exactly 'w' or 't' plus the letter of the item the Borg named) and carried out with the game's own inven_wield() / inven_takeoff(); the Borg then re-reads its gear with borg_find_all_items() and borg_notice(). 'tight': two Tulwars stacked in one slot, one free slot. 'roomy': one Tulwar, two free slots. 'nocloak': as 'tight' but without the cloak (and one more potion). This isolates the home equipment steps; it does not walk to the home or play other store steps.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/wearskip-85
git fetch https://github.com/diegoami/neo-angband-tracker.git task/wearskip-85-evidence
git show 620235082d9faf6644d496f52fbc177a970fe266:docs/audit/c-reports/wearskip-85/reproduce.py > /tmp/reproduce-wearskip-85.py
python3 /tmp/reproduce-wearskip-85.py --full-tests --fixtures-ref 620235082d9faf6644d496f52fbc177a970fe266 --fix-ref 5d73e0c2120b450428f2417db325041c1b6d3da9
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-item-wear.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `5d73e0c2120b450428f2417db325041c1b6d3da9` (branch `task/wearskip-85`), fixtures
  `620235082d9faf6644d496f52fbc177a970fe266` (branch `task/wearskip-85-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: roomy, nocloak
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_one_step_wearing_best() reset its skip counter on every pass, so with skip > 0 it skipped every remaining planned change instead of only the blocked ones. A level-21 Warrior at home with one free pack slot stopped after switching to a Tulwar and never wore the Cloak or the Metal Scale Mail it had planned; corrected, it makes all five planned changes. A control with room in the pack and a control where the skip happens but nothing else can be done are byte-identical.

Not measured: how often a home visit starts with exactly one free pack slot and a planned change that fills it (here a stacked item being worn out of a stack); what the Borg does after leaving the home with the plan discarded (borg_wear_stuff() in the dungeon also needs a free pack slot, so the cloak would stay unworn while the pack is full, but that was not run); the home-item paths (dropping into or taking from the home), which use the same skip and are not exercised here.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
