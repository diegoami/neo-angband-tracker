# A lightning-resisting swap weapon counted as acid resistance

This is **swapacid-199 / issue199**, a gear-reading defect in the C Borg.

A deep Borg (maximum depth 50 or more) keeps a second weapon in its pack and
swaps to it when it needs that weapon's protection. When the Borg records
what its chosen swap weapon protects against, it sets the **acid** entry
from the weapon's **lightning** resistance. Two things go wrong:

- a swap weapon that resists only lightning is counted as acid protection,
  so the Borg's depth-readiness check — which demands acid protection from
  depth 26 down — says the Borg is ready although nothing it owns resists
  acid;
- a swap weapon that really resists acid is not counted, so the Borg keeps
  saying it is not ready for those depths.

The correction reads the acid entry from the acid resistance, exactly as the
swap scan earlier in the same function and the armour-swap code already do:

```c
if (item->el_info[ELEM_ACID].res_level > 0)
    weapon_swap_resist_acid = true;
```

## What to watch

| Scenario (swap weapon in the pack) | Original | Corrected |
| --- | --- | --- |
| Dethanc — resists lightning only | acid counted **1**; depth-30 check: **ready** | acid counted **0**; check: **resist acid** |
| Angrist — resists acid | acid counted **0**; check: **resist acid** | acid counted **1**; check: **ready** |
| Control: Narthanc — resists fire only | acid 0; resist acid | identical log |
| Control: no swap weapon | acid 0; resist acid | identical log |

The two control logs are byte-identical before and after.

Fixture: Native Human Warrior, seed 199, character level 50, maximum depth
50, standing on depth 30. Worn: Ring of Resist Fire and Cold, Ring of Free
Action, Amulet of ESP, Metal Scale Mail of Resist Lightning, a Long Sword
and a lantern; carried: food, Word of Recall, Phase Door and Teleportation
scrolls, Cure Light Wounds potions. Nothing worn resists acid. All runes are
learned, so the Borg sees each item's real properties. The production
notice pass picks the swap weapon and `borg_prepared(30)` runs once (static
preparedness path).

[replay.html](replay.html) is the offline page for the captured run: choose a
scenario, then **Show decision** (the two cards show whether acid resistance
is counted, and the readiness answer) and **Show raw logs**.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/swapacid-199
git fetch https://github.com/diegoami/neo-angband-tracker.git task/swapacid-199-evidence
git show FETCH_HEAD:docs/audit/c-reports/swapacid-199/reproduce.py > /tmp/reproduce-swapacid-199.py
python3 /tmp/reproduce-swapacid-199.py --full-tests --fix-ref eddbf5b82293ea70a76fae960bc9f8debf1579c6
```

The first fetch brings the fix commit; the second sets `FETCH_HEAD` to the
evidence revision. `--fix-ref` is required: the runner asserts the correction
is exactly the acid-element read in the single touched file, checks every
case's expected before/after result and that both control logs are
byte-identical, runs both full native gates and generates `replay.html`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `eddbf5b82` (branch
  `task/swapacid-199`), fixtures `9eca003a5` (branch
  `task/swapacid-199-evidence`); the native run executed at the fixtures
  pin, the later commit only adds these captured artifacts
- before: Dethanc counted as acid (ready), Angrist not counted (resist acid)
  — both cases fail; controls pass
- after: all four cases pass; controls byte-identical
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Playwright Chromium headless, no console errors

## Scope and limits

The fixture isolates the swap reading and the readiness answer for one
character. It shows the Borg would consider itself ready for depths 26–39
with no real acid protection (and not ready with real protection); it does
not measure an autonomous dive, an acid attack or a death. Swap weapons are
only kept from maximum depth 50, so the defect affects deep characters only.
The fixture learns all runes and places items directly; it does not set any
Borg trait or flag.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`
(+ `profile/borg.txt`) are the generators. `before-*.log`, `after-*.log`,
`*-results.json`, `result.json`, `run-*.log` (configure, build, scenario and
full-gate logs), `replay.html`, `replay-data.json` and `receipt.json`
(hashes of every file) are the captured run.
