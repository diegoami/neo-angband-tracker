# The Borg ignored four monster status messages

This is **painmsg-197 / issue197**, a C Borg defect
(diegoami/neo-angband-tracker#197).

When the game reports a change in a monster near the Borg ("The kobold is no longer stunned.", "The kobold grunts with pain."), the Borg's message reader recognises the line and notes it against that monster on its own monster list. It keeps a table of these message endings, built when the Borg starts. Each entry is copied into a buffer one byte too short whenever the ending has no [singular|plural] markup, so four endings lost their final '.' or '!': "can move again." (a held monster is free again, for example after the Borg's Wand of Hold Monster wears off), "falls under your spell!", "is no longer under your control." and "shimmers for a moment.". The game's real lines never matched them, and the Borg ignored those lines. After the fix it recognises them and matches them to the monster, exactly as it already did for "is no longer stunned." and for pain messages.

## What to watch

Game message recognised as news about the kobold (1 yes, 0 no); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `8a0c13f71`) |
| --- | --- | --- |
| Held monster is free again: 'The kobold can move again.' | failed, recognised=0 | passed, recognised=1 |
| 'The kobold falls under your spell!' | failed, recognised=0 | passed, recognised=1 |
| 'The kobold is no longer under your control.' | failed, recognised=0 | passed, recognised=1 |
| 'The kobold shimmers for a moment.' | failed, recognised=0 | passed, recognised=1 |
| Control: 'The kobold is no longer stunned.' | passed, recognised=1 | byte-identical log |
| Control: pain message 'The kobold grunts with pain.' | passed, recognised=1 | byte-identical log |
| Not changed: 'There is a puff of smoke!' | failed, recognised=0 | failed, recognised=0 |

- **Held monster is free again: 'The kobold can move again.'.** The game holds the kobold, then ends the hold and prints 'The kobold can move again.'. Original: no reaction queued; the line is ignored. Corrected: reaction 'PAIN:The kobold' queued and matched to the kobold on the Borg's monster list in the next map update.

- **'The kobold falls under your spell!'.** The game puts the kobold under the player's command and prints 'The kobold falls under your spell!'. Original: ignored. Corrected: recognised and matched to the kobold.

- **'The kobold is no longer under your control.'.** The command effect ends and the game prints 'The kobold is no longer under your control.'. Original: ignored. Corrected: recognised and matched to the kobold.

- **'The kobold shimmers for a moment.'.** The game's monster-message code prints the failed-shapechange line 'The kobold shimmers for a moment.'. Original: ignored. Corrected: recognised and matched to the kobold.

- **Control: 'The kobold is no longer stunned.'.** Same sequence with the stun effect, whose ending has [is|are] markup and was always stored whole: recognised and matched in both versions; byte-identical logs.

- **Control: pain message 'The kobold grunts with pain.'.** The game's pain message after 3 damage, from the pain message table: recognised and matched in both versions; byte-identical logs.

- **Not changed: 'There is a puff of smoke!'.** The fifth ending named in the report. The game prints it without a monster name; the Borg stores it with a leading space, which the printed line does not have, so it is not recognised before or after this fix (and there is no monster name to note). Byte-identical logs; outside this fix.

## Conditions

Native Human Warrior, seed 197, character level 11, depth 5, lit open 24x40 arena; a kobold two grids east, seen and on the Borg's monster list. Real code: the game's monster timed-effect code (mon_inc_timed, mon_clear_timed), its monster-message code (add_monster_message, show_monster_messages, message_pain) and the Borg's production message parser (borg_parse) and map update (borg_update). Fixture interventions: the arena and the kobold are placed directly; an EVENT_MESSAGE handler records each line the game prints and hands it to borg_parse (the function the Borg's screen reader calls with each message line), instead of the Borg scraping the line from the screen; the shape-change and puff-of-smoke lines are requested from the game's monster-message code directly (the shapechange itself and a hit-and-run blow are not run); the 3 damage in the pain control is subtracted directly before the game's pain message is printed.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/painmsg-197
git fetch https://github.com/diegoami/neo-angband-tracker.git task/painmsg-197-evidence
git show ba2167d51fcdd99a957b9ed53f65c676ed4b4bff:docs/audit/c-reports/painmsg-197/reproduce.py > /tmp/reproduce-painmsg-197.py
python3 /tmp/reproduce-painmsg-197.py --full-tests --fixtures-ref ba2167d51fcdd99a957b9ed53f65c676ed4b4bff --fix-ref 8a0c13f7127642da8222c84380b4e6b8f9f96eff
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-messages.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `8a0c13f7127642da8222c84380b4e6b8f9f96eff` (branch `task/painmsg-197`), fixtures
  `ba2167d51fcdd99a957b9ed53f65c676ed4b4bff` (branch `task/painmsg-197-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: stun, hurt
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_get_parsed_pain() allocates strlen(pain) + 1 bytes for ' ' + text + NUL, and its copy loop stops at pos < buflen - 1, so text without [|] markup loses its final character. The suffix_pain entries for NOT_HELD, COMMAND, NOT_COMMAND and SHAPE_FAIL therefore lacked their final punctuation and suffix() never matched the printed line. With strlen + 2 the entries are complete: the four real messages queue 'PAIN:<monster>' and borg_update matches it to the monster; bracketed endings (stun) and pain.txt messages are unchanged.

Measured: whether the Borg recognises each real line and matches it to the monster on its list. Not measured: any change in what the Borg does next. A recognised line only re-locates the monster in the Borg's monster list (and, for unseen or off-screen monsters, notes an invisible monster or shifts the screen panel); the Borg does not estimate monster health from these lines, and no attack, flee or spell choice changed in this fixture. Held monsters are reachable in ordinary play (the Borg aims Wands of Hold Monster); the command lines come from the Necromancer command spell; shapechange failures are rare. The fifth ending, 'There is a puff of smoke!', is still not recognised after this fix, for a different reason (leading space on a line with no monster name); it is outside this fix.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
