# A Slay Troll spare weapon was undervalued, so the Borg kept the wrong one

This is **trollswap-201 / issue201**, a C Borg defect
(diegoami/neo-angband-tracker#201).

When the Borg carries more than one spare weapon it chooses the most useful as its swap weapon and keeps it; the others are offered to the shops. It scores each weapon's slays, and a Slay Troll weapon was scored with a damage multiplier of 2 in the first half of its reward although the weapon's multiplier is 3. An otherwise identical Slay Orc weapon was scored with its real multiplier of 3. So the Borg preferred a Slay Orc mace over a slightly better Slay Troll mace and offered the Slay Troll mace for sale. The correction uses the weapon's own multiplier.

## What to watch

The Slay Troll mace is the one offered for sale (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `4fe83bc81`) |
| --- | --- | --- |
| Slay Troll mace (+1 damage) against Slay Orc mace | failed, sells_troll=1 | passed, sells_troll=0 |
| Control: Slay Troll mace against a plain mace | passed, sells_troll=0 | byte-identical log |

- **Slay Troll mace (+1 damage) against Slay Orc mace.** A level-30 Warrior whose deepest level is 50 carries two spare maces that differ only in the slay: one of Slay Orc, one of Slay Troll with one extra point of damage. Both slays multiply damage by 3. Original: the Slay Troll mace is undervalued, so the Orc mace becomes the swap weapon and the Borg offers the Slay Troll mace for sale. Corrected: the Slay Troll mace is the swap weapon and the Borg offers the Slay Orc mace.

- **Control: Slay Troll mace against a plain mace.** The same Warrior with a plain Mace and a Slay Troll Mace. The Slay Troll mace is the better swap before and after the correction, so the plain mace is offered for sale in both.

## Conditions

Native Human Warrior, seed 201, character level 30, deepest level 50 (the Borg only carries a swap weapon from level 50), no gold, a torch and rations, in a lit room. Fixture interventions: the starting Potion of Berserk Strength is removed (it is the cheapest spare and is always offered for sale first, which would hide the choice between the maces); the Warrior wields the starting dagger so the maces are the only spare weapons; in the changed scenario the Slay Troll mace carries one extra point of damage so that, with the correct valuation, it is strictly (not tie-break) the better swap. All runes are known. The production borg_notice() chooses the swap weapon and the production borg_think_shop_sell_useless() chooses what to sell; the shop and the sale are not played.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/trollswap-201
git fetch https://github.com/diegoami/neo-angband-tracker.git task/trollswap-201-evidence
git show 600adff3dfc087f59e842bf2a896b742f0de1ac9:docs/audit/c-reports/trollswap-201/reproduce.py > /tmp/reproduce-trollswap-201.py
python3 /tmp/reproduce-trollswap-201.py --full-tests --fixtures-ref 600adff3dfc087f59e842bf2a896b742f0de1ac9 --fix-ref 4fe83bc81a2208e76481e09fa66a66481052cd44
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-trait-swap.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `4fe83bc81a2208e76481e09fa66a66481052cd44` (branch `task/trollswap-201`), fixtures
  `600adff3dfc087f59e842bf2a896b742f0de1ac9` (branch `task/trollswap-201-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: troll-plain
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With two spare maces that differ only in a x3 slay, the original Borg valued the Slay Troll mace at 2.5 times the base damage against 3 times for Slay Orc, kept the Slay Orc mace as its swap weapon and offered the Slay Troll mace for sale. With the troll multiplier used in both halves the Borg keeps the Slay Troll mace and offers the Slay Orc mace. A plain-mace control, where the Slay Troll mace wins either way, is byte-identical.

Valuation only, low priority: this changes which spare weapon the Borg keeps as a swap and offers for sale in one constructed pack. It does not measure combat, a swap in a fight, or how often a pack holds two near-equal slay weapons (a swap weapon is only carried from deepest level 50). The extra point of damage on the Slay Troll mace is a fixture choice; without it the corrected valuation ties and the later pack slot wins.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
