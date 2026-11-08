# A stray minus sign stopped the Borg from ever going deeper

This is **cfgminus-142 / issue142**, a C Borg defect
(diegoami/neo-angband-tracker#142).

borg.txt settings may be negative, written with a leading minus sign. The reader remembered a '-' and then negated the setting afterwards, even when no number followed it. So a line such as 'borg_no_deeper = -' (a half-edited value) turned the default 127 into -127. The Borg's depth-readiness check then answers 'No deeper -127.' for every depth below the first, so a strong, fully prepared Borg never goes deeper. The correction applies the sign only when a number is read; a bare minus keeps the default.

## What to watch

Ready for depth 20 (1 = no reason to stop); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `6bc43e71a`) |
| --- | --- | --- |
| borg_no_deeper = - (no number) | failed, prepared20=0 | passed, prepared20=1 |
| Control: borg_no_deeper = 100 | passed, prepared20=1 | byte-identical log |
| Control: borg_no_deeper = -5 | passed, prepared20=0 | byte-identical log |

- **borg_no_deeper = - (no number).** Original: no_deeper becomes -127 and the readiness check answers 'No deeper -127.'. Corrected: the default 127 stays and the Borg is ready for depth 20.

- **Control: borg_no_deeper = 100.** A normal value; both versions read 100 and are ready. Identical logs.

- **Control: borg_no_deeper = -5.** A real negative number is still read as -5 by both versions (and blocks depth 20, as configured). Identical logs.

## Conditions

Native Human Warrior, seed 142, level 50, maximum depth 50, on depth 30, worn gear resisting fire, cold and lightning plus Free Action and ESP, with food, recall, phase, teleport and cure supplies (the same character as swapacid-199, without a swap weapon). For each case the stock borg.txt gets one extra borg_no_deeper line; the production borg_init() reads it and the production borg_prepared(20) runs once on the static preparedness path.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cfgminus-142
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cfgminus-142-evidence
git show 18f79a3cc631b76b55524e9253a79f383fd6d9a3:docs/audit/c-reports/cfgminus-142/reproduce.py > /tmp/reproduce-cfgminus-142.py
python3 /tmp/reproduce-cfgminus-142.py --full-tests --fixtures-ref 18f79a3cc631b76b55524e9253a79f383fd6d9a3 --fix-ref 6bc43e71a4b36189ba7246abe53f114d6259469c
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-init.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `6bc43e71a4b36189ba7246abe53f114d6259469c` (branch `task/cfgminus-142`), fixtures
  `18f79a3cc631b76b55524e9253a79f383fd6d9a3` (branch `task/cfgminus-142-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: valid, negative
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_proc_setting() negated an integer setting after a '-' even when no digits followed. 'borg_no_deeper = -' gave -127 and borg_prepared(20) answered 'No deeper -127.' for a fully prepared level-50 Warrior; after applying the sign only to a read number the default 127 stays and the Borg is ready. 'borg_no_deeper = 100' and '= -5' controls are byte-identical.

Not measured: how often players write a bare minus. The issue's second part (numbers beyond the int range read through atoi) is not changed here: it only affects implausibly large values, and is left for a separate decision.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
