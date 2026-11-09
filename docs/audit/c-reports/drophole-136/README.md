# A full-pack Borg above level 35 dropped a spare amulet and kept a worthless potion

This is **drophole-136 / issue136**, a C Borg defect
(diegoami/neo-angband-tracker#136).

When the Borg's pack is full it drops the item whose loss costs it least. Above character level 35 the drop code is meant to protect a short list of valuable consumables (healing and mana potions, a few attack rods and wands, two protective scrolls) by giving them a very high value. The protection test was written as 'not A or not B', which is true for every item, so every potion, rod and wand counted as worth 300,000 gold and every scroll 30,000. A level-40 Warrior with a full pack therefore kept a Potion of Salt Water (worth nothing) and dropped a spare Amulet of ESP (25,000 gold), or a spare Amulet of Slow Digestion, inscribed for the Borg to destroy. With the comparisons corrected to 'is A or is B', only the listed items are protected and the Salt Water is dropped.

## What to watch

Dropped the spare amulet (1 = amulet dropped, 0 = potion dropped); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `ca9047877`) |
| --- | --- | --- |
| Level 40: spare Amulet of Slow Digestion and a Potion of Salt Water | failed, dropped_amulet=1 | passed, dropped_amulet=0 |
| Level 40: wearing an Amulet of ESP, carrying a second one and a Potion of Salt Water | failed, dropped_amulet=1 | passed, dropped_amulet=0 |
| Control: level 40, spare Amulet of Slow Digestion and a Potion of Restore Mana | passed, dropped_amulet=1 | byte-identical log |
| Control: level 30, spare Amulet of Slow Digestion and a Potion of Salt Water | passed, dropped_amulet=0 | byte-identical log |

- **Level 40: spare Amulet of Slow Digestion and a Potion of Salt Water.** Original: the Salt Water counts as worth 300,000 gold, so the Borg drops the amulet (worth 200 gold). Corrected: the Salt Water counts as worth nothing and is dropped.

- **Level 40: wearing an Amulet of ESP, carrying a second one and a Potion of Salt Water.** Original: the Borg drops the spare 25,000-gold Amulet of ESP and keeps the Salt Water. Corrected: it drops the Salt Water and keeps the amulet.

- **Control: level 40, spare Amulet of Slow Digestion and a Potion of Restore Mana.** Restore Mana is on the protected list. The changed potion test runs in both versions and values it at 300,000 gold either way, so both drop the amulet; the logs are byte-identical.

- **Control: level 30, spare Amulet of Slow Digestion and a Potion of Salt Water.** Below level 36 the drop code skips the high-level valuation block entirely; both versions drop the Salt Water and the logs are byte-identical. This shows the rest of the drop choice is unchanged.

## Conditions

Native Human Warrior, seed 136, depth 20, every rune known, standing on an open floor with no monsters. The starting kit's carried items are removed (the wielded torch stays), then the pack holds 3 rations, one spare amulet, one potion, and is filled to the last slot with fully known artifact weapons, which the full-pack drop never considers. The production borg_drop_hole(false) runs once; the queued keys are read back and checked to be the inscribe-and-drop sequence for one inventory letter, and that item is then dropped with the game's own inven_drop(). Fixture interventions: experience granted directly to reach the level, pack contents built directly, the Borg's map copied from the arena. This isolates the choice of item to drop; it does not play the later crushing of the dropped item or any trip to sell.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/drophole-136
git fetch https://github.com/diegoami/neo-angband-tracker.git task/drophole-136-evidence
git show 29ebf656d76555971255c63701b9d8de7d6acd9d:docs/audit/c-reports/drophole-136/reproduce.py > /tmp/reproduce-drophole-136.py
python3 /tmp/reproduce-drophole-136.py --full-tests --fixtures-ref 29ebf656d76555971255c63701b9d8de7d6acd9d --fix-ref ca904787777ca8fed7e113f52f1a6ae4fa02daeb
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-junk.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `ca904787777ca8fed7e113f52f1a6ae4fa02daeb` (branch `task/drophole-136`), fixtures
  `29ebf656d76555971255c63701b9d8de7d6acd9d` (branch `task/drophole-136-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: restore-mana, level30
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

In borg_drop_hole() the high-level 'don't crush cool stuff' checks for rods, staffs, wands, scrolls and potions compared svals with != joined by ||, which is always true. A level-40 Warrior with a full pack dropped a spare Amulet of Slow Digestion (200 gold) or a spare Amulet of ESP (25,000 gold) and kept a Potion of Salt Water. With == the Salt Water is dropped. A protected Restore Mana potion at level 40 and the same pack at level 30 give byte-identical logs.

Measured: the item chosen and dropped once, for potions only. Not measured: rods, wands, staffs and scrolls (the same pattern is corrected there; after the fix unlisted aware rods are valued at their price and unlisted wands, staffs, scrolls at a half or a tenth of it, so they become cheaper to drop than before); the later crushing of the item inscribed 'borg ignore'; the gold lost from not selling it; how often a level-36+ Borg is pack-full with such a choice. A level-40 Borg seldom needs gold, so the harm is the loss of a sellable or useful spare item, not a combat or survival effect. The staff test keeps its count conditions (fewer than two power/holiness staffs or destruction charges left without the item), which read as intended once == is used; this interpretation is the author's reading of the original intent.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
