# Home-sell trials drained staff charges and the Borg emptied its home

This is **sellcharges-132 / issue132**, a C Borg defect
(diegoami/neo-angband-tracker#132).

In town the Borg decides what to keep in its home by trying each pack item in each home slot and scoring the result. For a staff it moves one staff's share of the charges into the trial slot and takes them off the pack stack, but after each trial it put back only the staff, not the charges. Every later trial in the same scan saw a staff with fewer charges. A level-26 Mage with two Staffs of Teleportation (10 charges) and a Word of Recall scroll in the home scored 'replace the scroll with a staff' at full charges but 'add a staff beside the scroll' at a drained share, so it decided to take the scroll out of the home. The correction puts the charges back with the staff; the Borg then stores a staff and keeps the scroll at home.

## What to watch

Takes the recall scroll out of the home (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `58a2638c7`) |
| --- | --- | --- |
| Two teleport staves, recall scroll at home | failed, takes_recall_out=1 | passed, takes_recall_out=0 |
| Control: two teleport staves, empty home | passed, takes_recall_out=0 | byte-identical log |
| Control: one teleport staff | passed, takes_recall_out=0 | byte-identical log |
| Control: no staff or wand | passed, takes_recall_out=0 | byte-identical log |

- **Two teleport staves, recall scroll at home.** Original: by the second trial (adding a staff beside the scroll) the pack stack has lost 5 of its 10 charges, so the trial staff gets a 2-charge share (5/2) instead of 5; that addition scores below the first trial (replacing the scroll, home power 9,914), and the Borg sets out to buy the Word of Recall back out of the home. Corrected: the addition keeps its 5-charge share (home power 10,014) and the Borg stores a staff instead.

- **Control: two teleport staves, empty home.** Only one home slot to try, so the drain happens after the last trial and the caller's restore hides it. Both versions store a staff; the logs are identical.

- **Control: one teleport staff.** Giving up its only staff would lower the Borg's own power, so the item is never tried. Both versions leave the home alone; the logs are identical.

- **Control: no staff or wand.** No charged device in the pack. Both versions leave the home alone; the logs are identical.

## Conditions

Native Human Mage, seed 132, character level 26, 5000 gold, in town (depth 0) with its starting kit plus fixture items, all runes known. Every store is emptied. 'stack': two Staffs of Teleportation stacked in one pack slot with 10 charges set by the fixture (5 each), and one Scroll of Word of Recall in the home. 'stack-emptyhome': the same staves, home empty. 'single': one staff with 5 charges, recall scroll at home. 'none': no staff or wand, recall scroll at home. The Borg's items and stores are loaded with the production borg_find_all_items(); then the production borg_think_home_sell_useful() (step 1 of the store logic) runs once and its goal is read back. This isolates the home-sell decision; it does not walk to the home or carry out the transfer.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/sellcharges-132
git fetch https://github.com/diegoami/neo-angband-tracker.git task/sellcharges-132-evidence
git show 93253cd7250235f2f7ac0d9b878086f638dbea01:docs/audit/c-reports/sellcharges-132/reproduce.py > /tmp/reproduce-sellcharges-132.py
python3 /tmp/reproduce-sellcharges-132.py --full-tests --fixtures-ref 93253cd7250235f2f7ac0d9b878086f638dbea01 --fix-ref 58a2638c7acc7c684fc7e6fca6df3921e93b0d9a
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-sell.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `58a2638c7acc7c684fc7e6fca6df3921e93b0d9a` (branch `task/sellcharges-132`), fixtures
  `93253cd7250235f2f7ac0d9b878086f638dbea01` (branch `task/sellcharges-132-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: stack-emptyhome, single, none
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_think_home_sell_best() took one staff's share of charges off the pack stack on every trial and never put it back until the caller's final restore. With two Staffs of Teleportation (10 charges) and a Word of Recall at home, the 'add beside the scroll' trial was valued with 2 charges instead of 5, so 'replace the scroll' won and the Borg set out to take the scroll out of the home. Restoring the charges with the count makes it store a staff instead. Controls with an empty home, a single staff and no device are byte-identical.

Measured: the step-1 goal of one scan. Not measured: the walk to the home, the transfer itself, or what the Borg does on the following store turns (after taking the scroll out it may store the staff on a later scan). The pack stack's charges are fixture-set (10 for two staves). Wands follow the same code path but the home valuation (borg-home-notice.c) has no wand case, so a wand alone never scores above an empty home and is never tried here; the wand half has no observed consequence in this function. The caller restores the pack from safe_items after the scan, so the drain does not leak past borg_think_home_sell_useful().

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
