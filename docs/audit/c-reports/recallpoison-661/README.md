# Active poison damage is omitted from the Word of Recall decision

This is **recallpoison-661 / issue #661**, a C Borg defect
(diegoami/neo-angband-tracker#661).

A poisoned Borg can start Word of Recall or keep resting for it even when the poison already in its system can kill it before Recall activates. The borg keeps the poison timer only as a boolean; the rest-for-Recall branch checks the location but never compares the poison timer with the hit points; and the engine deals one point of poison damage per world turn, before the Recall countdown. While fleeing - exactly the Recall-wait situation - the heal cascade is skipped, so the rest branch won every turn: the recorded native case died at 7 HP with nine Cure Critical Wounds potions and Herbal Curing available.

## What to watch

1 = the borg rested for Recall (a poison tick every turn while it waits), 0 = it spent the turn otherwise; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `574b618c3`) | Corrected (fix `1128bd316`) |
| --- | --- | --- |
| Lethal poison with a cure in reach (the recorded shape) | failed, rested=1 | passed, rested=0 |
| Lethal poison with no cure at all | failed, rested=1 | passed, rested=0 |
| The exact edge: pending damage reaches HP on the last tick | failed, rested=1 | passed, rested=0 |
| Control: the poison cannot kill before lift-off | passed, rested=1 | byte-identical log |
| Control: no poison at all | passed, rested=1 | byte-identical log |

- **Lethal poison with a cure in reach (the recorded shape).** 7 HP, a 40-turn poison timer, Recall pending (word_recall 20), one Cure Critical Wounds potion, a quiet square. The original rests for Recall ('R 500') - a poison tick every turn, dead before lift-off. The corrected borg weighs the pending poison (one damage point per world turn, timer lowered by adj_con_fix(CON)+1, lift-off after word_recall turns) and spends the turn curing ('q') instead.

- **Lethal poison with no cure at all.** The same lethal state with nothing that cures. The original still rests into the poison; the corrected borg does not rest - no cure to quaff, so the regular flows keep it moving instead of waiting for the poison to kill it.

- **The exact edge: pending damage reaches HP on the last tick.** 8 HP, an 8-turn effective poison, Recall pending: the pending poison reaches the current HP on the last tick before lift-off, and the engine processes poison before the Recall countdown. The original rests; the corrected borg cures.

- **Control: the poison cannot kill before lift-off.** 20 HP, an 8-turn effective poison, Recall pending: the pending damage stays below the current HP, so resting for Recall is the right decision. Byte-identical before/after.

- **Control: no poison at all.** 7 HP, no poison, Recall pending: the plain wait-for-Recall rest. Byte-identical before/after.

## Conditions

Native Human Warrior, level 20 (experience forced), depth 5 in a synthetic open lit arena, a wooden torch as the only light, no monsters: a quiet square (danger 0). The recorded state per mode: fixed low HP, an active poison timer set through the production player_set_timed(), a Recall already counting down (player->word_recall 20, borg.goal.recalling populated by the production update), and borg.goal.fleeing set - the fleeing state is what skips the standing heal cascade and makes the rest-for-Recall branch the winning decision, exactly the recorded death path. The measured decision is the production borg_caution() itself, observed from the keys it queues ('R 500' = resting for Recall, 'q' = quaffing the cure). The paranoia roll inside the heal cascade is pinned by seeding before the decision.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/recallpoison-661
git fetch https://github.com/diegoami/neo-angband-tracker.git task/recallpoison-661-evidence
git show 1f68d122337bab4bd78a5d43cf529f188cf34480:docs/audit/c-reports/recallpoison-661/reproduce.py > /tmp/reproduce-recallpoison-661.py
python3 /tmp/reproduce-recallpoison-661.py --full-tests --fixtures-ref 1f68d122337bab4bd78a5d43cf529f188cf34480 --fix-ref 1128bd316f0a62a840e08134cdb8112464f849a8
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-caution.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix `1128bd316f0a62a840e08134cdb8112464f849a8` (branch `task/recallpoison-661`), fixtures
  `1f68d122337bab4bd78a5d43cf529f188cf34480` (branch `task/recallpoison-661-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: survivable, unpoisoned
- full native gates: before 966/966 unit checks and 5/5 frontend tests;
  after 966/966 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

Waiting for Recall no longer rests into a poison that can kill before lift-off: the rest-for-Recall branch now requires the pending poison damage (engine math: one point per world turn, timer lowered by adj_con_fix(CON)+1, lift-off after word_recall turns) to stay below the current HP, and when it does not, the production heal cascade spends the turn curing instead - with nine Cure Critical Wounds potions in reach, the recorded D0017 death now quaffs instead of resting. Waiting with a survivable poison, or with no poison, still rests exactly as before (byte-identical controls).

The fixture measures the wait-for-Recall decision after activation (the resting path that killed the recorded case). The decision to start Recall while poisoned is not changed: starting Recall is safe once the wait cures instead of resting, because the borg keeps getting turns. The borg's own damage-reduction state is deliberately not modelled - the pending poison is priced at one point per tick, which is the conservative reading. HP regeneration while waiting is likewise not credited.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
