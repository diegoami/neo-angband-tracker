# Removed-rubble handler marks only the goal grid

This is **rubblemark-192 / issue #192**, a C Borg defect
(diegoami/neo-angband-tracker#192).

A lightless Warrior digs rubble next to himself in the dark. The game prints its real dig message; the Borg's screen reader hands it to the production parser. The handler is meant to convert the rubble grids around the Borg to broken doors on the Borg's own map so the freshly dug ground is not treated as rubble any more. In the original, the loop walks a 2x2 block while the body only ever inspects the goal grid, so with a goal on clear floor nothing is converted and every dug grid stays rubble on the Borg's map. The correction walks the full 3x3 block around the Borg and converts each grid that is rubble.

## What to watch

Rubble grids converted to broken doors (0..5); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `1abc77849`) |
| --- | --- | --- |
| Lightless dig: the whole neighbourhood is marked | failed, broken=0 | passed, broken=5 |
| Lit dig: unchanged control | passed, broken=0 | byte-identical log |

- **Lightless dig: the whole neighbourhood is marked.** The Warrior carries no light and digs the rubble grid north of himself. The original converts nothing (its body tests only the clear-floored goal grid); the correction converts all five rubble grids around the Borg to broken doors.

- **Lit dig: unchanged control.** The same dig with a worn torch: the handler skips everything while the Borg has light, so no grid changes on either variant and the complete log is byte-identical.

## Conditions

Native arena fixture (24x40), Human Warrior at character level 16 with a wielded Shovel and a fixture-set digging skill of 400 so the fixed seed's dig roll clears; no light source at all in the defect case (the birth torch is taken off), a worn torch in the control; five rubble grids laid around the Borg (north, north-east, east, south-east, north-west); the Borg's goal grid sits on clear floor outside the neighbourhood, as it may in ordinary play; depth 8, seed 192, borg_rand_local 1000. The dig runs through the production command (do_cmd_tunnel), the game's own message is captured through the EVENT_MESSAGE handler and handed to borg_parse exactly as the screen reader would. Measured: the Borg map features at the five rubble grids after the parse. Not measured: later Borg decisions that consume the marked map, other dig positions, and any game-side effect of the dig beyond the message.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/rubblemark-192
git fetch https://github.com/diegoami/neo-angband-tracker.git task/rubblemark-192-evidence
git show 28a1553daefcc9251755cb50e319e49c2618a86b:docs/audit/c-reports/rubblemark-192/reproduce.py > /tmp/reproduce-rubblemark-192.py
python3 /tmp/reproduce-rubblemark-192.py --full-tests --fixtures-ref 28a1553daefcc9251755cb50e319e49c2618a86b --fix-ref 1abc778496e01d97cd40720cd72ec2759b6d3586
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-messages.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6`, fix `1abc778496e01d97cd40720cd72ec2759b6d3586` (branch `task/rubblemark-192`), fixtures
  `28a1553daefcc9251755cb50e319e49c2618a86b` (branch `task/rubblemark-192-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: control
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

A lightless Borg that digs rubble next to itself keeps all five dug neighbourhood grids as rubble on its own map with the original handler (which tests only the goal grid, here clear floor), and has all five converted to broken doors with the correction. The lit control, where the handler skips everything, is byte-identical between the variants, and both full native gates pass at 965/965 and 5/5 in each variant.

The fixture measures the Borg's map marking after the production parse, not later movement or digging decisions that consume it. The dig message is produced by the game's own dig code and captured verbatim; no message text is manufactured.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
