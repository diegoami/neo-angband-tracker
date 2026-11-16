# A hurt Borg on a down staircase did not take it

This is **dnstair-615**, a C Borg defect (diegoami/neo-angband-tracker#615). It was first seen in the #152 triage.

**The behaviour.** When `borg_caution()` finds the Borg in danger on an **up** staircase, it leaves the level by it. On a **down** staircase, danger alone did nothing: the Borg took the stairs only when fleeing or prepared for the next depth (`borg_prepared(depth + 1)`). So a hurt, unprepared Borg tried to teleport and, failing that, read Word of Recall. Recall takes 15+ turns to work, and the Borg stayed next to the monster meanwhile, although the stairs work at once.

**The correction** is in `src/borg/borg-caution.c`. On a down staircase, with the same danger test as for up staircases (danger above avoidance, or above half of it below character level 5), the Borg may now go one level deeper. This is a behaviour choice: the existing rule "only go down if fleeing or prepared" is relaxed only for this case.

## What to watch

The first key `borg_caution()` queues.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `142ed35fd`) |
| --- | --- | --- |
| In danger on a down staircase | failed: "Fleeing (failed to teleport)", **reads Word of Recall** | passed: **takes the stairs** (`>`) |
| Control: in danger on an up staircase | passed: takes the stairs (`<`) | byte-identical log |
| Control: on a down staircase, no monster | passed: stays | byte-identical log |

## Conditions

- Character: native Human Warrior, seed 615, character level 5, dungeon level 5, 15 of 56 hit points, starting kit (no teleport; Word of Recall carried).
- Arena: a lit 38x22 room, with the shipped `src/borg/borg.txt` of the base revision.
- Staircase: the Borg stands on one it has seen (down, or up in the up control).
- Monster: a cave bear stands awake next to it (south-east), except in the safe control. The Borg's danger estimate is 40, against an avoidance of 15.
- Run: the production `borg_caution()` runs once, and the first queued key is read.

[replay.html](replay.html) is the offline page (download it raw and open it locally). Choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dnstair-615
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dnstair-615-evidence
git show a0c25b35f91a8456e52c44077f2a179ca67d5c70:docs/audit/c-reports/dnstair-615/reproduce.py > /tmp/reproduce-dnstair-615.py
python3 /tmp/reproduce-dnstair-615.py --full-tests --fixtures-ref a0c25b35f91a8456e52c44077f2a179ca67d5c70 --fix-ref 142ed35fd7f4d90fe0dca147976934943d53afb9
```

The runner:
- asserts that the correction is exactly `fix.patch` (`src/borg/borg-caution.c` only);
- builds the original and corrected native C game in a disposable worktree;
- runs the three scenarios and checks each expected result and both byte-identical controls;
- runs both full native gates and writes `replay.html`.

Heavy steps wait on `/tmp/angband-validation.lock`. The frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master)
- fix `142ed35fd7f4d90fe0dca147976934943d53afb9` (branch `task/dnstair-615`)
- fixtures `a0c25b35f91a8456e52c44077f2a179ca67d5c70` (branch `task/dnstair-615-evidence`); later commits only add these artifacts
- byte-identical controls: up, safe
- full native gates: 965/965 unit checks and 5/5 frontend tests, before and after
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

In danger on a down staircase, the corrected Borg takes it at once instead of reading Word of Recall next to the monster. Up staircases and a safe down staircase are unchanged.

Not measured:
- whether the next level is safer for the Borg, and survival in full play;
- the interaction with a teleport the Borg could still use. The stair block runs before the escape code, so with the correction the stairs win over teleport when standing on them, as they already do for up staircases.

## Files

- Generators: `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`).
- Captured run: `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file).
