# Remove the dead mana-cost test from the Blackguard's Venom preparation

This is **venomcost-111 / issue #111**, a C Borg defect
(diegoami/neo-angband-tracker#111).

Cleanup, no behaviour change; owner decision (option C). Between fights the Borg casts 'perma' spells that prepare it for battle. For the Blackguard, borg_perma_aux_venom() decides whether to poison its weapon with Venom. The routine was copied from the Smite Evil one and still reads Smite Evil's mana cost. The Blackguard has no Smite Evil, so the cost is -1 and the test 'only if it is cheap' always passes; the spell-okay check just before it already requires enough spell points for Venom's real cost of 18. The change removes the cost lookup and the test and keeps today's behaviour: the Blackguard casts Venom without a mana-cost limit.

## What to watch

Venom cast by borg_perma_spell() (1 = cast, keys m b d queued; 0 = not cast); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `65aa5a6d4`) |
| --- | --- | --- |
| Control: unique 3 grids away, clevel 42 (cast) | passed, cast=1 | byte-identical log |
| Control: unique 3 grids away, clevel 50 (cast) | passed, cast=1 | byte-identical log |
| Control: a cave orc, no unique (not cast) | passed, cast=0 | byte-identical log |
| Control: unique near, weapon already poisoned (not cast) | passed, cast=0 | byte-identical log |

- **Control: unique 3 grids away, clevel 42 (cast).** Venom fails 12% of the time, within the 15% allowed near a unique. The original passes the dead cost test (cost read as -1 against 38 spell points) and casts; the changed version casts the same way (keys m b d, no_rest_prep 19000). Logs byte-identical.

- **Control: unique 3 grids away, clevel 50 (cast).** Same as 'unique' with 46 spell points: both versions cast Venom. Logs byte-identical.

- **Control: a cave orc, no unique (not cast).** Without a unique the failure allowance is 5%, below Venom's 12%, so both versions stop before the cost test and cast nothing. Logs byte-identical.

- **Control: unique near, weapon already poisoned (not cast).** The weapon is already poisoned, so both versions return before the cost test and cast nothing. Logs byte-identical.

## Conditions

Native Human Blackguard, seed 111, clevel 42 (50 in 'unique50'), depth 20, full HP and spell points, a torch, rations and only the [Fear and Torment] book (so Venom is the only perma spell available), Venom learned through spell_learn(), in a lit, open 24x40 arena with one monster 3 grids east, observed through the Borg's observe_kill_diff(); borg_near_monster_type(12) runs as borg_think_dungeon() runs it. The production borg_perma_spell() runs (simulation, then the real cast); the queued keys are read back. 'unique': Bullroarer the Hobbit. 'unique50': the same at clevel 50. 'nounique': a cave orc. 'poisoned': Bullroarer, with the weapon already poisoned (TMD_ATT_POIS set). Fixture interventions: the arena and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_VIEW, borg.avoidance is set to current HP, current HP and SP are set to their maximum, and in 'poisoned' the timed effect is set directly.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/venomcost-111
git fetch https://github.com/diegoami/neo-angband-tracker.git task/venomcost-111-evidence
git show 06f485e8d7f39db503166d7241369194a699cb42:docs/audit/c-reports/venomcost-111/reproduce.py > /tmp/reproduce-venomcost-111.py
python3 /tmp/reproduce-venomcost-111.py --full-tests --fixtures-ref 06f485e8d7f39db503166d7241369194a699cb42 --fix-ref 65aa5a6d4cc047559b9e633b91534ee5fa920310
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-fight-perm.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `65aa5a6d4cc047559b9e633b91534ee5fa920310` (branch `task/venomcost-111`), fixtures
  `06f485e8d7f39db503166d7241369194a699cb42` (branch `task/venomcost-111-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: unique, unique50, nounique, poisoned
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The removed test in borg_perma_aux_venom() can never refuse the cast: it reads Smite Evil's cost, which is -1 for the Blackguard (the only class with Venom), and -1 >= spell points / 10 (or / 7) needs negative spell points, while borg_spell_okay_fail(VENOM) just above requires at least 18. Four native borg_perma_spell() scenarios with a Blackguard, two that cast Venom through the removed lines and two that do not cast, give byte-identical logs before and after, and both full native gates pass on both builds.

This is a cleanup with no failing mode: the scenarios show unchanged behaviour, they cannot show a difference. Unreachability of a refusal is argued from the source and the stock class.txt (only the Blackguard has Venom, and it has no Smite Evil). Not measured: whole-game runs, or whether the Blackguard should have a mana budget for Venom (owner options A and B on the issue, not taken).

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
