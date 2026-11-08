# A far staircase hid the one two steps away

This is **deskstair-153 / issue153 (CBUG-0137)**, an escape-decision defect
in the C Borg.

When the Borg is in trouble (fleeing, a scary monster, or danger above its
avoidance at character level below 35) and a staircase is only a step or
two away, it dashes for it ("Desperation Head for stairs" in
`borg_caution()`). To decide, it looks up how far the nearest free up and
down staircases are. Both look-ups kept the **farthest** staircase
(`/* skip the closer ones */ if (b_j >= j) continue;`), so when the Borg
also knew a second staircase of the same kind far away, the staircase next
to it did not count and the dash was skipped.

In the recorded scenario a badly hurt level-5 Warrior with an awake cave
bear beside it and a staircase two steps away then fell through to its
other escapes: it has no teleport, so it **read Word of Recall** — which
only takes effect many turns later — and stayed next to the bear. With only
the near staircase on the map it dashes for it.

The correction keeps the nearest free staircase in both scans, matching
their comments ("How far is the nearest ...") and the closeness tests that
follow (`b_j < 3`, `b_j <= 2`, `b_j < 20`, `b_m <= 2`):

```c
/* skip the farther ones */
if (b_j <= j && b_j != -1)
    continue;
```

## What to watch

| Scenario | Original | Corrected |
| --- | --- | --- |
| Up staircases: one 2 steps west, one 25 steps east | reads **Word of Recall** (no dash) | **"Desperate for Stairs (two)"**, step west (key 52) |
| Down staircases: same layout | reads **Word of Recall** | **"Desperate for Stairs (four)"**, step west |
| Control: only the near up staircase | dashes for it | byte-identical log |

Fixture: Native Human Warrior, seed 153, level 5, depth 5, 15 of 86 hit
points, starting kit (no teleport, one Word of Recall), a lit 38x22 room. An
awake cave bear stands next to the Borg; the Borg's danger estimate at its
grid is 20, above its 15 hit points. The staircases are placed in the game
map and tracked by the Borg. The production `borg_caution()` runs once and
the first queued key is read back.

[replay.html](replay.html): choose a scenario, then **Show decision** and
**Show raw logs**.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/deskstair-153
git fetch https://github.com/diegoami/neo-angband-tracker.git task/deskstair-153-evidence
git show FETCH_HEAD:docs/audit/c-reports/deskstair-153/reproduce.py > /tmp/reproduce-deskstair-153.py
python3 /tmp/reproduce-deskstair-153.py --full-tests --fix-ref 62768100602b693ed5845f0e5749830563d7e3b3
```

The runner asserts the correction is exactly the two scan changes (and the
down-scan comment) in the single touched file, checks each expected result
and that the control log is byte-identical, runs both full native gates and
writes `replay.html`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `627681006` (branch
  `task/deskstair-153`), fixtures `366fd1b08` (branch
  `task/deskstair-153-evidence`); the native run executed at the fixtures
  pin, the later commit only adds these captured artifacts
- before: twostairs 0/1, twodown 0/1 (Word of Recall read); control 1/1
- after: all three 1/1 (dash for the near staircase); control byte-identical
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Playwright Chromium headless, no console errors

## Scope and limits

The fixture isolates one escape decision. It does not play out the turns
that follow (whether the bear kills the Borg before the recall activates,
or whether the dash succeeds), so no death or survival difference is
claimed — only that the hurt Borg spends a Recall scroll and stays beside
the monster instead of stepping toward a staircase two squares away. How
often a real Borg knows a second staircase of the same kind (common on
explored levels) was not measured.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`
(+ `profile/borg.txt`) are the generators. `before-*.log`, `after-*.log`,
`*-results.json`, `result.json`, `run-*.log`, `replay.html`,
`replay-data.json` and `receipt.json` (hashes of every file) are the
captured run.
