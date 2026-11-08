# The Borg did not refuel its lantern from a spare that had fuel

This is **lantern-90 / issue90**, a C Borg defect
(diegoami/neo-angband-tracker#90).

With no flask of oil the Borg refuels its wielded lantern from a spare lantern, walking the lanterns from the first one and skipping Everburning and empty ones. The scan started from borg_slot(), which returns the last lantern in the pack. When that lantern was empty or Everburning the scan stopped at once, so an earlier lantern with fuel was never used. The correction starts at the first lantern in the pack.

## What to watch

The Borg refuels from the spare lantern (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `bd4698a85`) |
| --- | --- | --- |
| Full spare lantern in front of an empty one | failed, refuels=0 | passed, refuels=1 |
| Control: one full spare lantern | passed, refuels=1 | byte-identical log |

- **Full spare lantern in front of an empty one.** A Warrior wields a Lantern with 500 turns of fuel, has no flask of oil, and carries two spare Lanterns: one with 5000 turns, then one with 0. Original: the scan starts at the last lantern (empty), finds nothing to use and the Borg does not refuel (unmet need). Corrected: the scan starts at the full lantern and the Borg refuels with it.

- **Control: one full spare lantern.** The same Warrior with only the full spare Lantern. The first and the last lantern are the same, so the Borg refuels from it before and after.

## Conditions

Native Human Warrior, seed 90, character level 12, depth 5, a lit open arena. The Warrior wields a Lantern with 500 turns of fuel (the refuel threshold is 1000), has no flasks of oil (the starting ones are removed), and carries a Lantern with 5000 turns and, except in the control, an empty Lantern. The production borg_maintain_light() decides; its keypresses are read, not played. Fixture intervention: the starting flasks are removed and the lanterns' fuel is set.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lantern-90
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lantern-90-evidence
git show 505180c1268aa95a36818078b0633175b29e11c9:docs/audit/c-reports/lantern-90/reproduce.py > /tmp/reproduce-lantern-90.py
python3 /tmp/reproduce-lantern-90.py --full-tests --fixtures-ref 505180c1268aa95a36818078b0633175b29e11c9 --fix-ref bd4698a850e9fb926d74c9a26be298716a33e5ba
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-light.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `bd4698a850e9fb926d74c9a26be298716a33e5ba` (branch `task/lantern-90`), fixtures
  `505180c1268aa95a36818078b0633175b29e11c9` (branch `task/lantern-90-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: single
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With a full spare lantern in front of an empty one, the original Borg did not refuel (the scan began at the empty one and stopped); with the scan starting at the first lantern it refuels from the full one. The single-spare control is byte-identical.

Priority low to medium: this measures the refuel decision in one pack layout, not the game turns until the light runs out. The layout (a full lantern in front of an empty or Everburning one) needs two spare lanterns, which is not common. The same scan code is not otherwise changed.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
