# A shape-changed monster made the Borg think Grip was dead

This is **uniquedead-94 / issue #94**, a C Borg defect
(diegoami/neo-angband-tracker#94).

The Borg keeps a table of which uniques are dead. When it looks at a monster whose race is a unique the player has already killed, a fix-up marks that unique as dead. The fix-up used the number of the Borg's own monster record instead of the monster's race number, so it marked an unrelated race as dead, permanently for the session. This can happen in normal play: a Maia of Aulë can change into the shape of Quaker, Master of Earth, even after the player killed Quaker. In the fixture the Borg had 46 other monster records, so the Maia's record was number 47, which is the race number of Grip, Farmer Maggot's Dog, still alive. On the next level the Borg's living-unique list lost Grip, the depth of its third-shallowest living unique jumped from 17 to 41, and when it read Word of Recall at depth 15 (maximum depth 60) it answered 'n' to 'Set recall depth to current depth?', so the next recall returns to depth 60. The corrected Borg marks Quaker (the race it saw), keeps Grip alive and answers 'y', as it does without the bug.

## What to watch

Answers 'y' to 'Set recall depth to current depth?' (1 = yes, recall depth becomes 15); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `ea3e72139`) |
| --- | --- | --- |
| Maia in Quaker's shape becomes record 47 (Grip's race number) | failed, reset_recall=0 | passed, reset_recall=1 |
| Control: the same sighting becomes record 5 (a non-unique race) | passed, reset_recall=1 | byte-identical log |

- **Maia in Quaker's shape becomes record 47 (Grip's race number).** Original: the fix-up marks race 47 (Grip, alive) dead instead of Quaker; on the next level the Borg counts 56 living uniques instead of 57, its first living unique becomes Fang and its hunted depth 41 instead of 17; it answers 'n' and the recall depth stays 60. Corrected: Quaker is marked, Grip stays alive, hunted depth 17, the Borg answers 'y' and the game sets recall depth (and maximum depth) to 15.

- **Control: the same sighting becomes record 5 (a non-unique race).** With only four earlier records the Maia's record is 5 (blubbering idiot, not a unique), so the misplaced mark has no visible effect; both versions keep Grip alive and answer 'y'. The changed line runs in both; the logs are byte-identical.

## Conditions

Native Human Warrior, seed 94, character level 31, depth 15, maximum and recall depth 60, a lit 38x38 room, one Scroll of Word of Recall. Fixture interventions: the character's unique kills are recorded as the game records them (r_info max_num = 0 and a lore kill, mon-util.c:1101, load.c:533-535): every unique shallower than depth 40 except Grip, Fang and Nár, plus Quaker; the Borg's map grids are marked on-screen/in view. Real code: borg_init(), the first borg_update() (whose new-level branch runs the unique scan), 46 stock wild dogs (4 in the control) each placed and observed with the production observe_kill_diff(); a stock Maia of Aulë placed and changed with the game's monster_change_shape() until it takes Quaker's shape (one try with this seed); its first sighting through observe_kill_diff() -> borg_new_kill() -> borg_update_kill_old(); a second borg_update() on a new level (borg.status.old_depth changed, as on arrival); the production borg_recall(); the Borg's queued answer is given to the game's own 'Set recall depth to current depth?' question while the real read-scroll command runs.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/uniquedead-94
git fetch https://github.com/diegoami/neo-angband-tracker.git task/uniquedead-94-evidence
git show cf21e7cf57d9d282173d8d5e0844fae2153dc04b:docs/audit/c-reports/uniquedead-94/reproduce.py > /tmp/reproduce-uniquedead-94.py
python3 /tmp/reproduce-uniquedead-94.py --full-tests --fixtures-ref cf21e7cf57d9d282173d8d5e0844fae2153dc04b --fix-ref ea3e7213955bfa1125b84ded987b0adee56c0ee3
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-kill.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `ea3e7213955bfa1125b84ded987b0adee56c0ee3` (branch `task/uniquedead-94`), fixtures
  `cf21e7cf57d9d282173d8d5e0844fae2153dc04b` (branch `task/uniquedead-94-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: slot5
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_update_kill_old() marked borg_race_death[i] with the Borg's monster-record index i instead of the race index when it saw a monster whose race is an already-killed unique. A Maia of Aulë in the shape of the dead Quaker, seen as record 47, marked Grip (race 47, alive) dead for the rest of the session; the Borg's living-unique list then lost Grip and its recall-depth answer at depth 15 changed from 'y' (recall depth 15) to 'n' (recall depth 60). Using kill->r_idx marks the race actually seen. The record-5 control is byte-identical.

Requires a monster seen in the shape of a unique the player has killed (stock: Maia of Aulë -> Quaker, Maia of Oromë -> Huan) or a stale record matched to a reused monster index, and a record index equal to the race index of a living unique (stock uniques with race index below 255 are the 22 shallowest, depth 21 or less). How often that happens in real games is not measured. The record index is reached with 46 observed stock monsters. Other consumers of the wrong mark (borg_prepared()'s depth gate with borg_kills_uniques, which the stock profile leaves off, and the depth-98 Morgoth readiness check) were read, not measured. The write itself is in bounds (index below 256, table size r_max), so no sanitizer variant applies. Whether resetting the recall depth to 15 is the better play is not claimed; the defect is that the Borg's own rule is fed a wrong list of living uniques.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
