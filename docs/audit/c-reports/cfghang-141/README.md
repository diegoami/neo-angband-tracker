# One mistyped borg.txt line froze the game at start

This is **cfghang-141 / issue141**, a C Borg defect
(diegoami/neo-angband-tracker#141).

When the Borg starts, it reads its settings from borg.txt. For a number setting, the reader skipped spaces and '=' and then waited for a digit or a minus sign; any other character was never consumed, so it looped forever. A single line such as 'borg_stop_dlevel = off', or a setting name with extra letters such as 'borg_stop_dlevelx 5', froze the game at the moment the Borg was started. The correction stops at any other character and keeps that setting's default value.

## What to watch

Borg start-up finished (0 = hung reading borg.txt); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `45dfc9b1f`) |
| --- | --- | --- |
| borg_stop_dlevel = off | failed, init_completed=0 | passed, init_completed=1 |
| borg_stop_dlevelx 5 (extra letter in the name) | failed, init_completed=0 | passed, init_completed=1 |
| Control: borg_stop_dlevel = 7 | passed, init_completed=1 | byte-identical log |

- **borg_stop_dlevel = off.** Original: the reader never moves past the 'o'; borg_init() does not return and the watchdog fires after 20 seconds. Corrected: start-up completes; borg_stop_dlevel keeps its default (128).

- **borg_stop_dlevelx 5 (extra letter in the name).** Original: the name matches as a prefix and the reader spins on the 'x'. Corrected: start-up completes with the default 128.

- **Control: borg_stop_dlevel = 7.** A well-formed number; both versions read 7 and the logs are identical.

## Conditions

Native Human Warrior, seed 141. For each case the stock borg.txt (from this package) is copied with one line appended, and the production borg_init() runs as when the Borg is started. A 20-second watchdog (SIGALRM) reports a hang and ends the run if borg_init() does not return; in the original build both malformed cases hit it. The valid case appends 'borg_stop_dlevel = 7' and is identical in both builds.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cfghang-141
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cfghang-141-evidence
git show a3164a61b830bf1c67e10017f9a2be7dec9a1763:docs/audit/c-reports/cfghang-141/reproduce.py > /tmp/reproduce-cfghang-141.py
python3 /tmp/reproduce-cfghang-141.py --full-tests --fixtures-ref a3164a61b830bf1c67e10017f9a2be7dec9a1763 --fix-ref 45dfc9b1f4930b28841d0c38b73cc4cd3171aab0
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-init.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `45dfc9b1f4930b28841d0c38b73cc4cd3171aab0` (branch `task/cfghang-141`), fixtures
  `a3164a61b830bf1c67e10017f9a2be7dec9a1763` (branch `task/cfghang-141-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: valid
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_proc_setting() never consumed a character other than space, '=', '-' or a digit after an integer setting name, so 'borg_stop_dlevel = off' or 'borg_stop_dlevelx 5' in borg.txt hung borg_init() (watchdog fired at 20 s). Stopping at any other character keeps the default and start-up completes; the valid-setting control is byte-identical.

Not proved: how often real players hit this. It needs a hand-edited borg.txt with a non-numeric value or a misspelt number-setting name; the stock file is well formed. The fix keeps the setting's default silently (no new warning). The 20-second watchdog is a fixture device: without it the original simply never returns.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
