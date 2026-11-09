# Constitution-drain blows were feared by strength, not constitution

This is **losecon-61 / issue61**, a C Borg defect
(diegoami/neo-angband-tracker#61).

The Borg rates a monster blow that drains constitution as extra dangerous when its constitution is below 8 (the code comment says 'extra scary to have con drain below 8'). The check read strength instead. A level-49 Warrior on 500 hit points, constitution drained to 7, standing next to a purple mushroom patch (three constitution-draining spores) therefore rated the patch at 456 danger and stayed. A Warrior with healthy constitution but strength drained to 7 rated the same patch at 756 and read a Scroll of Teleportation to get away from a threat to a stat it was not short of. After the fix the low-constitution Warrior rates the patch at 756 and teleports away, and the low-strength Warrior rates it at 456 and keeps its scroll.

## What to watch

Escaped (1 = read a Scroll of Teleportation, '# Danger Level 2.1'); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `0ea55f39b`) |
| --- | --- | --- |
| Constitution drained to 7, strength 17 | failed, escape=0 | passed, escape=1 |
| Strength drained to 7, constitution 16 | failed, escape=1 | passed, escape=0 |
| Control: strength 17, constitution 16 | passed, escape=0 | byte-identical log |
| Control: strength and constitution both drained to 7 | passed, escape=1 | byte-identical log |

- **Constitution drained to 7, strength 17.** Original: danger 456 (no extra fear, strength is 17), below the 500 hit points; the Borg does not escape. Corrected: danger 756; it reads a Scroll of Teleportation ('# Danger Level 2.1').

- **Strength drained to 7, constitution 16.** Original: danger 756 because strength is below 8; the Borg reads a Scroll of Teleportation. Corrected: danger 456; it keeps the scroll.

- **Control: strength 17, constitution 16.** Neither stat below 8: danger 456 and no escape in both versions; logs identical.

- **Control: strength and constitution both drained to 7.** Both stats below 8, so the extra fear applies either way: danger 756 and a Teleportation read in both versions; logs identical.

## Conditions

Native Human Warrior, seed 61, level 49, depth 30, a lit arena, starting kit plus three known Scrolls of Teleportation, 500 current hit points (avoidance 500), an awake purple mushroom patch adjacent (east). The mode sets the current (drained) value of strength and/or constitution to 7 while the maximum stays at its birth value; nothing else differs. The production borg_caution() runs once and the first queued key is read back. This isolates the danger estimate and the escape decision; it does not play out the following turns or measure actual stat loss.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/losecon-61
git fetch https://github.com/diegoami/neo-angband-tracker.git task/losecon-61-evidence
git show 4ba84acc1dff95dc31abf3b2254a3b0ebcd624f3:docs/audit/c-reports/losecon-61/reproduce.py > /tmp/reproduce-losecon-61.py
python3 /tmp/reproduce-losecon-61.py --full-tests --fixtures-ref 4ba84acc1dff95dc31abf3b2254a3b0ebcd624f3 --fix-ref 0ea55f39b583c1e3d113c7834e8e9f237f5cbf7d
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-danger.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `0ea55f39b583c1e3d113c7834e8e9f237f5cbf7d` (branch `task/losecon-61`), fixtures
  `4ba84acc1dff95dc31abf3b2254a3b0ebcd624f3` (branch `task/losecon-61-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: neither, bothlow
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The constitution-drain blow case in borg_danger_physical() tested BI_CSTR < 8 for its 'con drain below 8' fear. With BI_CCON, a level-49 Warrior on 500 HP with constitution 7 beside a purple mushroom patch rates it at 756 instead of 456 and reads Teleportation; with strength 7 and healthy constitution it rates it at 456 instead of 756 and keeps the scroll; both controls are byte-identical.

One synthetic fixture with an injected stat drain, one monster and one decision call; the following turns and the actual constitution loss are not played out. Whether teleporting from a stationary mushroom patch is the best response is the existing escape policy, not changed here; the fix only changes which stat gates the extra fear.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
