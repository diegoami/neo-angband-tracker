# A respawning Borg got the wrong race or class

This is **rebirthkey-52 / issue #52**, a C Borg defect
(diegoami/neo-angband-tracker#52).

When the Borg dies with the game's 'cheat_live' option on (and its own borg_cheat_death off), or wins with borg_respawn_winners on, it starts a new character by typing keys into the normal birth screens: it picks a race and a class (chosen at random, or fixed by borg_respawn_race / borg_respawn_class in borg.txt) by typing that entry's menu letter. The Borg typed letters from the plain a-z table, but the birth menus label their entries with a table that skips h, j, k and l. From the eighth entry on the Borg typed the wrong letter. Wanting a Dunadan it typed 'i', which is the Half-Troll's letter, and got a Half-Troll; wanting a Blackguard it got a Ranger. Wanting a Half-Troll (or a High-Elf or Kobold) it typed a letter the menu does not have, so the race already highlighted was kept: the new character stayed Human. The random name is still made for the race wanted, so a Half-Troll could get a human name and a Human an orc name. The correction types the same letters the menus use, and the Borg gets the character it chose.

## What to watch

Character created as wanted (1 = race and class both match); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `85e7ce6c6`) |
| --- | --- | --- |
| Wanted Dunadan Warrior (race index 8), previous Human Warrior | failed, right_character=0 | passed, right_character=1 |
| Wanted Human Blackguard (class index 8), previous Human Warrior | failed, right_character=0 | passed, right_character=1 |
| Wanted Half-Troll Warrior (race index 7), previous Human Warrior | failed, right_character=0 | passed, right_character=1 |
| Control: wanted Dwarf Priest (race 5, class 3), previous Human Warrior | passed, right_character=1 | byte-identical log |
| Control: wanted Human Warrior (race 0, class 0), previous Elf Mage | passed, right_character=1 | byte-identical log |

- **Wanted Dunadan Warrior (race index 8), previous Human Warrior.** Original: the Borg types 'i' for race 8, which is the Half-Troll's letter in the birth menu, and a Half-Troll Warrior is created (with a name made for a Dunadan). Corrected: it types 'm' and a Dunadan Warrior is created.

- **Wanted Human Blackguard (class index 8), previous Human Warrior.** Original: the Borg types 'i' for class 8, which is the Ranger's letter, and a Human Ranger is created. Corrected: it types 'm' and a Human Blackguard is created.

- **Wanted Half-Troll Warrior (race index 7), previous Human Warrior.** Original: the Borg types 'h', which the birth menu does not use; the highlighted race (Human) is kept and a Human Warrior with an orc name is created. Corrected: it types 'i' and a Half-Troll Warrior is created.

- **Control: wanted Dwarf Priest (race 5, class 3), previous Human Warrior.** Below index 7 both tables give the same letters ('f' and 'd'); both versions create a Dwarf Priest and the logs are identical.

- **Control: wanted Human Warrior (race 0, class 0), previous Elf Mage.** Letters 'a' and 'a' in both tables; both versions create a Human Warrior and the logs are identical.

## Conditions

Native build with the Borg, seed 52. A previous Human Warrior (Elf Mage for one control) is made natively; the game option cheat_live is on and borg_cheat_death is off, so a fatal hit asks 'Die?'. The respawn race and class are set as borg_respawn_race / borg_respawn_class would set them. A fatal hit is dealt through take_hit(); from then on every key is produced by the Borg's own keypress hook: it answers 'Die?', queues the respawn keys (borg_reincarnate_start), and those keys are consumed by the real death menu (death_screen: New Game, confirm) and the real birth screens (textui_do_birth: quick-start prompt, race, class, roller, point-based stats, name, history, confirm). Fixture interventions: the previous character and the fatal hit are synthetic; the savefile save and the game re-initialisation that close_game()/play_game() do between the death menu and the birth screens are skipped; no message window is attached, so no -more- prompts appear. The race-menu highlight therefore starts on the previous character's race.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/rebirthkey-52
git fetch https://github.com/diegoami/neo-angband-tracker.git task/rebirthkey-52-evidence
git show 10140a539877c0a96a98df231ce150394485866f:docs/audit/c-reports/rebirthkey-52/reproduce.py > /tmp/reproduce-rebirthkey-52.py
python3 /tmp/reproduce-rebirthkey-52.py --full-tests --fixtures-ref 10140a539877c0a96a98df231ce150394485866f --fix-ref 85e7ce6c689e05bc209feda6d30a3bba51c533c6
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-reincarnate.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `85e7ce6c689e05bc209feda6d30a3bba51c533c6` (branch `task/rebirthkey-52`), fixtures
  `10140a539877c0a96a98df231ce150394485866f` (branch `task/rebirthkey-52-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: dwarfpriest, humanwarrior
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_reincarnate_start() types all_letters[race] and all_letters[class], but the birth menus are labelled with all_letters_nohjkl. Race/class index 7 types a letter the menu lacks (the highlighted entry is kept), index 8 and up select the entry before the one wanted (Dunadan becomes Half-Troll, Blackguard becomes Ranger). Typing all_letters_nohjkl restores the intended character; index < 7 controls are byte-identical.



## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
