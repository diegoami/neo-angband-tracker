# Excessive danger never made an ordinary Borg flee

This is **dangerflee-156 / issue156 (CBUG-0140)**, an escape-decision
defect in the C Borg, corrected in the direction the owner chose on the
issue.

`borg_caution()` has an "Excessive danger" check: when the danger at the
Borg's grid is more than twice its hit points it starts fleeing. Its
comment reads "do not flee level if going after Morgoth or fighting a
unique". The condition instead required `borg.ready_morgoth == 1`, which
is only true once the Borg is ready to face Morgoth — so ordinary Borgs
never fled here, and only the Morgoth-ready Borg did.

Owner decision (issue comment): match the comment.

```c
&& (borg.trait[BI_CDEPTH] < 100 && borg.ready_morgoth != 1)) {
```

Starting to flee early matters because several later escapes in
`borg_caution()` apply only to a fleeing Borg; for example the dash for a
staircase a step or two away requires `fleeing` (or a scary monster) once
the character is level 35 or higher.

## What to watch

| Scenario | Original | Corrected |
| --- | --- | --- |
| Level 40, 8 HP, cave bear adjacent, staircase 2 steps west | no excessive-danger flee; teleport fails; **reads Word of Recall** beside the bear | **"# Fleeing (excessive danger)"**, **"Desperate for Stairs (two)"**, step west (key 52) |
| Same, marked ready for Morgoth (intended reversal) | flees and dashes for the staircase | no excessive-danger flee; reads Word of Recall |
| Control: 15 HP (danger between 1x and 2x HP) | reads Word of Recall | byte-identical log |

Fixture: Native Human Warrior, seed 156, level 40, depth 30, starting kit
(no teleport, one Word of Recall), a lit room, an awake cave bear adjacent
(the Borg's danger estimate is 20), an up staircase two steps west, tracked
by the Borg. The "ready for Morgoth" case sets `borg.ready_morgoth = 1` at
depth 30, which a real game would not do; it is there only to show the
reversal the comment asks for. The production `borg_caution()` runs once.

[replay.html](replay.html): choose a scenario, then **Show decision** and
**Show raw logs**.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dangerflee-156
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dangerflee-156-evidence
git show FETCH_HEAD:docs/audit/c-reports/dangerflee-156/reproduce.py > /tmp/reproduce-dangerflee-156.py
python3 /tmp/reproduce-dangerflee-156.py --full-tests --fix-ref 007d45483832b2792ddde1b2c368b67975365496
```

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `007d45483` (branch
  `task/dangerflee-156`), fixtures `809e556b3` (branch
  `task/dangerflee-156-evidence`); the native run executed at the fixtures
  pin, the later commit only adds these captured artifacts
- before: excessive 0/1 (Recall read), morgoth 0/1 (flees); control 1/1
- after: excessive 1/1 (flee + stair dash), morgoth 1/1 (no flee);
  control byte-identical
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Playwright Chromium headless, no console errors

## Scope and limits

The fixture isolates one escape decision and does not play out the turns
that follow, so no survival claim is made. The change makes ordinary Borgs
flee more often whenever danger exceeds twice their hit points; its effect
on overall play (level-bouncing, progress) was not measured. The
owner-reviewed D0008 trace on the issue (a level-1 Priest on 2 HP facing
danger 16, `ready_morgoth` 0, not fleeing, who died three turns later) is
a natural case where this branch was skipped; changing this alone is not
shown to rescue it.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`
(+ `profile/borg.txt`) are the generators. `before-*.log`, `after-*.log`,
`*-results.json`, `result.json`, `run-*.log`, `replay.html`,
`replay-data.json` and `receipt.json` (hashes of every file) are the
captured run.
