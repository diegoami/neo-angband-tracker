# Borg started after Sauron's death swaps in a weaker helm or throws away an item on its first move

This is **deaduniq-101 / issue #101**, a C Borg defect
(diegoami/neo-angband-tracker#101).

When the Borg starts, borg_init_flow_kill() is meant to copy the game's record of dead uniques into the Borg's own table, but its test is inverted (it skips the uniques and checks the non-uniques), so the table starts empty. The first map update fills it in, but borg_think() examines the character before that update and computes the Borg's reference power score (borg.power) from that examination. On a character that has killed Sauron and carries Scrolls of Mass Banishment, borg_power() adds 2,500 per scroll once Sauron is known dead, so the reference score of the first pass is 12,500 lower than every alternative the same pass scores. In this run a level-41 Warrior at depth 98 (maximum depth 99) wears a Metal Cap and carries a Hard Leather Cap. The original Borg swaps the Metal Cap for the weaker Hard Leather Cap; on the junk decision it drops and ignores (marks for destruction) a Potion of Berserk Strength, or, without the birth-kit potion, one of its five Scrolls of Mass Banishment. After the fix the table is filled at start-up, the reference score matches the alternatives, and both decisions do nothing. With Sauron alive both versions behave identically.

## What to watch

Borg acts on its first pass: swaps the helm or drops an item (1 yes, 0 no); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `35d4ff3f4`) |
| --- | --- | --- |
| Sauron dead, 5 Mass Banishment: spare Hard Leather Cap | failed, acts=1 | passed, acts=0 |
| Sauron dead, 5 Mass Banishment: junk decision | failed, acts=1 | passed, acts=0 |
| Sauron dead, 5 Mass Banishment, no birth-kit potions: junk decision | failed, acts=1 | passed, acts=0 |
| Control: Sauron alive, spare Hard Leather Cap | passed, acts=0 | byte-identical log |
| Control: Sauron alive, junk decision | passed, acts=0 | byte-identical log |
| Sauron dead, no Mass Banishment scrolls: spare Hard Leather Cap | passed, acts=0 | passed, acts=0 |

- **Sauron dead, 5 Mass Banishment: spare Hard Leather Cap.** Original: the first pass's reference power is 1,513,679 while the same gear scores 1,526,179 once Sauron is known dead; the Hard Leather Cap scores 1,524,179 (2,000 below the worn Metal Cap) and still beats the stale reference, so the Borg sends 'w' and swaps the Metal Cap [3] for the Hard Leather Cap [2]. Corrected: the reference is 1,526,179 and no swap is made.

- **Sauron dead, 5 Mass Banishment: junk decision.** Original: borg_drop_junk() (enabled by the first map update's new-level branch) compares each possible drop with the stale reference, finds the Potion of Berserk Strength's removal 'better', and requests '{' 'borg ignore' then 'd': the potion is dropped and marked for destruction. Corrected: nothing is dropped.

- **Sauron dead, 5 Mass Banishment, no birth-kit potions: junk decision.** The same junk decision without the birth-kit potions. Original: the cheapest removal is one Scroll of Mass Banishment, which the Borg drops and marks 'borg ignore'. Corrected: nothing is dropped.

- **Control: Sauron alive, spare Hard Leather Cap.** No unique is dead, so the table is empty in both versions and the reference score is consistent. No swap in either version; byte-identical logs (the corrected start-up loop runs over every race).

- **Control: Sauron alive, junk decision.** Sauron alive, same pack as the junk case. Nothing is dropped in either version; byte-identical logs.

- **Sauron dead, no Mass Banishment scrolls: spare Hard Leather Cap.** Sauron dead but no Mass Banishment scrolls, so his death changes no power term. The first examination still reads Sauron as alive in the original (sauron_dead_trait=0) and as dead after the fix, but the reference score is the same and neither version swaps. Not byte-identical (the trait line differs); shown to bound the effect.

## Conditions

Native Human Warrior, seed 101, character level 41, depth 98, maximum depth 99, lit open 24x40 arena with no monsters; worn Metal Cap [3], Wooden Torch, the birth-kit dagger and soft leather armour; carried 30 Potions of Healing and 15 Potions of Speed (so BI_PREP_BIG_FIGHT is false, borg-trait.c:2928-2947), 5 Scrolls of Mass Banishment (none in wear-noscrolls), plus a Hard Leather Cap [2] (wear modes) or 5 Scrolls of Teleportation and 5 Potions of Cure Critical Wounds (junk modes); profile borg.txt from the shared evidence profile. Real code: production borg_init() (which runs borg_init_flow_kill), then the first pass in borg_think()'s order (borg_find_all_items, borg_notice, borg_update, borg.power = borg_power(); borg-think.c:186-217), the borg_notice(true) that borg_think_dungeon() runs before its decisions (borg-think-dungeon.c:1435), and the production decision borg_wear_stuff() (wear modes) or borg_drop_junk() (junk modes); the requested keys are checked and carried out with the game's inven_wield() or inven_drop(). Fixture interventions: Sauron's death is recorded as the game records it (lore kill count 1 and r_info max_num 0, mon-util.c:1101, load.c:533-535) instead of by fighting him; the character, arena and gear are built directly; the birth kit's spare torches are removed (and, in junk-nokitpotion, its potions); the decision function is called directly rather than reached through the whole dungeon thinker; the 'borg ignore' inscription that precedes the junk drop is checked in the keys but not carried out, only the drop.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/deaduniq-101
git fetch https://github.com/diegoami/neo-angband-tracker.git task/deaduniq-101-evidence
git show b70349d7da21677c27a879fae79fa6ba1f97996a:docs/audit/c-reports/deaduniq-101/reproduce.py > /tmp/reproduce-deaduniq-101.py
python3 /tmp/reproduce-deaduniq-101.py --full-tests --fixtures-ref b70349d7da21677c27a879fae79fa6ba1f97996a --fix-ref 35d4ff3f43ad839952466d7d8580320adba95b8c
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-kill.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `35d4ff3f43ad839952466d7d8580320adba95b8c` (branch `task/deaduniq-101`), fixtures
  `b70349d7da21677c27a879fae79fa6ba1f97996a` (branch `task/deaduniq-101-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: alive-wear, alive-junk
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_init_flow_kill() (borg-flow-kill.c:3321-3335) skips uniques and tests non-uniques, whose max_num is never 0 (load.c:516-520, player-birth.c:421-427), so borg_race_death[] starts empty. borg_update() refills it on its first new-level pass (borg-update.c:2273-2287; borg.status.old_depth starts at 128, borg-init.c:519), but borg_think() runs borg_notice() first (borg-think.c:211) and sets borg.power after the update from that examination (borg-think.c:217), so BI_SAURON_DEAD (borg-trait.c:3131) is 0 in the reference and 1 in every candidate scored later in the pass (borg-power.c:1454 adds 2,500 per Mass Banishment scroll). borg_wear_stuff() (borg-item-wear.c:767, 995) and borg_drop_junk() (borg-junk.c:254, 449) compare candidates against that reference. Skipping non-uniques seeds the table at start-up, as the comment and borg_update()'s scan do.

Measured: the first pass's reference score and one production decision (wear or junk) right after borg_init(), with Sauron's death recorded as the game records it. Not measured: the whole dungeon thinker (decisions earlier in borg_think_dungeon() may act first on that pass), the second pass (borg.power is recomputed with the table filled, so the window is one pass after each Borg start, and a swapped helm would likely be swapped back), other dead-unique terms (the Tarrasque check in borg-trait-swap.c:516/1084 is re-run by borg_notice(true) inside the same pass), and the shop decisions that also compare with borg.power. The effect needs a character whose savefile already records Sauron dead, carrying Mass Banishment scrolls with BI_PREP_BIG_FIGHT false. A unique killed while the Borg is running is recorded by borg_update() and is not affected. Reincarnation (borg-reincarnate.c) does not call borg_init_flow_kill() and is unchanged.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
