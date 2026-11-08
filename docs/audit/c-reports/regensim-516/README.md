# The Borg queued a Rapid Regeneration cast while only scoring its perma-spell options

This is **regensim-516 / issue #516**, a C Borg defect
(diegoami/neo-angband-tracker#516).

Between fights a high-level caster can cast long-lasting preparatory spells. The Borg first scores each candidate with a simulation flag set, then runs only the best one for real. The Rapid Regeneration routine did not check the flag. While scoring it queued the cast and set the no-rest timer, so a cast was queued even when another spell won, and queued twice when Rapid Regeneration itself won. The correction returns a low score while simulating, as the Heroism and Bless routines do.

## What to watch

Spell casts queued by one perma-spell call; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `3553ca8ac`) |
| --- | --- | --- |
| Rapid Regeneration is the only choice (Haste Self already active) | failed, casts_queued=2 | passed, casts_queued=1 |
| Haste Self is the best choice; Rapid Regeneration is also available | failed, casts_queued=2 | passed, casts_queued=1 |
| Control: Rapid Regeneration already active | passed, casts_queued=1 | byte-identical log |

- **Rapid Regeneration is the only choice (Haste Self already active).** A level-50 Druid at full spell points, 20 levels down, with Haste Self already on. Rapid Regeneration is the only perma spell left, so it wins. Original: the scoring pass queues the cast and the real pass queues it again, six keys, two casts. Corrected: one cast, three keys.

- **Haste Self is the best choice; Rapid Regeneration is also available.** The same Druid with no spell active. Haste Self scores 5 and Rapid Regeneration 1, so Haste Self should be the only cast. Original: the scoring pass has already queued Rapid Regeneration, so the keys are Rapid Regeneration then Haste Self, two casts. Corrected: Haste Self only.

- **Control: Rapid Regeneration already active.** The same Druid with Rapid Regeneration already running. The routine returns before the changed lines, so only Haste Self is queued, before and after. This control only shows that the rest of the perma-spell choice is unchanged; every path that reaches the changed lines changes its output.

## Conditions

Native Human Druid, seed 516, character level 50, stats raised to the maximum, full hit points and spell points (401), depth 20, a lit open arena, no monsters, a lit torch, rations and every Druid book. Fixture interventions: the Druid is marked as having learned and cast Haste Self and Rapid Regeneration; stats are set by the fixture so the 20-point cost passes the Borg's one-tenth-of-spell-points test; the Borg's map update is not run, so the casts are queued, not played. The production borg_perma_spell() chooses and queues the spells.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/regensim-516
git fetch https://github.com/diegoami/neo-angband-tracker.git task/regensim-516-evidence
git show a7f567ec8745d6c43f22d7e3b9773f6aa5d07223:docs/audit/c-reports/regensim-516/reproduce.py > /tmp/reproduce-regensim-516.py
python3 /tmp/reproduce-regensim-516.py --full-tests --fixtures-ref a7f567ec8745d6c43f22d7e3b9773f6aa5d07223 --fix-ref 3553ca8ac550d6510cc51663b0d611b8cc72aa45
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-fight-perm.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `3553ca8ac550d6510cc51663b0d611b8cc72aa45` (branch `task/regensim-516`), fixtures
  `a7f567ec8745d6c43f22d7e3b9773f6aa5d07223` (branch `task/regensim-516-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: regen_active
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With Haste Self as the best perma spell the original also queued Rapid Regeneration (two casts); with Rapid Regeneration as the only choice it queued it twice. After the correction each case queues the one cast that was chosen.

Priority low: the cost is a wasted cast of 20 of 401 spell points and a game turn, in one class (Druid, spell level 37) when it is rested and deep enough for perma spells. The no-rest timer set by the scoring pass is not measured separately, because the winning spell overwrites it. The fixture does not play the queued keys.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
