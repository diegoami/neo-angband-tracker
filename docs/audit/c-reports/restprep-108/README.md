# The Borg rested straight after a prep spell once it reached character level 33

This is **restprep-108 / issue108**, a C Borg defect
(diegoami/neo-angband-tracker#108).

Before it takes stairs the Borg casts preparatory spells such as Protection from Evil, and sets a timer that keeps it from resting so the spell is not wasted. For Protection from Evil, and for several spells in the fight and escape code, the timer is set to character level times 1000. The timer was a 16-bit integer, which holds up to 32767, so from character level 33 the value wrapped negative and the 'do not rest just after a prep spell' tests treated it as already expired. The correction makes the timer an int.

## What to watch

The Borg may rest right after the prep spell (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `9d1a7f1d2`) |
| --- | --- | --- |
| Level-35 Priest after casting Protection from Evil | failed, rests_after_prep=1 | passed, rests_after_prep=0 |
| Control: level-30 Priest after casting Protection from Evil | passed, rests_after_prep=0 | byte-identical log |

- **Level-35 Priest after casting Protection from Evil.** A level-35 Priest at full hit points and spell points, with Protection from Evil learned, prepares to leave the level. The production prep routine chooses Protection from Evil and sets the no-rest timer to level times 1000. Original: the 16-bit timer wraps to -30536, the "no resting just after a prep spell" test does not apply, and borg_check_rest() allows resting. Corrected: the timer is 35000 and resting is refused.

- **Control: level-30 Priest after casting Protection from Evil.** The same Priest at level 30. The timer is 30000, which fits in 16 bits, so resting is refused before and after.

## Conditions

Native Human Priest, seed 108, character level 35 (control: 30), full hit points and spell points, depth 5, a lit open arena, no monsters, a lit torch, rations and the book [Cleansing Power]. Fixture interventions: the Priest is marked as having learned and cast Protection from Evil (and no other prayer, so the earlier prep spells are not legal); the Borg's map update is not run, so the cast is queued, not played. The production borg_prep_leave_level_spells() chooses and queues the cast and the production borg_check_rest() gives the resting decision.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/restprep-108
git fetch https://github.com/diegoami/neo-angband-tracker.git task/restprep-108-evidence
git show 0653738eeaa460db189cc3126e8a791705db613d:docs/audit/c-reports/restprep-108/reproduce.py > /tmp/reproduce-restprep-108.py
python3 /tmp/reproduce-restprep-108.py --full-tests --fixtures-ref 0653738eeaa460db189cc3126e8a791705db613d --fix-ref 9d1a7f1d2a9b14a60261ce01381e79d940ef7532
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg.h`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `9d1a7f1d2a9b14a60261ce01381e79d940ef7532` (branch `task/restprep-108`), fixtures
  `0653738eeaa460db189cc3126e8a791705db613d` (branch `task/restprep-108-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: clevel30
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

At character level 35 the original timer after Protection from Evil was -30536, the resting check did not see a recent prep spell and the Borg was allowed to rest. With an int timer the value is 35000 and resting is refused, as at level 30 (30000, unchanged).

Priority low to medium: this measures the timer value and the resting check right after the cast, not how long the spell lasts, a rest in play or its cost. The timer is also set to level times 1000 in the fight and escape code, which share the correction; their decisions are not measured. Other timers set in the same code (up to 21000) already fit in 16 bits.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
