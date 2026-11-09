# Escaping up a staircase, the Borg also read a scroll on the next level

This is **stairphase-34 / issue #34**, a C Borg defect
(diegoami/neo-angband-tracker#34).

When the Borg is in some danger while standing on an up staircase, its escape routine is meant to just take the stairs. In five of its escape steps the stair check was written as one half of the condition for a Phase Door: the check queues the climb key ('<') at once, and the rest of the condition still runs. A level-16 Warrior on an up staircase at the end of a corridor, next to a hill giant, queued the climb and then a Phase Door read. The game took the stairs, and then played the leftover keys on the new level: before the Borg had looked at that level, it read a Phase Door and was moved off the staircase it had arrived on. When a phase was blocked (the Borg had stood in an anti-summon spot ten turns earlier), the climb key stayed queued and the routine went on to queue a Teleportation read, which was then spent on the new level. The correction makes the stair escape a complete choice, as the two most dangerous escape steps already do: the Borg queues only '<', arrives on the new level and keeps its scrolls.

## What to watch

Scroll read on the new level from keys queued on the old one (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `2176cf71c`) |
| --- | --- | --- |
| Up staircase, Phase Door possible | failed, scroll_read_on_new_level=1 | passed, scroll_read_on_new_level=0 |
| Up staircase, phase blocked by a recent anti-summon spot | failed, scroll_read_on_new_level=1 | passed, scroll_read_on_new_level=0 |
| Control: down staircase (the stair check fails, the phase is chosen) | passed, scroll_read_on_new_level=0 | byte-identical log |
| Control: plain floor (the stair check fails, the phase is chosen) | passed, scroll_read_on_new_level=0 | byte-identical log |
| Control: up staircase, more hurt (climbing is the only choice) | passed, scroll_read_on_new_level=0 | byte-identical log |

- **Up staircase, Phase Door possible.** Original: keys '<', Esc, Esc, 'r', 'd' (Danger Level 4.1). The game climbs to depth 9, the Borg arrives on a down staircase at (59,28), then reads a Phase Door there and lands at (63,36), off the staircase, with 4 scrolls left. Corrected: only '<' is queued; the Borg arrives at (59,28) on the staircase with all 5 scrolls.

- **Up staircase, phase blocked by a recent anti-summon spot.** Original: '<' is queued by the stair check, the phase is refused by the anti-summon timer, and the routine goes on to Danger Level 4.2 and queues a Teleportation read after the '<'. The game climbs and the Borg reads Teleportation on the new level (2 to 1 scrolls). Corrected: the stair escape is final; only '<' is queued and no scroll is read.

- **Control: down staircase (the stair check fails, the phase is chosen).** The changed condition runs, the stair check returns false and the Borg reads a Phase Door on the current level in both versions; the logs are byte-identical.

- **Control: plain floor (the stair check fails, the phase is chosen).** The same without any staircase: both versions read a Phase Door on depth 10; the logs are byte-identical.

- **Control: up staircase, more hurt (climbing is the only choice).** At 117 hit points the danger is above the hit points and the earlier, unchanged escape step (Danger Level 2.1) takes the stairs alone in both versions; the logs are byte-identical. This control does not reach the changed lines.

## Conditions

Native Human Warrior, seed 34, character level 16, depth 10, 5 Scrolls of Phase Door, 2 Scrolls of Teleportation, 5 Potions of Cure Light Wounds, a lit wooden torch and food. A lit one-wide corridor of 26 grids runs east from a dead end in solid granite; the Borg stands on the dead end (an up staircase, a down staircase or floor, by case) with an awake hill giant on the next grid. The Borg's danger estimate for its grid is 147; the player is set to 172 of 212 hit points (147 is about 85% of 172; 117 hit points in the climb control). Real code: the production borg_caution(), run once, which reaches borg_escape() with the danger of the Borg's grid; then the Borg's whole key queue is played through the game's own text UI (textui_process_command(), reading each key through the Borg's keypress hook) and the game loop (run_game_loop()), including the real level change and the generation of the new level. Fixture interventions: the level, the staircase and the monster are placed directly and the Borg's map is filled from the game's map; the pack is set as listed and sorted by the game; hit points are set as above; the Borg's clock is set to 5000 game turns with the level and town entered 1000 turns earlier, and in the antisummon case the Borg's anti-summon timer is set to ten turns ago, as borg_flow does when it stands in an anti-summon spot. Game messages are recorded with an EVENT_MESSAGE handler and are not shown to the Borg (no message flushes the Borg's key queue in this situation; only 'You failed', recall and deep-descent lift-off messages do). The Borg is not asked to think again once its queue is empty.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairphase-34
git fetch https://github.com/diegoami/neo-angband-tracker.git task/stairphase-34-evidence
git show eb91a6756c9937929bc3b90578ec6cb408b6b26f:docs/audit/c-reports/stairphase-34/reproduce.py > /tmp/reproduce-stairphase-34.py
python3 /tmp/reproduce-stairphase-34.py --full-tests --fixtures-ref eb91a6756c9937929bc3b90578ec6cb408b6b26f --fix-ref 2176cf71cebb4b2922e67a3516e3c82c283ed426
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-escape.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `2176cf71cebb4b2922e67a3516e3c82c283ed426` (branch `task/stairphase-34`), fixtures
  `eb91a6756c9937929bc3b90578ec6cb408b6b26f` (branch `task/stairphase-34-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: downstair, floor, climb
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

In borg_escape() danger levels 3.1, 4.1, 5.1, 6.1 and 8 combine the stair check (which queues '<') with the phase condition. On an up staircase a level-16 Warrior queued '<' followed by a Phase Door read (or, with the phase blocked by the anti-summon timer, a Teleportation read from the next step). The game took the stairs and then played the leftover keys on the new level, spending the scroll and, for Phase Door, moving the Borg off its arrival staircase before it had looked at the level. Making the stair escape a complete alternative queues only '<'; three controls are byte-identical, two of them through the changed condition.

Measured in one fixture: Danger Level 4.1 (phase possible) and the 4.1 to 4.2 fall-through (phase blocked). Levels 3.1, 5.1, 6.1 and 8 have the same shape and the same change but were not run. Not measured: what the Borg met on the new level after the scroll, how often it escapes while standing on an up staircase, or the cost of a Phase Door versus a Teleportation scroll in a real game. Only the depth-9 level generated with seed 34 was seen. With the correction a stair escape at these steps still logs 'Danger Level 3.1/4.1/...' and applies the phase bookkeeping (borg.escapes is reduced by one and the anti-summon timer reset); borg.escapes is reset when the Borg arrives on a new level, so this has no effect after the climb. With the correction the Borg on an up staircase no longer reaches the later steps that set the fleeing/leaving goals or teleport when the phase is blocked; it climbs instead.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
