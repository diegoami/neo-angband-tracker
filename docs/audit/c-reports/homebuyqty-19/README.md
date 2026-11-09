# A skipped home-buy trial hid a home weapon from the backup-weapon step

This is **homebuyqty-19 / issue #19**, a C Borg defect
(diegoami/neo-angband-tracker#19).

In town the Borg's step 4 tries each item in its home to see whether buying it back would make the Borg stronger. For each trial it first takes the item off its own record of the home, and only then checks that the pack has two free slots. When that check fails it skips the item without putting it back on the record. A level-21 Warrior with one free pack slot and a single Tulwar in the home therefore records the home as holding zero Tulwars; step 7A, which looks for a backup weapon and needs only one free slot, then skips the Tulwar as an empty home slot and the Borg takes nothing. The correction puts the item back on the record before skipping it; step 7A then sees the Tulwar and the Borg sets out to buy it as its backup weapon.

## What to watch

Sets out to buy the home item (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `e075f0ac7`) |
| --- | --- | --- |
| One Tulwar at home, one free pack slot | failed, buys_home_ware0=0 | passed, buys_home_ware0=1 |
| Two Tulwars at home, one free pack slot | failed, buys_home_ware0=1 | passed, buys_home_ware0=1 |
| Control: one Tulwar, two free pack slots | passed, buys_home_ware0=1 | byte-identical log |
| Control: a potion at home that stacks with the pack | passed, buys_home_ware0=0 | byte-identical log |

- **One Tulwar at home, one free pack slot.** Original: step 4 takes the Tulwar off its home record, finds only one free pack slot and skips it, leaving the record at 0 Tulwars; step 7A then skips home slot 0 as empty and no step sets a goal. Corrected: the record is restored to 1, step 7A tries the Tulwar as a backup weapon and the Borg sets out to buy it from the home.

- **Two Tulwars at home, one free pack slot.** The same early skip leaves the record at 1 of 2 Tulwars in the original (2 corrected), so the record check fails before and passes after. One Tulwar is still visible, so step 7A buys a backup weapon in both versions: the decision is unchanged here, only the recorded quantity differs.

- **Control: one Tulwar, two free pack slots.** With two free slots the room check passes, the trial runs to the end and the record is restored as before; step 4 itself picks the Tulwar in both versions. The logs are identical.

- **Control: a potion at home that stacks with the pack.** The home potion would join an existing pack stack, so the inventory trial needs no free slot and runs to the end; the record is restored as before and no step takes anything. The logs are identical.

## Conditions

Native Human Warrior, seed 19, character level 21, maximum depth 50 (the depth at which the Borg starts carrying backup weapons; the shipped borg.txt has borg_uses_swaps = TRUE), 5000 gold, in town (depth 0) with its starting kit, all runes known. The pack is filled with distinct single potions and scrolls until only its last slot is free ('roomy': the last two slots). Every store is emptied. 'single': one Tulwar in the home. 'pair': two Tulwars stacked in one home slot. 'roomy': one Tulwar, two free pack slots. 'stack': one Potion of Cure Light Wounds in the home, which stacks with the one in the pack. The Borg's items and stores are loaded with the production borg_find_all_items(); then the production store steps 4 (borg_think_home_buy_useful), 5, 6, 7A and 7B run in the order borg_choose_shop() runs them, stopping at the first that sets a goal, and the goal is read back. Steps 1 to 3 (selling to the home and shops, buying from shops) are not run: with this filler pack they choose to sell single potions or scrolls first, which would end the scan before step 4.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/homebuyqty-19
git fetch https://github.com/diegoami/neo-angband-tracker.git task/homebuyqty-19-evidence
git show 40495b33a5136be40b09b317f731f60841686c00:docs/audit/c-reports/homebuyqty-19/reproduce.py > /tmp/reproduce-homebuyqty-19.py
python3 /tmp/reproduce-homebuyqty-19.py --full-tests --fixtures-ref 40495b33a5136be40b09b317f731f60841686c00 --fix-ref e075f0ac7c618fdd32eb5bd4b49d1936e8738da4
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-buy.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `e075f0ac7c618fdd32eb5bd4b49d1936e8738da4` (branch `task/homebuyqty-19`), fixtures
  `40495b33a5136be40b09b317f731f60841686c00` (branch `task/homebuyqty-19-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: roomy, stack
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_think_home_buy_useful() subtracted the trial quantity from its home record before the two 'require two empty slots' checks, and those checks skipped the ware with a bare continue that bypassed the end-of-loop restore. With one free pack slot and a single Tulwar at home the record read 0 Tulwars, so the next store step that reads it (step 7A, backup weapon from home) skipped the Tulwar and the Borg took nothing. Restoring the ware before each skip makes step 7A buy the Tulwar. Controls with two free slots and with a stacking potion are byte-identical.

Measured: the goal set by one pass of store steps 4 to 7B. Not measured: steps 1 to 3 (not run, see conditions), the walk to the home, the purchase, or later turns. Each think re-reads the stores, so the record is corrected on the next turn, but the same early skip repeats while the pack has exactly one free slot (inferred from the code, not measured over turns). The staff and wand charge decrement on the same path is restored by the same lines; no charge-dependent consequence was measured. Steps 5 and 6 also read the home record but return early when only one pack slot is free, so with this bug's precondition only steps 7A and 7B can see the reduced record; 7B (backup armour) was not separately measured. No real item is lost: only the Borg's record is wrong.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
