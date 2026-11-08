# Waiting for recall, the low-mana caster never restored its mana

This is **recallmana-157 / issue157 (CBUG-0141)**, an escape-decision
defect in the C Borg.

When the Borg has read Word of Recall and is in danger while it waits, the
end of `borg_caution()` "buys time". Its first step restores mana when a
caster with more than 60 maximum mana is below a quarter of it, so it can
cast its escapes. The test compared the current mana with a quarter of the
**current** mana (`BI_CURSP < BI_CURSP / 4`), which is never true, so the
step never ran.

The correction compares with a quarter of the maximum mana:

```c
&& borg.trait[BI_CURSP] < (borg.trait[BI_MAXSP] / 4)
```

## What to watch

| Scenario | Original | Corrected |
| --- | --- | --- |
| 19 of 193 mana | no action from caution (rc 0) | **"Quaffing Potions of Restore Mana"**, "Buying time waiting for Recall.(1)" (key q) |
| Control: 96 of 193 mana | no action | byte-identical log |

Fixture: Native Human Mage, seed 157, level 35, depth 30, 9 hit points,
starting kit plus 2 Restore Mana potions, an awake cave bear adjacent (the
Borg's danger estimate is 20, above twice its hit points), an up staircase
two steps west. Word of Recall is active (the game's recall timer is set,
as after reading the scroll). The production `borg_caution()` runs once.

[replay.html](replay.html): choose a scenario, then **Show decision** and
**Show raw logs**.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/recallmana-157
git fetch https://github.com/diegoami/neo-angband-tracker.git task/recallmana-157-evidence
git show FETCH_HEAD:docs/audit/c-reports/recallmana-157/reproduce.py > /tmp/reproduce-recallmana-157.py
python3 /tmp/reproduce-recallmana-157.py --full-tests --fix-ref 57d0df0633d12064602993f39fe6a5ee5f62524c
```

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `57d0df063` (branch
  `task/recallmana-157`), fixtures `b6bd252e1` (branch
  `task/recallmana-157-evidence`); the native run executed at the fixtures
  pin, the later commit only adds these captured artifacts
- before: lowmana 0/1 (no action); control 1/1
- after: lowmana 1/1 (Restore Mana quaffed); control byte-identical
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Playwright Chromium headless, no console errors

## Scope and limits

The Borg is not given its spells in this fixture, so it cannot cast Phase
Door in either version; with spells available the original would go on to
its later "buy time" steps (phase, cures). The fixture isolates the first
step and shows that it never ran; it does not show that restoring mana is
better than the alternatives, or play out the recall. That ordering is
the code's own design.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`
(+ `profile/borg.txt`) are the generators. `before-*.log`, `after-*.log`,
`*-results.json`, `result.json`, `run-*.log`, `replay.html`,
`replay-data.json` and `receipt.json` (hashes of every file) are the
captured run.
