# The Borg judged Phase Door by landings that never happen

This is **phasemodel-595 / issue595**, a C Borg defect (diegoami/neo-angband-tracker#595), found during the strategy-manual comparison (C-FIX-POSTING §8.1).

Before reading Phase Door, `borg_caution_phase()` simulates 100 landings and refuses if too many would be dangerous. It sampled landings **5 to 10** squares away. The game's teleport (`effect_handler_TELEPORT`) aims at the dice distance — 10 for Phase Door — randomised by up to a quarter (`randint0(dis / 4)`, so ±1), and lands on the grid closest to that distance: **9, 10 or 11** squares away. The Borg weighed landing spots that never occur (5–8) and ignored the farthest ring (11). The correction draws the target distance the way the game does and judges landings at that distance.

## What to watch

Whether the Borg finds Phase Door safe; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `37acca6c1`) |
| --- | --- | --- |
| Grey molds on every grid 6 squares away | failed: "# No Phase. scary squares: 37" | passed: "# Safe to Phase. scary squares: 0" |
| Control: no monsters | passed: "# Safe to Phase. scary squares: 0" | byte-identical log |

Grey molds never move, so only landings next to one (5 to 7 squares from the Borg) are dangerous. A real Phase Door lands 9 to 11 squares away, never next to a mold; the original refused it because of landings that cannot happen.

## Conditions

Native Human Warrior, seed 595, character level 20, 3 of 242 hit points, five Scrolls of Phase Door, an open lit 40×60 arena. In the ring case, 32 grey molds stand on every grid exactly 6 squares from the Borg. Fixture interventions: hit points set to 3 so a landing next to a mold counts as scary (its danger exceeds them); the molds enter the Borg's monster list through the production `observe_kill_diff()`. The production `borg_caution_phase(20, 2)` — the threshold most of its callers use — runs once.

[replay.html](replay.html) is the offline page (download raw, open locally): choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/phasemodel-595
git fetch https://github.com/diegoami/neo-angband-tracker.git task/phasemodel-595-evidence
git show 30fef466412c6e5f1e9e30731c10ca7e82e53ffc:docs/audit/c-reports/phasemodel-595/reproduce.py > /tmp/reproduce-phasemodel-595.py
python3 /tmp/reproduce-phasemodel-595.py --full-tests --fixtures-ref 30fef466412c6e5f1e9e30731c10ca7e82e53ffc --fix-ref 37acca6c162698e3f8ecd0d111ae69c3e1309cdd
```

The runner asserts the correction is exactly `fix.patch` (`src/borg/borg-escape.c` only), builds the original and corrected native C game in a disposable worktree, runs both scenarios, checks each expected result and the byte-identical control, runs both full native gates and writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`; the frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `37acca6c162698e3f8ecd0d111ae69c3e1309cdd` (branch `task/phasemodel-595`), fixtures `30fef466412c6e5f1e9e30731c10ca7e82e53ffc` (branch `task/phasemodel-595-evidence`); later commits only add these artifacts.
- control byte-identical: control
- full native gates: before and after 965/965 unit checks and 5/5 frontend tests
- corrected source sha256 (`src/borg/borg-escape.c`): `ce3e07b00bad804eb91754599d695942fc74850f9a695cc91b7ffbdc96f32b13`
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

The Borg's Phase Door safety check now samples the distances the game actually uses; in a constructed ring it no longer refuses a safe Phase Door because of landings 5–8 squares away. The opposite error (approving a phase whose real landings at 11 squares are dangerous) is covered by the same change but not exercised here.

Not measured: what the Borg does instead after a refusal, fights, deaths. Real maps are rarely a perfect ring; the effect in play is a shifted risk estimate, not always a flipped decision. Portal reportedly reuses this check although it travels much farther; that is not changed here (see the issue).

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`) are the generators; `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file) are the captured run.
