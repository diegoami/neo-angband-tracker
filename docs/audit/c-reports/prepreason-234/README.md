# The next depth's reason hid 'Collect from house'

This is **prepreason-234 / issue #234**, a C Borg defect
(diegoami/neo-angband-tracker#234).

When the Borg decides whether to leave a level, it asks borg_prepared() why it is not ready for the current depth and for the next one. Every formatted reason is written into the same static text buffer, so asking about the next depth overwrote the current depth's answer before it was used. At depth 99 with Potions of *Healing* waiting at home, the current depth's reason is 'Collect from house (20 potions).', which should send the Borg back to town to fetch them. With borg_no_deeper set to 100 (stop before Morgoth's level), the next depth's reason is 'No deeper 100.', and that text replaced it: the Borg printed the wrong reason, did not decide to collect, and walked to an up staircase instead of reading Word of Recall. The correction copies the current depth's reason before the next call; the Borg then recalls to town to collect.

## What to watch

Decided to collect from the house (1 = 'Returning to town to Collect stock'); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `52792d3e5`) |
| --- | --- | --- |
| Depth 99, borg_no_deeper = 100: current 'Collect from house', next 'No deeper 100.' | failed, collect=0 | passed, collect=1 |
| Control: depth 99, borg_no_deeper = 127: both depths say 'Collect from house' | passed, collect=1 | byte-identical log |
| Control: depth 97 with 30 *Healing* carried: current depth prepared | passed, collect=0 | byte-identical log |

- **Depth 99, borg_no_deeper = 100: current 'Collect from house', next 'No deeper 100.'.** Original: the depth-100 call overwrites the buffer, so the notes say 'not prep for current level: No deeper 100.', the 'Collect from house' test fails, and the Borg flows toward the up staircase (key 6). Corrected: the notes give 'Collect from house (20 potions).', the Borg decides 'Returning to town to Collect stock.' and reads Word of Recall.

- **Control: depth 99, borg_no_deeper = 127: both depths say 'Collect from house'.** The next depth gives the same text, so the overwrite changes nothing; both versions decide to collect and read Word of Recall, and the logs are identical.

- **Control: depth 97 with 30 *Healing* carried: current depth prepared.** No reason for the current depth; neither version takes the collect branch (both power-dive), and the logs are identical.

## Conditions

Native Human Warrior, seed 234, character level 50, maximum depth 99, at depth 99 ('nodeeper100', 'default127') or 97 ('prepared97'), in a lit 40x24 arena with one up staircase 25 steps east that the Borg has seen. Endgame kit from real game items with every rune known: 'Bladeturner' (all resists, blind, confusion, hold life), 'Dal-i-thalion' (free action, +5 speed), an Amulet of ESP, two Rings of Speed, a Mace of Disruption, a Lantern; carried: food, oil, Word of Recall x4, Teleportation x8, Teleport Level x3, Phase Door x10, Healing x5, Speed x15, and *Healing* x15 (x30 in 'prepared97'). The home holds 20 Potions of *Healing*. Fixture interventions, disclosed: STR, DEX and CON set to 18/100; the Borg's clock set so it has just arrived on the level and was last in town 5000 game turns ago; borg_cfg[BORG_NO_DEEPER] set to 100 (the same as the borg.txt line 'borg_no_deeper = 100') except in 'default127', which keeps the profile's 127. The production borg_leave_level(false) runs once; its notes, goal and first queued keys are read back. This isolates the leave-level decision; it does not play the recall or the visit home.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/prepreason-234
git fetch https://github.com/diegoami/neo-angband-tracker.git task/prepreason-234-evidence
git show f1443b56915456ec9991d5f103a91143832e20dc:docs/audit/c-reports/prepreason-234/reproduce.py > /tmp/reproduce-prepreason-234.py
python3 /tmp/reproduce-prepreason-234.py --full-tests --fixtures-ref f1443b56915456ec9991d5f103a91143832e20dc --fix-ref 52792d3e51f76efeb7c649905a6de7fa9d626162
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-think-dungeon-util.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `52792d3e51f76efeb7c649905a6de7fa9d626162` (branch `task/prepreason-234`), fixtures
  `f1443b56915456ec9991d5f103a91143832e20dc` (branch `task/prepreason-234-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: default127, prepared97
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_leave_level() kept the pointer borg_prepared() returned for the current depth while later borg_prepared() calls rewrote the same static buffer. A level-50 Warrior at depth 99 with 20 Potions of *Healing* at home and borg_no_deeper = 100 printed 'No deeper 100.' as its reason for the current level and skipped the 'Collect from house' return, walking to an up staircase instead of reading Word of Recall. Copying the reason first restores the decision; the same-reason and prepared controls are byte-identical.

Not measured: what happens after the missed decision. On the next level up (98) the next depth is 99, below borg_no_deeper, so both depths give 'Collect from house' again and the original Borg would most likely decide to collect there: the measured harm is one level of delay (and one extra stair trip at depth 99) rather than a lost trip. The flip needs the current depth to say 'Collect from house' while a later call writes a different formatted reason, which in this source needs borg_no_deeper at or just above the current depth (or depth + 5 on the power-dive branch); with the default borg_no_deeper = 127 the texts match and only the notes are affected when the depths differ in other formatted reasons. How often players set borg_no_deeper this low was not measured. The other affected notes ('Returning to town (too deep)', 'Climbing (too deep)') print the corrected text in the same run.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
