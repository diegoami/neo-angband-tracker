# Remove the dead stun test from the HURT blow danger

This is **hurtstun-62 / issue #62**, a C Borg defect
(diegoami/neo-angband-tracker#62).

Cleanup, no behaviour change; owner decision (2026-11-08). When the Borg rates how dangerous a monster's melee is, borg_danger_physical() handles each HURT blow by setting z to dice x sides and then testing z > dice x sides for a +200 'stun' penalty. The two sides are the same expression, so the test is never true. The next test (fewer than 3 sides and more than 5 dice, +400) is live and already covers the knock-out kicks the comment describes; it is kept. The change deletes the dead test.

## What to watch

Borg_danger() at the Borg's grid with the monster adjacent; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `f1aca2e9a`) |
| --- | --- | --- |
| Control: mystic, 4 x KICK:HURT:10d2 (kept +400 test fires) | passed, danger=1680 | byte-identical log |
| Control: soldier ant, BITE:HURT:1d2 (removed test evaluated, +400 not taken) | passed, danger=2 | byte-identical log |
| Control: cave orc, HIT:HURT:1d8 (ordinary HURT blow) | passed, danger=8 | byte-identical log |
| Control: hill giant, 2 x HIT:HURT:4d8 (ordinary HURT blows) | passed, danger=64 | byte-identical log |

- **Control: mystic, 4 x KICK:HURT:10d2 (kept +400 test fires).** Each 10d2 kick has fewer than 3 sides, so the original evaluates the removed test (false: z equals dice x sides) and then takes the kept +400. Danger 1604 partial / 1680 full (4 x 400 plus the kick damage) in both versions. Logs byte-identical.

- **Control: soldier ant, BITE:HURT:1d2 (removed test evaluated, +400 not taken).** A 1d2 bite has fewer than 3 sides, so the original evaluates the removed test (false); with one die the +400 test is not taken either. Danger 2 in both versions. Logs byte-identical.

- **Control: cave orc, HIT:HURT:1d8 (ordinary HURT blow).** An ordinary HURT blow: the removed test short-circuits on d_side < 3. Danger 8 in both versions. Logs byte-identical.

- **Control: hill giant, 2 x HIT:HURT:4d8 (ordinary HURT blows).** Two ordinary HURT blows. Danger 64 in both versions. Logs byte-identical.

## Conditions

Native Human Warrior, seed 62, clevel 31, depth 30, full HP (371), hasted (TMD_FAST, speed 120), a torch and rations, in a lit, open 24x40 arena with one awake monster adjacent (east), observed through observe_kill_diff(). The production borg_danger_one_kill() (partial and full damage) and borg_danger() at the Borg's grid run and are printed. Modes: 'mystic' (4 x KICK:HURT:10d2: both the removed test's d_side < 3 half and the kept +400 test fire), 'ant' (soldier ant, BITE:HURT:1d2: the removed test is evaluated, the +400 test is not taken), 'orc' (cave orc, HIT:HURT:1d8) and 'giant' (hill giant, 2 x HIT:HURT:4d8): ordinary HURT blows. Fixture interventions: the arena and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_VIEW, borg.avoidance is set to current HP, haste is set directly (so the mystic is rated below the 2000 danger cap and the +400 terms show), and borg_kills[k].awake is set by hand.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/hurtstun-62
git fetch https://github.com/diegoami/neo-angband-tracker.git task/hurtstun-62-evidence
git show 2cb17f80146015dc9fc162f45b50cf5ba88ae11f:docs/audit/c-reports/hurtstun-62/reproduce.py > /tmp/reproduce-hurtstun-62.py
python3 /tmp/reproduce-hurtstun-62.py --full-tests --fixtures-ref 2cb17f80146015dc9fc162f45b50cf5ba88ae11f --fix-ref f1aca2e9a3d4e6ffe7e301c9aa750173dd3883bd
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-danger.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `f1aca2e9a3d4e6ffe7e301c9aa750173dd3883bd` (branch `task/hurtstun-62`), fixtures
  `2cb17f80146015dc9fc162f45b50cf5ba88ae11f` (branch `task/hurtstun-62-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: mystic, ant, orc, giant
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The deleted test in the HURT case of borg_danger_physical() compares z with d_dice * d_side immediately after z = d_dice * d_side, with no change to either in between, so it is never true and its +200 never runs. Four native scenarios with production borg_danger_one_kill() and borg_danger(), one with many-dice low-sides kicks (mystic, where the kept +400 test fires and the removed test is evaluated), one with a 1d2 bite (removed test evaluated, +400 not taken) and two with ordinary HURT blows, give byte-identical logs before and after, and both full native gates pass on both builds.

This is a cleanup with no failing mode: the scenarios show unchanged behaviour, they cannot show a difference. That the test is never true is argued from the source. One adjacent monster per scenario; whole-game runs are not measured. Reviving the stun penalty with a different condition (owner option 2 on the issue) is not addressed.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
