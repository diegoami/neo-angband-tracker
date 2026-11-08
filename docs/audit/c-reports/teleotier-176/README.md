# Remove the unreachable fail-rate tier from the Teleport Other defence

This is **teleotier-176 / issue #176**, a C Borg defect
(diegoami/neo-angband-tracker#176).

Cleanup, no behaviour change; owner decision (option C). When the Borg considers teleporting a monster away, borg_defend_aux_tele_away() accepts less spell failure the more dangerous the situation is: 50% by default, 45% when the danger p1 is above twice the Borg's avoidance figure, 40% above three times. A third tier (+5, 'a little scary') sits in the else of the 'twice' test but asks for p1 above five halves of the avoidance, which a danger that is not above twice the avoidance never is. The change deletes the dead tier.

## What to watch

Teleport Other cast by borg_defend() (1 = cast and aimed; 0 = refused); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `841cd99ee`) |
| --- | --- | --- |
| Control: danger 3.5x avoidance (allowance 40%) | passed, cast=0 | byte-identical log |
| Control: danger 2.75x avoidance, the band the dead test names (allowance 45%) | passed, cast=1 | byte-identical log |
| Control: danger 2.25x avoidance (allowance 45%) | passed, cast=1 | byte-identical log |
| Control: danger 1.5x avoidance (allowance 50%) | passed, cast=1 | byte-identical log |

- **Control: danger 3.5x avoidance (allowance 40%).** Above three times the avoidance the allowance is 40%; Teleport Other fails 41% of the time, so both versions refuse it and borg_defend() does nothing. Logs byte-identical.

- **Control: danger 2.75x avoidance, the band the dead test names (allowance 45%).** 2.75x is above five halves, the condition of the deleted tier, but the 'twice' test catches it first in both versions: allowance 45%, Teleport Other cast and aimed. Logs byte-identical.

- **Control: danger 2.25x avoidance (allowance 45%).** Allowance 45%: cast in both versions. Logs byte-identical.

- **Control: danger 1.5x avoidance (allowance 50%).** Not above twice the avoidance: the original evaluates the dead test (false) and keeps the default 50%; the changed version keeps 50% directly. Cast in both. Logs byte-identical.

## Conditions

Native Human Mage, seed 176, clevel 16, depth 15, full HP (82) and SP, a torch, rations and only the [Magical Defences] book with only Teleport Other learned (spell_learn()), in a lit, open 24x40 arena with an awake hill giant 3 grids east, observed through observe_kill_diff(). The player is afraid (TMD_AFRAID set), which adds 20 to the failure rate and puts Teleport Other at 41%, between the 40% and 45% allowances, so the tiers decide the outcome. The production borg_defend(p1) runs with p1 set to a multiple of borg.avoidance (82): 'over3' 3.5x, 'upper' 2.75x (the band the dead test names), 'lower' 2.25x, 'under2' 1.5x; the queued keys are read back. Fixture interventions: the arena and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_VIEW, borg.avoidance is set to current HP, HP/SP are set to maximum, fear is set directly, borg_kills[k].awake is set by hand and p1 is passed directly instead of being computed by the caller.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/teleotier-176
git fetch https://github.com/diegoami/neo-angband-tracker.git task/teleotier-176-evidence
git show e8e36b09306a29c853188a45c9ac983bf52c55de:docs/audit/c-reports/teleotier-176/reproduce.py > /tmp/reproduce-teleotier-176.py
python3 /tmp/reproduce-teleotier-176.py --full-tests --fixtures-ref e8e36b09306a29c853188a45c9ac983bf52c55de --fix-ref 841cd99eebbe47ef6175b1ed2300548a2bfe0c2b
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-fight-defend.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `841cd99eebbe47ef6175b1ed2300548a2bfe0c2b` (branch `task/teleotier-176`), fixtures
  `e8e36b09306a29c853188a45c9ac983bf52c55de` (branch `task/teleotier-176-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: over3, upper, lower, under2
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The deleted tier in borg_defend_aux_tele_away() is unreachable: it is only tested when p1 is not above 2 x avoidance, and asks for p1 above (5 x avoidance) / 2, which is at least 2 x avoidance for any avoidance that is not negative (it is always set from hit points or a positive constant). Four native borg_defend() scenarios, one in each danger band, with a failure rate between the tiers so that the band decides whether Teleport Other is cast, give byte-identical logs before and after, and both full native gates pass on both builds.

This is a cleanup with no failing mode: the scenarios show unchanged behaviour, they cannot show a difference. p1 is passed to borg_defend() directly rather than computed from the monsters, to place it in each band. Not measured: whole-game runs, or which threshold the tier was meant to have (owner options A and B on the issue, not taken).

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
