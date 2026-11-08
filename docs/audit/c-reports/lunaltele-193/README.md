# A monster teleport did not end the Borg's Lunal mode

This is **lunaltele-193 / issue193**, a C Borg defect
(diegoami/neo-angband-tracker#193).

Lunal mode is a Borg setting (borg_lunal_mode in borg.txt, or engaged by the Borg itself when it is far stronger than the level) in which the Borg bounces between staircases to dive quickly. While it is on, the Borg skips its usual safety checks: it walks without checking how dangerous each grid is, it refuses to rest, and it takes the stairs down even when it is not prepared for the next depth. The Borg's code intends to switch Lunal mode off when a monster spell moves it away from its grid ("he is not on the stairs anymore"), but that check never fires: every monster spell line is recognised earlier as a spell reaction. In this run a tengu commands the Borg to return (and, separately, a mage commands it to go away). The original Borg stays in Lunal mode and immediately starts walking to the down staircase, although its own preparedness check says it is not ready for depth 11 ("restock phase door"). After the fix the Borg leaves Lunal mode when the teleport reaction is handled, and the same stair decision refuses to go down. A spell that does not move the Borg (a shrieker's shriek) leaves Lunal mode on in both versions.

## What to watch

Borg starts walking to the down staircase to depth 11, for which it is not prepared (1 yes, 0 no); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `4ee9a38f3`) |
| --- | --- | --- |
| Tengu: 'The tengu commands you to return.' | failed, heads_down=1 | passed, heads_down=0 |
| Mage: 'The mage commands you to go away.' | failed, heads_down=1 | passed, heads_down=0 |
| Unseen caster: 'Something commands you to go away.' | failed, heads_down=1 | passed, heads_down=0 |
| Control: 'The shrieker mushroom patch makes a high-pitched shriek.' | passed, heads_down=1 | byte-identical log |
| Control: tengu teleport-to with Lunal mode not in effect | passed, heads_down=0 | byte-identical log |

- **Tengu: 'The tengu commands you to return.'.** The tengu's teleport-to moves the Borg from (10,12) to (12,12), next to the tengu. Original: Lunal mode stays on (lunal_mode after=1) and the stair decision starts walking to the down staircase although the Borg is not prepared for depth 11. Corrected: Lunal mode is switched off when the SPELL_068 reaction is handled, and the same decision refuses the descent.

- **Mage: 'The mage commands you to go away.'.** The mage's teleport-away moves the Borg from (10,12) to (38,1). Original: Lunal mode stays on and the Borg heads for the down staircase. Corrected: Lunal mode is off after the SPELL_070 reaction and the descent is refused.

- **Unseen caster: 'Something commands you to go away.'.** The same teleport-away, cast as by a monster the player cannot see. The Borg recognises it as SPELL_070:Something. Original: stays in Lunal mode and heads down. Corrected: Lunal mode off, descent refused.

- **Control: 'The shrieker mushroom patch makes a high-pitched shriek.'.** A monster spell that does not move the Borg goes through the same spell-reaction code. Lunal mode stays on and the Borg heads for the down staircase in both versions; byte-identical logs.

- **Control: tengu teleport-to with Lunal mode not in effect.** The same tengu teleport-to with Lunal mode already off. Both versions refuse the descent because the Borg is not prepared for depth 11; byte-identical logs.

## Conditions

Native Human Warrior, seed 193, character level 15, depth 10, lit open 24x40 arena with one down staircase at (24,18) and one up staircase at (3,3); profile borg.txt with borg_lunal_mode = TRUE (the configured Lunal mode, read by borg_init). Real code: the game's monster spell code (do_mon_spell with the game's teleport-to/teleport-away/shriek effects, which really move the player), the Borg's production message parser (borg_parse), its map/message update (borg_update, three passes), borg_notice, borg_prepared and the production stair decision borg_flow_stair_more(GOAL_BORE, false, false) as called from borg_leave_level when the Borg is bored on a level. Fixture interventions: the arena, stairs and caster are placed directly; the spell is cast by calling do_mon_spell directly (the monster's own spell choice is not run; the tengu and mage have the cast spell in their stock spell lists, the shrieker has SHRIEK); an EVENT_MESSAGE handler records each line the game prints and hands it to borg_parse instead of the Borg scraping it from the screen; in teleaway-unseen the spell is cast as by an unseen caster (seen=false) so the game prints its 'Something ...' line; in teleto-nolunal the fixture sets borg.lunal_mode = false after borg_init.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lunaltele-193
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lunaltele-193-evidence
git show f781e1389112154fe13891482409506b81cd57f4:docs/audit/c-reports/lunaltele-193/reproduce.py > /tmp/reproduce-lunaltele-193.py
python3 /tmp/reproduce-lunaltele-193.py --full-tests --fixtures-ref f781e1389112154fe13891482409506b81cd57f4 --fix-ref 4ee9a38f305d0e44624efdd45c5358d784852754
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-update.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `4ee9a38f305d0e44624efdd45c5358d784852754` (branch `task/lunaltele-193`), fixtures
  `f781e1389112154fe13891482409506b81cd57f4` (branch `task/lunaltele-193-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: shriek, teleto-nolunal
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_parse_aux() queues every monster spell line as SPELL_nnn and returns (borg-messages.c:636-658), so the Lunal-off test at borg-messages.c:1281-1289 (which also uses prefix() on lines that start with the monster's name) never runs. borg_update() now clears borg.lunal_mode when it handles a SPELL_ reaction for RSF_TELE_TO or RSF_TELE_AWAY. With Lunal mode off, borg_flow_stair_more() applies its preparedness filter (borg-flow-stairs.c:196) and refuses the descent; with it on, the filter is skipped.

Measured: the Lunal flag and one production stair decision (borg_flow_stair_more as called from borg_leave_level) right after a real teleport line. Not measured: a full game loop, deaths, or the other Lunal effects (flow danger checks at borg-flow.c:386/440/549 and the rest refusal at borg-flow-misc.c:266), which follow the same flag. Whether staying in Lunal mode after a teleport is harmful depends on the player's intent: configured Lunal mode is a deliberate dive setting, and after this fix it ends for the rest of the session at the first teleport-to or teleport-away (owner-accepted consequence). Self-engaged Lunal mode (borg_self_lunal) is re-engaged at the next dungeon think wherever its conditions still hold (borg-think-dungeon.c:1437-1455); that was not run here. Teleport-level is not included: it changes the level rather than moving the Borg on it, and was not tested. The unreachable string test in borg-messages.c is left unchanged.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
