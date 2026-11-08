# Glowing floors were never flagged to the borg

This is **glowtrace-213 / issue213**, a map-update bookkeeping defect found by
code reading (borg-rs conversion review; no native measurement existed). The
map update's Floors branch tested `g.f_idx == FEAT_NONE` — the same condition
as the Darkness branch before it in the same `else if` chain — so it was never
taken: the game's permanent lighting never reached the borg's grid flags
(`BORG_GLOW`/`BORG_DARK`/`BORG_LIGHT`), and the flow-toward-light consumer
always found an empty target list.

The condition is a transcription slip from the original borg's `switch`,
which cased `FEAT_FLOOR` here (verified against the 2011 import at
angband commit `a50705002`; the slip entered with the 2023 "Borgsource"
reorg). Upstream master still carries the dead branch today, so this fix has
no upstream counterpart to adopt.

The correction restores the `FEAT_FLOOR` condition and reads the glow state
from `square_islit()` — the grid's permanent-glow flag, matching the
`CAVE_GLOW` test the original branch performed. The `grid_data` lighting enum
cannot express glow: `LIGHTING_LIT` is `map_info`'s default for grids whose
lighting is not otherwise established, so the transcribed
`g.lighting == LIGHTING_LIT` test would have marked remembered dark floors as
glowing. Since the branch never executed at the base, every behaviour the
corrected branch adds is new and limited to floor grids' lighting flags;
walls, doors, traps and unknown grids keep their existing handling.

## What to watch

| Version | Distant glowing floor | Adjacent glowing floor | Perm wall (control) | Flow-toward-light targets |
| --- | --- | --- | --- | --- |
| Original | not flagged | not flagged | not flagged | **0 (empty list)** |
| Corrected | **flagged BORG_GLOW** | **flagged BORG_GLOW** | not flagged | **484** |

Fixture: Human Warrior, seed 213, depth 9, a uniformly permanently-glowing
arena with perm walls, no monsters, no ailments. The Borg views the whole
arena; the production `borg_update()` (including the map update) runs once,
and grid flags plus the consumer state are read back directly.

[replay.html](replay.html) is the offline page for the captured run.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowtrace-213
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glowtrace-213-evidence
git show FETCH_HEAD:docs/audit/c-reports/glowtrace-213/reproduce.py > /tmp/reproduce-glowtrace-213.py
python3 /tmp/reproduce-glowtrace-213.py --full-tests --fix-ref da9934216eb15d421220050b374c72b65b9cd680
```

The first fetch brings the fix commit; the second sets `FETCH_HEAD` to the
evidence revision. `--fix-ref` is required: the runner pins the corrected
source by that commit and asserts the correction is exactly the Floors branch
restoration in the single touched file.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `da9934216` (branch
  `task/glowtrace-213`), fixtures `5edc188e` (branch
  `task/glowtrace-213-evidence`)
- before: suite 0/1 — `distant_glow=0 near_glow=0 wall_glow=0` (lit=1 in the
  game, unmarked in the borg), flow-light list empty
- after: suite 1/1 — `distant_glow=1 near_glow=1 wall_glow=0`, flow-light
  list 484 targets
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Chrome headless

## Scope and limits

The fixture isolates the map-update flag stage and one consumer
(`borg_flow_light`). It does not measure autonomous play, encounter outcomes
or the other consumers (`borg-fight-defend.c` unlit-Borg checks,
`borg-commands.c` rest-site ladder), which read the same flags and gain the
same corrected data. No survival or encounter-frequency claim.
