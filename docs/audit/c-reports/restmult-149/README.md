# The Borg could rest a turn after killing a breeder because its check cleared the timer

This is **restmult-149 / issue149**, a C Borg defect
(diegoami/neo-angband-tracker#149).

After killing a multiplying monster the Borg should not rest for a few turns, in case unseen breeders are nearby. borg_check_rest() tests this with a timer, but the test only holds when the timer is above zero, which excludes the turn of the kill itself, and the function then cleared the timer. So a check made on the turn of the kill erased the memory of the kill, and the check one turn later allowed resting. The correction stops clearing the timer.

## What to watch

The Borg may rest one game turn after killing a multiplier (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `e66740cc4`) |
| --- | --- | --- |
| A multiplier was killed this very game turn | failed, rests_one_turn_after_the_kill=1 | passed, rests_one_turn_after_the_kill=0 |
| Control: a multiplier was killed two game turns ago | passed, rests_one_turn_after_the_kill=0 | byte-identical log |

- **A multiplier was killed this very game turn.** A level-12 Warrior with full food, light and hit points checks whether it may rest on the game turn it killed a multiplying monster, then again one game turn later. Original: the first check passes the 'recent multiplier kill' test (the timer is exactly 0) and clears the timer, so the second check, one turn after the kill, finds no recent kill and allows resting. Corrected: the timer is kept and the second check refuses.

- **Control: a multiplier was killed two game turns ago.** The same Warrior with the kill two game turns earlier. The test refuses at once, before the line that was removed, so both checks refuse before and after.

## Conditions

Native Human Warrior, seed 149, character level 12, full hit points, food and light, depth 5, a lit open arena with no monsters. Fixture interventions: the game-turn counter is set to 1000 and the timer of the last multiplier kill is set by the fixture (this turn, or two turns ago); the second check follows an increase of the counter by one. The production borg_check_rest() answers both checks.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/restmult-149
git fetch https://github.com/diegoami/neo-angband-tracker.git task/restmult-149-evidence
git show e76b239b3882a1f63ac88516cf380eeae2a0ccd1:docs/audit/c-reports/restmult-149/reproduce.py > /tmp/reproduce-restmult-149.py
python3 /tmp/reproduce-restmult-149.py --full-tests --fixtures-ref e76b239b3882a1f63ac88516cf380eeae2a0ccd1 --fix-ref e66740cc4563224ca5dddab5d7bd543d8c8f1342
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-misc.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `e66740cc4563224ca5dddab5d7bd543d8c8f1342` (branch `task/restmult-149`), fixtures
  `e76b239b3882a1f63ac88516cf380eeae2a0ccd1` (branch `task/restmult-149-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: recent
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

On the turn of a multiplier kill, the original check passed the recent-kill test and cleared the timer, so a check one turn later allowed resting; without the clearing, that second check refuses. With the kill two turns old both checks refuse before and after, and that control is byte-identical.

Priority low: this changes the answer for the turns right after a kill in a scene with no monsters; it does not measure resting next to real breeders or the other sub-symptoms in the report (the vault neighbourhood the check examines, and the danger of a monster measured at its own grid), which this fix does not touch.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
