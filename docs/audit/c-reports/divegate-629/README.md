# Dive gates skipped while no up staircase has been seen

This is **divegate-629 / issue #629**, a C Borg defect
(diegoami/neo-angband-tracker#629).

Before the Borg walks down a staircase it runs four safety gates: am I prepared for the next depth, am I hungry, do I still have things to sell, do I have a light. All four sit inside a check that only runs when the Borg has seen an up staircase on this level. A Borg that arrives on a level without having seen an up staircase - exactly what a monster Teleport-Level spell or a Deep Descent scroll produces - therefore dives with no checks at all. The correction runs the gates unconditionally; the gates themselves are unchanged.

## What to watch

1: the borg dove without its gates; 0: the gates held; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `cdd36650d`) |
| --- | --- | --- |
| No up-stair seen: the gates must hold | failed, flowed=1 | passed, flowed=0 |
| Up-stair seen: unchanged control | passed, flowed=0 | byte-identical log |

- **No up-stair seen: the gates must hold.** A lightless Borg stands one grid from a down staircase and has seen no up staircase. The original skips every dive gate and queues the step toward the stairs; the correction runs the no-light gate and refuses.

- **Up-stair seen: unchanged control.** The same lightless Borg with an up staircase also seen: the no-light gate already ran inside the wrapper, so both variants refuse, and the complete log is byte-identical.

## Conditions

Native arena fixture (24x40), Human Warrior at character level 17 with no light source at all (the birth torch is taken off), standing one grid south of a down staircase that is registered as seen, on depth 8, seed 629, borg_rand_local 1000, no monsters. The defect scenario has seen no up staircase; the control has also seen an up staircase two grids east. The production dive decision runs with the ordinary dungeon-brain arguments (borg_flow_stair_more(GOAL_FLEE, false, false)). Measured: the flow decision, the queued keys and the Borg's own flow note. Not measured: the full dungeon brain around the decision, the preparedness and hunger gates' own thresholds beyond this character, and any later turn the Borg might take after refusing.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/divegate-629
git fetch https://github.com/diegoami/neo-angband-tracker.git task/divegate-629-evidence
git show 97d3524ab2afc84cab09348c6e361333c47d599d:docs/audit/c-reports/divegate-629/reproduce.py > /tmp/reproduce-divegate-629.py
python3 /tmp/reproduce-divegate-629.py --full-tests --fixtures-ref 97d3524ab2afc84cab09348c6e361333c47d599d --fix-ref cdd36650d774c8ecd423d0c770d1af9ffc07731d
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-stairs.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6`, fix `cdd36650d774c8ecd423d0c770d1af9ffc07731d` (branch `task/divegate-629`), fixtures
  `97d3524ab2afc84cab09348c6e361333c47d599d` (branch `task/divegate-629-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: control
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

A lightless Borg one grid from a down staircase, having seen no up staircase, dives (queues the step) on the original and refuses with the correction; the same lightless Borg with the up staircase also seen refuses on both variants, and that control's complete log is byte-identical. Both full native gates pass at 965/965 and 5/5 in each variant.

The fixture measures the production dive decision only, not the full dungeon brain that surrounds it. The no-light gate is the deterministic gate for this character; the other three gates (preparedness, hunger, sell-crap) are the same code motion and are exercised by every caller in the ordinary seen-up-stair case, which the run history covers.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
