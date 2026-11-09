# Home backup-item trials erased the Borg's own backups from its pack record

This is **swaptrial-31 / issue #31**, a C Borg defect
(diegoami/neo-angband-tracker#31).

In town the Borg's last two home steps look for a better backup weapon (7A) and backup armour (7B) to carry. They try each home item in the pack slot of the backup the Borg already carries, and after each trial only set that slot's count to zero instead of putting the old backup back. For the rest of that store turn the Borg's own record of its pack no longer held its backup weapon or backup helm. A level-50 Warrior carrying the quarterstaff of Olórin (resists nether) and the helm 'Holhenneth' as backups, with the amulet 'Evenstar' (also resists nether) and the long sword 'Elvagil' in its home, then valued 'Evenstar' as if no backup resisted nether, took it from the home, stored it again the next turn, took it back the turn after and left carrying a redundant amulet. The correction restores the slot after each trial; the Borg keeps its record of both backups, takes nothing and leaves.

## What to watch

Times the Borg takes the redundant amulet 'Evenstar' out of its home; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `571380b3d`) |
| --- | --- | --- |
| Backups carried, 'Evenstar' and 'Elvagil' at home | failed, evenstar_retrieved=2 | passed, evenstar_retrieved=0 |
| Control: no backup weapon, 'Elvagil' at home | passed, evenstar_retrieved=0 | byte-identical log |
| Control: backups carried, home empty | passed, evenstar_retrieved=0 | byte-identical log |

- **Backups carried, 'Evenstar' and 'Elvagil' at home.** Original: step 7A tries 'Elvagil' in the quarterstaff's slot and leaves that slot empty; step 7B tries 'Evenstar' in the helm's slot with no backup weapon in the record, so it gets the bonus for a high resist no backup covers and is chosen. After the decision the record shows both backup slots empty while the game's pack still holds both. The real commands then retrieve 'Evenstar' (round 1), stash it (round 2) and retrieve it again (round 3) before the Borg leaves carrying it. Corrected: both slots are restored after each trial, 'Evenstar' is valued with the quarterstaff present, no step chooses anything and the Borg leaves in round 1.

- **Control: no backup weapon, 'Elvagil' at home.** With no backup weapon, step 7A tries 'Elvagil' in the first empty pack slot (the changed restore line runs on an empty slot) and chooses it as the first backup weapon in both versions; the real retrieve takes it and the Borg leaves in round 2. The logs are identical.

- **Control: backups carried, home empty.** Nothing in the home, so neither trial loop copies anything into the pack record; both versions keep both backups in the record and leave in round 1. The logs are identical.

## Conditions

Native Human Warrior, seed 31, character level 50, maximum depth 60 (the Borg carries backups from depth 50; the shipped borg.txt has borg_uses_swaps = TRUE), 5000 gold, standing on the home entrance in town, all runes known. The starting kit's torch, armour, dagger, food, Word of Recall scroll and Berserk Strength potion are replaced by: worn lantern, long sword 'Ringil', helm of Dor-Lómin (see invisible, telepathy, the four basic resists), ring of Free Action, amulet of Trickery and metal scale mail; carried 5 rations, 5 Potions of Cure Critical Wounds, 5 Scrolls of Teleportation and 2 of Word of Recall. Nothing worn resists nether, confusion or blindness. Backups carried: the quarterstaff of Olórin (except 'no-weapon-swap') and the helm 'Holhenneth'. Every store is emptied; the home holds 'Evenstar' and 'Elvagil' ('swap-held'), 'Elvagil' only ('no-weapon-swap') or nothing ('no-candidate'). Fixture intervention ('no-weapon-swap' only): one sale-history entry saying 'Elvagil' was just stored at home (the fields borg_think_shop_sell() writes), so that steps 4 and 5, which read that history, leave it and step 7A, which does not, reaches it through the empty-slot path. The Borg's items and stores are loaded with the production borg_find_all_items() and borg_notice(); the production borg_choose_shop() makes the first store decision (steps 1 to 6 find nothing to do) and the fixture prints the Borg's record of the two backup slots and the first free slot against the game's pack; then up to six rounds each run one production borg_think_store() turn after re-reading the pack as borg_think() does, and its queued keys are carried out with the real do_cmd_retrieve() or do_cmd_stash(). A slot whose recorded count is zero is printed without its stale description: every reader skips such a slot.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/swaptrial-31
git fetch https://github.com/diegoami/neo-angband-tracker.git task/swaptrial-31-evidence
git show 7c9b0a909c68eabff5fe522d20c69cbbd85ee135:docs/audit/c-reports/swaptrial-31/reproduce.py > /tmp/reproduce-swaptrial-31.py
python3 /tmp/reproduce-swaptrial-31.py --full-tests --fixtures-ref 7c9b0a909c68eabff5fe522d20c69cbbd85ee135 --fix-ref 571380b3d4724a37a639dc96f2a9a16782f33d94
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-buy.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `571380b3d4724a37a639dc96f2a9a16782f33d94` (branch `task/swaptrial-31`), fixtures
  `7c9b0a909c68eabff5fe522d20c69cbbd85ee135` (branch `task/swaptrial-31-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: no-weapon-swap, no-candidate
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_think_home_buy_swap_weapon() and borg_think_home_buy_swap_armour() tried each home item in the pack slot of the carried swap and afterwards only zeroed that slot's quantity, so the Borg's record lost its backup weapon and backup armour for the rest of the store turn. With the quarterstaff of Olórin and 'Holhenneth' carried and 'Evenstar' at home, the original Borg valued 'Evenstar' without the backup weapon's nether resistance, retrieved it, stashed it and retrieved it again (3 store turns) and left carrying it; restoring the slot from safe_items makes it keep its record, take nothing and leave. Controls with no backup weapon (empty-slot path) and an empty home are byte-identical.

Measured: the first store decision with the Borg's record of the backup slots, and the following store turns (at most six) carried out with the real retrieve/stash commands. Not measured: later turns outside the store, whether a later pack refresh or sale gets rid of the amulet, and how often a carried backup and a home item share a high resist in real games. The record is re-read from the game at the start of every borg_think(), so the loss lasts one store turn; the measured harm is a redundant backup carried out of town (a pack slot and its weight) and two wasted store turns, not a lost game item. The sale-history entry in 'no-weapon-swap' is a disclosed fixture write.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
