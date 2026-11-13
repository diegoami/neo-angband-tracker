# A hurt Borg retreated through a grid next to a faster monster

This is **retreatadj-154 / issue154** (CBUG-0138), a C Borg defect
(diegoami/neo-angband-tracker#154).

When a monster makes its grid too dangerous, the hurt Borg looks for a *strategic retreat*: a safer grid in view and a short path to it. For the first step of each route it means to skip a grid next to a monster faster than itself, because ("He will just hit me when I land on that grid") that monster gets to attack as it lands. The check found such a monster but its `break` only left the monster scan, so the route was accepted anyway. The correction records the match and skips that route.

## What to watch

The retreat's first step is next to the side monster (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `3b0a94673`) |
| --- | --- | --- |
| Faster cave spider beside the retreat route | failed: "# Retreating to 12,15 … via 12,15 (24 > 8)", the Borg steps next to the spider | passed: that route is skipped; no retreat this turn |
| Control: soldier ant (not faster) beside the route | passed: same retreat | byte-identical log |

In both cases the step's own danger (8 with the spider, 2 with the ant) is below the first-step limits (40% of hit points, half the current danger), so only the faster-monster check can reject it — which is why this is not masked here.

## Conditions

Native Human Warrior, seed 154, character level 20, 40 of 259 hit points, dungeon level 5, a torch, in a lit one-wide east–west corridor with one alcove. An awake **cave bear** (normal speed) stands next to the Borg on the west (current danger 24). In the alcove at (16,11), diagonally next to the first step east (15,12), stands a **cave spider** (speed +10, a 1d4 bite) or, in the control, a **soldier ant** (normal speed, a 1d2 bite). Fixture interventions: the map is built directly and recorded in the Borg's map; both monsters enter the Borg's monster list through the production `observe_kill_diff()`; hit points set to 40; level timers set so the retreat is allowed. The production `borg_caution()` runs once; the step is not played.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/retreatadj-154
git fetch https://github.com/diegoami/neo-angband-tracker.git task/retreatadj-154-evidence
git show f87cada6322bc2108c44b86a4b14a070fe7f26e1:docs/audit/c-reports/retreatadj-154/reproduce.py > /tmp/reproduce-retreatadj-154.py
python3 /tmp/reproduce-retreatadj-154.py --full-tests --fixtures-ref f87cada6322bc2108c44b86a4b14a070fe7f26e1 --fix-ref 3b0a9467344b461a1d0177877dc190b17aee08e4
```

The runner asserts the correction is exactly the three expected hunks in
`src/borg/borg-caution.c`, builds the original and corrected native C game in
a disposable worktree, runs both scenarios, checks the expected results and the
byte-identical control, runs both full native gates and writes
`replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`. The
frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `3b0a9467344b461a1d0177877dc190b17aee08e4` (branch `task/retreatadj-154`), fixtures `f87cada6322bc2108c44b86a4b14a070fe7f26e1` (branch `task/retreatadj-154-evidence`); later commits only add these artifacts.
- control byte-identical: ant
- full native gates: before and after 965/965 unit checks and 5/5 frontend tests
- corrected source sha256 (`src/borg/borg-caution.c`): `2b256df39d56ef5ecd7c4388cc94d6af887e241d29de46fd02e22e89c0ce26ef`
- replay: Playwright Chromium headless render check (every option and visible button), no console errors

## What this proves, and what it does not

The strategic retreat's faster-monster test never rejected a step: with a faster cave spider beside the only route, the original Borg retreated into the grid next to it. With the correction that route is skipped; with a monster that is not faster, nothing changes.

Not measured: what the Borg does instead on the following turns (here `borg_caution()` takes no action this turn; fighting, escapes and back-away remain), the fight, deaths, or how often a weak faster monster stands beside the only retreat route. Earlier work on #155 suspected this check was masked by the first-step danger limits; it is masked whenever the faster monster's danger at the step exceeds those limits, and live when it is weak, as here.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+ `profile/borg.txt`) are the generators; `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file) are the captured run.
