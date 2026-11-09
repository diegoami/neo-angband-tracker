# One skipped monster stopped the Borg hunting any monster

This is **flowkillhall-98 / issue98**, a C Borg defect
(diegoami/neo-angband-tracker#98).

When the Borg looks for a monster to walk up to and fight, it goes through every monster it knows about and drops the unsuitable ones. The note that a monster should be dropped was never cleared for the next monster, so once one monster was dropped every monster after it in the list was dropped too. A level-20 Warrior with one soldier ant two steps away (dropped by design: stepping closer lets it bite first) and a second soldier ant in plain view five steps away hunted neither and stood still. A second check, meant to keep the Borg in a corridor rather than chase a pack monster out in an open room, counted walls on top of the Borg's own count and so never fired; the Borg left its corridor for a wild dog in the open room. The correction judges each monster on its own: the Borg walks toward the second ant, and stays in its corridor instead of chasing the dog.

## What to watch

Sets off toward a monster to fight it (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `69862d0f5`) |
| --- | --- | --- |
| Open room: ant 2 steps east noted first, ant 5 steps south second | failed, flowed=0 | passed, flowed=1 |
| Control: the same two ants, far one noted first | passed, flowed=1 | byte-identical log |
| Borg in a corridor, wild dog (pack monster) in the open room | failed, flowed=1 | passed, flowed=0 |
| Control: Borg and wild dog both in the corridor | passed, flowed=1 | byte-identical log |

- **Open room: ant 2 steps east noted first, ant 5 steps south second.** Original: the near ant is dropped (two steps away, melee only), the drop note is kept, the far ant is dropped too; no target, the Borg does not move. Corrected: the far ant at (10,17) is the target and the Borg steps south (key 50).

- **Control: the same two ants, far one noted first.** The far ant is judged before the near one sets the note; both versions target it and step south. Logs are byte-identical.

- **Borg in a corridor, wild dog (pack monster) in the open room.** Original: the dog's wall count continues from the Borg's own six corridor walls and the 'fewer than four walls' test never fires; the Borg steps east out of the corridor toward the dog (key 54). Corrected: the dog has no walls around it, it is dropped, and the Borg stays put.

- **Control: Borg and wild dog both in the corridor.** The dog has six walls around it, so both versions keep it and step west toward it (key 52). Logs are byte-identical.

## Conditions

Native Human Warrior, seed 98, level 20, depth 5, full hit points (247), torch and rations, a lit 38x22 room. Corridor cases fill columns 1-14 with granite except row 12, so the Borg at (10,12) stands in a one-wide corridor that opens into the room at column 15. Monsters are placed with t_add_monster (single monsters; the wild dog's pack is not generated) and noted by the Borg's own observe_kill_diff in the stated order, which fixes their order in the Borg's monster list. The production borg_flow_kill(true, 250) runs once and the first queued key is read back. This isolates the hunt decision; it does not play out the walk or the fight.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/flowkillhall-98
git fetch https://github.com/diegoami/neo-angband-tracker.git task/flowkillhall-98-evidence
git show 8ca38b4bba96a8d03d234b6d473648e84d91059f:docs/audit/c-reports/flowkillhall-98/reproduce.py > /tmp/reproduce-flowkillhall-98.py
python3 /tmp/reproduce-flowkillhall-98.py --full-tests --fixtures-ref 8ca38b4bba96a8d03d234b6d473648e84d91059f --fix-ref 69862d0f5e2a860d63281df9157d77951875adf3
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-kill.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `69862d0f5e2a860d63281df9157d77951875adf3` (branch `task/flowkillhall-98`), fixtures
  `8ca38b4bba96a8d03d234b6d473648e84d91059f` (branch `task/flowkillhall-98-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: reverse, hallctl
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_flow_kill() sets skip_monster and hall_walls once per call instead of once per monster. Once one monster is skipped every later monster is skipped too (a level-20 Warrior with a two-step soldier ant noted first does not hunt a second ant in plain view), and the pack-monster wall count runs on from the Borg's own count with the test inside the counting loop, so it never fires (the Borg leaves a corridor for a wild dog in an open room). Resetting both per monster and testing the count after all nine grids restores both decisions; two controls are byte-identical.

Measured: the single hunt decision (target list, first step key) in four fixture situations. Not measured: whole-game survival or kill rates. Fixture interventions: arena and granite layout written directly, Borg map copied from the game map with every grid marked known and in view, monsters placed singly and marked awake in the Borg's list, borg.avoidance set to current hit points, no stairs tracked. The corridor threshold (fewer than four walls around the monster) is the existing one; the correction only makes it count the monster's own grids. The corrected corridor rule makes the Borg more cautious with pack monsters in rooms, which is what its comment describes; its long-run effect on play was not measured.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
