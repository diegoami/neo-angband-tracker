# No borg.txt and no way to write one crashed the Borg

This is **cfgmissing-144 / issue144**, a C Borg defect
(diegoami/neo-angband-tracker#144).

When the Borg starts and finds no borg.txt, it tries to write a default one. If that write fails (for example the user folder is read-only or missing), it printed a warning and returned before creating its settings table, then crashed at the first setting it read. The correction creates the default settings first, so the Borg starts with defaults and the warning.

## What to watch

Borg start-up finished (0 = crashed); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `8cbee6944`) |
| --- | --- | --- |
| borg.txt cannot be read or written | failed, init_completed=0 | passed, init_completed=1 |
| Control: stock borg.txt present | passed, init_completed=1 | byte-identical log |

- **borg.txt cannot be read or written.** Original: after the 'unable to write default BORG.TXT' warning, borg_init() reads the unallocated settings table and the process gets SIGSEGV. Corrected: start-up completes with the default settings (borg_stop_dlevel 128).

- **Control: stock borg.txt present.** borg.txt is read normally; both versions start and the logs are identical.

## Conditions

Native Human Warrior, seed 144. The production borg_init() runs as when the Borg is started. 'missing': the user directory is set, after the game's own start-up, to a path that runs through a regular file, so borg.txt can be neither read nor written even as root. A SIGSEGV handler reports a crash instead of losing the output. 'present' (control): the user directory holds the stock borg.txt; identical in both builds. The second sub-case (default written but not re-readable) is corrected by the same change but not exercised.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cfgmissing-144
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cfgmissing-144-evidence
git show 1ff9ca1e59d639b6693b9bcb40190c6d836c6fa9:docs/audit/c-reports/cfgmissing-144/reproduce.py > /tmp/reproduce-cfgmissing-144.py
python3 /tmp/reproduce-cfgmissing-144.py --full-tests --fixtures-ref 1ff9ca1e59d639b6693b9bcb40190c6d836c6fa9 --fix-ref 8cbee69447cda2526b4810fd6ea74677921ff1d1
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-init.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `8cbee69447cda2526b4810fd6ea74677921ff1d1` (branch `task/cfgmissing-144`), fixtures
  `1ff9ca1e59d639b6693b9bcb40190c6d836c6fa9` (branch `task/cfgmissing-144-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: present
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

When borg.txt is missing and cannot be written, borg_init_txt_file() returned before allocating borg_cfg and borg_init() crashed (SIGSEGV) reading it. Allocating the defaults before opening the file lets start-up complete with defaults; the reopen-failure path now also returns with defaults. The stock-file control is byte-identical.

Not proved: how often it happens (it needs both a missing borg.txt and an unwritable user folder). The crash is a NULL-pointer read observed directly as SIGSEGV; no sanitizer build is needed to see it. The second path (default written but not re-readable) is corrected by the same change but is not exercised by this fixture.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
