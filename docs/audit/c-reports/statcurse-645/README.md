# Worn stat curses never lower the borg's derived stats

This is **statcurse-645 / issue #645**, a C Borg defect
(diegoami/neo-angband-tracker#645).

When the Borg notices a known-cursed non-artifact it is wearing, it skips the item entirely in its equipment scan - it is going to take the item off, so the scan does not total its bonuses. One consequence: the four stat curses (dullness, sickliness, weakness, clumsiness) are never recorded, and the penalties that the code did apply went to current-stat traits that the effective-stat pass immediately overwrites with the game's own base stat. The engine instead adds the curse's own modifiers into stat_add, so the game's effective stats include the penalties. The result is that a Borg wearing, say, weakness gloves believes it has STR 38 when the game gives it 10, and plans a carry allowance of 2400 where the game allows 1200. The fixture equips the mode's known-cursed item through the real wear path (append_object_curse, inven_wield, the wear-time notice), recomputes the game's stats, runs one production borg_notice() and requires every derived stat to equal the game's own effective stat.

## What to watch

1 = every derived stat equals the game's own effective stat (per-stat values on the STATINFO lines of each log); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `574b618c3`) | Corrected (fix `e56b1dbb9`) |
| --- | --- | --- |
| Plain Leather Gloves (control) | passed, stats_ok=1 | byte-identical log |
| Weakness gloves (STR -10) | failed, stats_ok=0 | passed, stats_ok=1 |
| Weakness and clumsiness gloves (STR -10, DEX -10) | failed, stats_ok=0 | passed, stats_ok=1 |
| Sickliness helm (STR/DEX/CON -5) | failed, stats_ok=0 | passed, stats_ok=1 |
| Dullness helm (INT/WIS -5) | failed, stats_ok=0 | passed, stats_ok=1 |

- **Plain Leather Gloves (control).** Uncursed gloves. The borg's derived stats already equal the game's; the complete log is byte-identical.

- **Weakness gloves (STR -10).** Gloves carrying a known weakness curse. The game's effective STR is 10; the original borg derives 38 and a carry allowance of 2400 where the game allows 1200. The corrected borg derives 10 and 1200.

- **Weakness and clumsiness gloves (STR -10, DEX -10).** Both glove curses. The game's effective STR/DEX are 10/8; the original borg derives 38/18. The corrected borg derives 10/8.

- **Sickliness helm (STR/DEX/CON -5).** An Iron Helm carrying sickliness. The game's effective STR/DEX/CON are 15/13/13; the original borg derives 38/18/18. The corrected borg matches the game on all three.

- **Dullness helm (INT/WIS -5).** An Iron Helm carrying dullness. The game's effective INT/WIS are 3/3; the original borg derives 8/8, which overstates mana and spell-failure adjustments. The corrected borg derives 3/3.

## Conditions

Native Human Warrior, level 20 (experience forced), depth 5 in a synthetic open lit arena with no monsters, a wooden torch as the only light, infravision 0. Each mode equips one plain or cursed item (Set of Leather Gloves / Iron Helm) with curses attached by the game's own append_object_curse() and learned through the wear-time notice path (object_learn_on_wield, player_learn_curse), then recomputes the engine bonuses (PU_BONUS) and runs one production borg_notice(). No combat, movement or random placement consumes the RNG (fixed seed 645); a scenario log is fully determined by its mode. Limits: the fixture measures the stat model (derived stats, stat index, carry allowance), not a specific fight or wear decision; the borg's wider skip of known-cursed non-artifacts (armor, speed and weight totals while such an item is worn) is untouched by this change and left as a separate owner question recorded on the issue.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario to see the original and corrected runs side by side on the dungeon map, drawn with the game's own symbols (the legend under the maps explains them), press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/statcurse-645
git fetch https://github.com/diegoami/neo-angband-tracker.git task/statcurse-645-evidence
git show 9afa40b6b5bd79d2709eb5975c3316d2006b3a75:docs/audit/c-reports/statcurse-645/reproduce.py > /tmp/reproduce-statcurse-645.py
python3 /tmp/reproduce-statcurse-645.py --full-tests --fixtures-ref 9afa40b6b5bd79d2709eb5975c3316d2006b3a75 --fix-ref e56b1dbb92c82d8bb01b24ed2a3a6255d2b34472
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-trait.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix `e56b1dbb92c82d8bb01b24ed2a3a6255d2b34472` (branch `task/statcurse-645`), fixtures
  `9afa40b6b5bd79d2709eb5975c3316d2006b3a75` (branch `task/statcurse-645-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: plain
- full native gates: before 966/966 unit checks and 5/5 frontend tests;
  after 966/966 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With a known-cursed item worn, the borg's derived stats diverge from the game's effective stats exactly as described and the curse traits never fire: weakness gloves read STR 38 against the game's 10 with a carry allowance of 2400 where the game allows 1200; weakness+clumsiness reads STR/DEX 38/18 against 10/8; a sickliness helm reads STR/DEX/CON 38/18/18 against 15/13/13; a dullness helm reads INT/WIS 8/8 against 3/3. With the correction every derived stat, the stat index and the carry allowance equal the game's, and the uncursed control is byte-identical. Both full native gates pass on both variants.

The fixture measures the stat model: derived stats, the STR stat index behind the carry allowance, and the curse traits. Wear/trial comparisons, HP and mana adjustments consume these traits and change with them, but no individual wear decision or combat is replayed here. The borg's wider skip of worn known-cursed non-artifacts - its own modifiers, armor, speed and weight totals while worn - is untouched by this change and is recorded on the issue as a separate owner question; items whose own stat modifiers differ from zero would still need that decision before their full model matches the game.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
