# A fear-protecting spare weapon got no credit, so the Borg kept the wrong one

This is **swapfear-202 / issue202**, a C Borg defect
(diegoami/neo-angband-tracker#202).

When the Borg carries more than one spare weapon it keeps the most useful as its swap weapon and offers the others to the shops. Its scoring adds a bonus (+5500) for a swap weapon that protects from fear when the Borg has no fear protection of its own. But the flag that bonus reads was never set from the weapon, so a fear-protecting weapon — a *(Holy Avenger)*, an *of Fury* weapon, or one of the 26 artifacts with fear protection — was scored as if it gave nothing. The Borg kept a plain Mace with one more point of damage and offered the fear-protecting Mace for sale. The correction sets the flag (and the matching armour flag) from the item, exactly as protection from confusion and blindness already are.

## What to watch

The fear-protecting Mace is the one offered for sale (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `c5eae07cb`) |
| --- | --- | --- |
| Fear-protecting Mace against a plain Mace with +1 damage | failed, sells_fear=1, swap value 1299 | passed, sells_fear=0, swap value 6724 |
| Control: same Maces, Warrior already fearless (level 30) | passed, sells_fear=1 | byte-identical log |
| Control: two plain Maces | passed | byte-identical log |

- **Fear-protecting Mace against a plain Mace with +1 damage.** A level-29 Warrior with no fear protection of its own carries two spare Maces: one protects from fear, the other has one extra point of damage. Original: the fear protection is never recorded (`swap_rfear=0`), the +1 Mace wins on its extra point of damage and is kept as the swap weapon (swap value 1299), and the fear-protecting Mace is offered for sale. Corrected: the fear Mace scores 1224 + 5500 = 6724, is kept, and the +1 Mace is offered for sale.

- **Control: the same Maces, Warrior already fearless.** At level 30 the Warrior class protects from fear by itself (`BRAVERY_30`), so a swap's fear protection is worth nothing and the bonus correctly does not apply. The +1 Mace is kept in both builds.

- **Control: two plain Maces.** No fear protection anywhere; the +1 Mace is kept in both builds.

## Conditions

Native Human Warrior, seed 202, character level 29 (30 in the brave control), deepest level 50 (the Borg only carries a swap weapon from level 50), no gold, a torch and rations, in a lit room, every rune known. Fixture interventions: the starting Potion of Berserk Strength is removed (it is the cheapest spare and would be offered first); the Warrior wields the starting dagger so the Maces are the only spare weapons; the two Maces are made directly — one plain Mace with the `PROT_FEAR` object flag set (the flag the *(Holy Avenger)* and *of Fury* egos and 26 artifacts carry, isolated from their other properties), one plain Mace with one extra point of damage so that without the fear bonus it is strictly the better swap. The production `borg_notice()` chooses the swap weapon and the production `borg_think_shop_sell_useless()` chooses what to sell; the shop and the sale are not played.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/swapfear-202
git fetch https://github.com/diegoami/neo-angband-tracker.git task/swapfear-202-evidence
git show 27a567ee553986dd0e23352fe094652de7325eb9:docs/audit/c-reports/swapfear-202/reproduce.py > /tmp/reproduce-swapfear-202.py
python3 /tmp/reproduce-swapfear-202.py --full-tests --fixtures-ref 27a567ee553986dd0e23352fe094652de7325eb9 --fix-ref c5eae07cbcc6fb458ec15d04c234e5faf66ea6f1
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the eight expected hunks in `src/borg/borg-trait-swap.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.
The frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `c5eae07cbcc6fb458ec15d04c234e5faf66ea6f1` (branch `task/swapfear-202`), fixtures
  `27a567ee553986dd0e23352fe094652de7325eb9` (branch `task/swapfear-202-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: fear-brave, plain-plain
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- corrected source sha256 (`src/borg/borg-trait-swap.c`): `118a690211d0ba688434f9ecac02836c9dad8c72efe66d20479394b723eca964`
- replay: Playwright Chromium headless render check of the committed page
  (every option and visible button exercised), no console errors

## What this proves, and what it does not

With two spare Maces that differ only in fear protection against one point of damage, the original Borg gave the fear protection no value, kept the +1 Mace as its swap weapon and offered the fear-protecting Mace for sale. With the flag recorded, the existing +5500 bonus applies and the Borg keeps the fear-protecting Mace. When the Borg already protects itself from fear, the bonus correctly stays off and nothing changes.

Valuation only, low to medium priority: this changes which spare weapon the Borg keeps and offers for sale. It does not measure a fear episode, combat, or how often a pack holds a fear-protecting spare (a swap weapon is only carried from deepest level 50). The armour flag is now also recorded, but no armour valuation reads it at this commit (`BI_SRFEAR` has no reader), so the armour half changes no decision. The fear flag is set directly on a plain Mace to isolate it; real fear-protecting items carry other properties that the Borg already values.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
