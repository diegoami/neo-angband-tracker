# The Borg took back rations it had just stored at home

This is **tradehist-20 / issue #20**, a C Borg defect
(diegoami/neo-angband-tracker#20).

The Borg keeps a short list of what it has just sold or stored at home, and of what it has just bought, so that it does not buy back what it just got rid of (and the reverse). The list kept the position of the newest entry where the readers expected a count, so every check stopped one entry short and the newest sale or purchase was never looked at. A level-6 Warrior in its home with eleven rations stored three of them, then on its very next store turn took a ration back out, and again on the turn after, ending with two of the three back in its pack. The correction counts the newest entry; the Borg stores the three rations and leaves.

## What to watch

Rations taken back out of the home right after storing them; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `9ae5e4585`) |
| --- | --- | --- |
| Eleven rations, nothing else to store or sell | failed, retrieved_rations=2 | passed, retrieved_rations=0 |
| Control: the stored rations are not the newest entry | passed, retrieved_rations=0 | byte-identical log |

- **Eleven rations, nothing else to store or sell.** Original: turn 1 stores 3 rations (newest history entry, not checked); turns 2 and 3 take a ration back out each time (step 4, buy useful things from the home) until the pack holds ten. Corrected: turn 1 stores 3 rations, turn 2 sees them in the history and leaves.

- **Control: the stored rations are not the newest entry.** Turn 1 stores 3 rations, turn 2 stores the Word of Recall scroll, so the rations are the older entry, which both versions check. Neither takes anything back; the logs are identical.

## Conditions

Native Human Warrior, seed 20, level 6, 3000 gold, standing in the home entrance of a small arena with every store and the home empty. Kit: the starting Warrior kit with its ration stack raised to eleven (the Borg values up to ten rations it carries, and a level-6 Borg values rations stored at home). 'rations-only': the starting Scroll of Word of Recall and Potion of Berserk Strength are removed, as after recalling to town and drinking the potion. 'with-recall': both are kept. Each store turn runs the production borg_think_store(); the keys it queues are read back, the inventory letter or home slot is checked, and the matching real store command (do_cmd_stash or do_cmd_retrieve, quantity as typed; retrieval accepts the prompt's default of one) is executed. The fixture then runs the next store turn directly at the same entrance instead of walking out and back in. The sale/purchase history starts from its program start-up values; no history entry is written by the fixture.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tradehist-20
git fetch https://github.com/diegoami/neo-angband-tracker.git task/tradehist-20-evidence
git show 942c80a50ecd986daa5629c7903139bdfa1866f7:docs/audit/c-reports/tradehist-20/reproduce.py > /tmp/reproduce-tradehist-20.py
python3 /tmp/reproduce-tradehist-20.py --full-tests --fixtures-ref 942c80a50ecd986daa5629c7903139bdfa1866f7 --fix-ref 9ae5e4585599aa4f3c5dce7c2259fe7e2604cea1
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-buy.c, src/borg/borg-store-sell.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `9ae5e4585599aa4f3c5dce7c2259fe7e2604cea1` (branch `task/tradehist-20`), fixtures
  `942c80a50ecd986daa5629c7903139bdfa1866f7` (branch `task/tradehist-20-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: with-recall
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_think_shop_sell() and borg_think_shop_buy() stored the index of the entry just written in sold_item_num / bought_item_num, while every reader scans p < num, so the newest sale or purchase was never checked. A level-6 Warrior that stored 3 of its 11 rations at home took 2 back out over its next two store turns. Counting the slots in use restores the check; the control in which the stored rations are not the newest entry is byte-identical.

Measured: the home store-then-retrieve loop for rations, three to four store turns. Not measured: the purchase-history half (sell-back of something just bought) as a game-level decision; it shares the same counter and readers and is corrected by the same change, but no scenario here shows it changing a decision. Not measured: how often this happens in full games, or the loop with the new-level reset values (num 0, next slot 1), which by reading the source miss the newest entry in the same way. The loop is bounded by the Borg's own power comparison (it stops at ten carried rations), so the cost is wasted store turns and fewer rations kept at home, not a game-ending error. The ring still wraps at nine and never uses its tenth slot; that capacity detail is left unchanged.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
