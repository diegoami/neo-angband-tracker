# Remove the unreachable second amulet tier from the home score

This is **neckdead-47 / issue #47**, a C Borg defect
(diegoami/neo-angband-tracker#47).

Cleanup, no behaviour change; owner decision (option C). The Borg scores the contents of its home and takes points off for keeping too many items of one type, so that it does not fill the home with duplicates. For amulets, borg_power_home_aux1() subtracts 1500 for each amulet past the third; in the else of that test it repeats the same condition (num_neck > 3) for a gentler 700-point tier, so that tier can never run. The change deletes the dead tier. Today's behaviour is kept: one to three amulets cost nothing, each amulet past the third costs 1500.

## What to watch

Spare Amulet of Slow Digestion stored in the home by borg_think_home_sell_useful() (1 = stored, 0 = kept in the pack); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `e9080f35f`) |
| --- | --- | --- |
| Control: 2 amulets in the home (spare stored) | passed, stores_amulet=1 | byte-identical log |
| Control: 3 amulets in the home (spare kept) | passed, stores_amulet=0 | byte-identical log |
| Control: 4 amulets in the home, first tier active (spare kept) | passed, stores_amulet=0 | byte-identical log |
| Control: 5 amulets in the home, first tier active (spare kept) | passed, stores_amulet=0 | byte-identical log |

- **Control: 2 amulets in the home (spare stored).** Two amulets at home, num_neck=2: no amulet penalty. Home score 2040; storing the spare raises it to 2090, so both versions store the Amulet of Slow Digestion. Logs byte-identical.

- **Control: 3 amulets in the home (spare kept).** Three amulets at home, num_neck=3: no penalty yet (home score 3120); a fourth would cost 1500, so both versions keep the spare in the pack. Logs byte-identical.

- **Control: 4 amulets in the home, first tier active (spare kept).** Four amulets at home, num_neck=4: the 1500 penalty applies and the original then skips the dead else. Home score 2625; both versions keep the spare. Logs byte-identical.

- **Control: 5 amulets in the home, first tier active (spare kept).** Five amulets at home, num_neck=5: penalty 2 x 1500. Home score 2130; both versions keep the spare. Logs byte-identical.

## Conditions

Native Human Warrior, seed 47, clevel 21, in town (depth 0), with the starting pack plus one known Amulet of Slow Digestion. Every store is emptied, then the home is given 2, 3, 4 or 5 distinct known amulets (Resist Lightning, Resist Acid, Resistance, Regeneration, Infravision, in that order of choice) through store_carry(), so the duplicate check does not enter the score. The Borg reads the stores with borg_find_all_items(); the production borg_notice_home() and borg_power_home() give the amulet count and home score, and the production borg_think_home_sell_useful() decides whether to store the spare amulet. Fixture interventions: the stores are emptied and stocked directly, all runes are learned with player_learn_all_runes(), dynamic preparedness formulas are off (BORG_USES_DYNAMIC_CALCS = false), and current HP is set to maximum.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/neckdead-47
git fetch https://github.com/diegoami/neo-angband-tracker.git task/neckdead-47-evidence
git show 530689cc736c52295417c5ba853fd2ee1df26b22:docs/audit/c-reports/neckdead-47/reproduce.py > /tmp/reproduce-neckdead-47.py
python3 /tmp/reproduce-neckdead-47.py --full-tests --fixtures-ref 530689cc736c52295417c5ba853fd2ee1df26b22 --fix-ref e9080f35f1d2e8c8166116d0025d2e61e499899e
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-home-power.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `e9080f35f1d2e8c8166116d0025d2e61e499899e` (branch `task/neckdead-47`), fixtures
  `530689cc736c52295417c5ba853fd2ee1df26b22` (branch `task/neckdead-47-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: neck2, neck3, neck4, neck5
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The removed tier in borg_power_home_aux1() can never run: it is the else of 'num_neck > 3' and tests 'num_neck > 3' again. Four native scenarios with 2, 3, 4 and 5 amulets in the home, which run the production home score and the production decision to store a spare amulet (stored with 2, kept with 3, 4 and 5), give byte-identical logs before and after, and both full native gates pass on both builds.

This is a cleanup with no failing mode: the scenarios show unchanged behaviour, they cannot show a difference. Unreachability is argued from the source (the else repeats its own if). Not measured: whole-game runs, or what second threshold the tier was meant to have (owner options A and B on the issue, not taken).

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
