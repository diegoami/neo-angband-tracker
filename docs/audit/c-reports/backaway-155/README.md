# One bad neighbour stopped the Borg from backing away

This is **backaway-155 / issue155 (CBUG-0139)**, an escape-decision defect
in the C Borg.

When a monster threatens a hurt Borg, the "Back away" step of
`borg_caution()` looks at the eight grids around it for a safer one to step
to. The grids are checked in a fixed order (south first). Two slips cut that
search short:

- `adjacent_monster` was set inside the loop and never cleared, so once one
  candidate grid was next to a monster, **every later candidate** was also
  treated as next to a monster and skipped;
- a known trap on one candidate ended the **whole** search (`break`)
  instead of skipping just that grid.

The correction clears the flag for each candidate and skips a trapped grid
with `continue`.

## What to watch

| Scenario | Original | Corrected |
| --- | --- | --- |
| South neighbour next to the cave bear | no retreat (caution does nothing) | **"# Backing up to 9,11"**, step north-west (key 55) |
| Known pit on the south neighbour | no retreat | **"# Backing up to 9,11"** |
| Control: full hit points | no retreat (below threshold) | byte-identical log |

Fixture: Native Human Warrior, seed 155, level 20, depth 5, starting kit, a
lit room with no staircases. An awake cave bear stands two steps away (the
Borg's danger estimate at its grid is 20; the back-away step needs danger
above 40% of the hit points, here 40). The Borg has been on the level 1000
game turns and no anti-summon corridor is in use. In the trap case the bear
stands south-south-west and a pit is placed (after the bear, so the bear's
random roll is the same) on the grid south of the Borg; the Borg's map
records the trap. The production `borg_caution()` runs once.

[replay.html](replay.html): choose a scenario, then **Show decision** and
**Show raw logs**.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/backaway-155
git fetch https://github.com/diegoami/neo-angband-tracker.git task/backaway-155-evidence
git show FETCH_HEAD:docs/audit/c-reports/backaway-155/reproduce.py > /tmp/reproduce-backaway-155.py
python3 /tmp/reproduce-backaway-155.py --full-tests --fix-ref df5361ad5cc3977c75a86061ad0d944568885d77
```

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `df5361ad5` (branch
  `task/backaway-155`), fixtures `fa0293e58` (branch
  `task/backaway-155-evidence`); the native run executed at the fixtures
  pin, the later commit only adds these captured artifacts
- before: monster 0/1, trap 0/1 (no retreat); control 1/1
- after: monster 1/1, trap 1/1 (backs up to 9,11); control byte-identical
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Playwright Chromium headless, no console errors

## Scope and limits

The fixture isolates the retreat decision for one turn; it does not play
out the fight, so no survival claim is made. Each scenario exercises one of
the two slips on its own (the trap case has no grid next to the bear before
the trap; the monster case has no trap). How often real fights hit this
order of neighbours was not measured.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`
(+ `profile/borg.txt`) are the generators. `before-*.log`, `after-*.log`,
`*-results.json`, `result.json`, `run-*.log`, `replay.html`,
`replay-data.json` and `receipt.json` (hashes of every file) are the
captured run.
