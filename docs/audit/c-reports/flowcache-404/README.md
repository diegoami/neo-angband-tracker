# A retreat left a dangerous grid marked "checked"

This is **flowcache-404 / issue [#404](https://github.com/diegoami/neo-angband-tracker/issues/404)**,
the Borg's flow danger cache. Origin: **C Borg**, pristine
`e473720ed88a0f0c7ca66815cef3000eb4a38bc3` (consumer case: better-borg-c D0008).

## In plain words

When the Borg plans a walk it checks how dangerous each square is and refuses
squares that are too dangerous. It remembers "I already checked this square" so
it does not repeat the work. But when the Borg is in an emergency mode
(retreating, lunal, munchkin, digging) it deliberately **skips** the danger
check, yet it still wrote "already checked" into the memory. Afterwards, back in
normal mode, the next walk found that note, trusted it, and walked straight
through a square that had never been checked. In the real game this sent a
priest back next to a grey mold five times in a row.

The correction writes "already checked" only when the check was really made.

**Play it:** download [play.html](play.html) (one self-contained file; open it in any browser, no network needed). It steps through frames recorded from the real game code, original and corrected side by side, for the retreat-then-explore walk (mold visible, remembered or forgotten) and for each of the four emergency modes.

## What this evidence shows, and what it does not

Shown, in native C, from the original game code:

- The memory entry is written without a danger check, in all four modes and in both
  places that write it (`borg_flow_spread`, `borg_flow_enqueue_grid`).
- A later normal-mode flow reuses it and routes through a square whose danger is
  16 against a limit of 1. With the correction it does not.
- The same happens through the production path: the real `borg_caution()` picks the
  retreat ("Desperate for Stairs"), then the real `borg_flow_dark()` steps back
  toward the mold. With the correction `borg_flow_dark()` finds no step.
- Safe flows, genuinely dangerous flows, blocked routes and emergency retreats behave
  identically before and after.

**Not shown:** that this priest (D0008) would have survived, or that overall
survival improves. The fix removes one wrong decision; later decisions, RNG and
other faults (source #156 low-HP guard, #152 staircase flags) are separate and are
not part of this change.

## Repeat it

From a clone of `diegoami/neo-angband-tracker`:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/c-borg-flow-danger-cache
git fetch https://github.com/diegoami/neo-angband-tracker.git task/flowcache-404-evidence
git show FETCH_HEAD:docs/audit/c-reports/flowcache-404/reproduce.py > /tmp/reproduce-flowcache-404.py
python3 /tmp/reproduce-flowcache-404.py --full-tests \
  --fixtures-ref 7f903a9a52172a943c8afabbda1311a2f1adb4d1 \
  --fix-ref b71b32c152e272886cb46bb8f253e8c4f5130711
```

`record-play.py` (same `--fixtures-ref` and `--fix-ref`, no `--full-tests`) records the frames behind `play.html`.

It builds the native game five ways from the pristine base, runs 30 native cases against each,
asserts the whole outcome matrix, runs both full regression gates (before and after), and writes
`result.json` and `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.
Prerequisites: Git, Python 3, CMake, a C toolchain, `flock`, X11 development headers.

## What was built and run (recorded receipt)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`; correction `b71b32c152e272886cb46bb8f253e8c4f5130711`
  (one file, `src/borg/borg-flow.c`, +19 −7); fixtures `7f903a9a52172a943c8afabbda1311a2f1adb4d1`
- Debug build, `-DSUPPORT_BORG=ON -DSUPPORT_TEST_FRONTEND=ON`. Fixture: Human Priest level 1, depth 1, seed 1000,
  one-wide corridor, up staircase at (13,7), grey mold in a niche at (15,8), Borg HP 6 (so the bravery limit
  is `6*3/10 = 1`); `borg_danger` of the corridor grids beside the mold is **16** (the same figure the D0008 trace logged).
  Torch light 2, Borg map beyond x=17 unexplored.
- Measurement only: the scenario wraps `borg_danger()` and `borg_note()` at link time (`-Wl,--wrap`) to count calls and
  capture notes. The wrapper forwards unchanged and checks the RNG state before and after every call: **0 changes in all
  150 case runs**.
- Both full native gates, before and after: **965/965 unit checks, 5/5 frontend tests**.

### 1. The fault, all four modes, both writers (contract cases)

For each mode a flow runs in that mode, then a normal flow runs with no wipe in between. Contract: *a normal flow never enters a
grid whose present danger exceeds the limit.*

| Case group | Original | Corrected |
| --- | --- | --- |
| `cache-{desperate,lunal,munchkin,digging}-spread` (grid reached by `borg_flow_spread`) | FAIL ×4: entry `know=1, icky=0` written with no danger check; normal flow routes through | pass ×4: entry not written; the normal flow assesses the grid, finds 16 > 1, marks it icky, no route |
| `cache-{…}-enqueue` (grid enqueued by `borg_flow_enqueue_grid`) | FAIL ×4: normal flow enqueues the dangerous grid unchecked | pass ×4 |
| `caution`: real `borg_caution()` retreat, then real `borg_flow_dark()` | retreat west (`4`), then `Flowing toward dark-3`, step **east** (`6`) | retreat west, then no step |
| `prod-visible`, `prod-remembered` (see below) | FAIL ×2 (step east) | pass ×2 |

`borg_caution()` itself chooses the retreat here ("Desperate for Stairs (two)", the note D0008 logged); only the geometry and
the Borg's HP are set up by the fixture.

### 2. Negative controls and an alternative

| Variant | Failing cases | Meaning |
| --- | --- | --- |
| Original | the 11 contract cases | the fault |
| **Corrected** | none | both writers fixed |
| Only `borg_flow_spread` fixed | the 4 `enqueue` cases | the second writer matters |
| Only `borg_flow_enqueue_grid` fixed | the 4 `spread` cases + `caution`, `prod-visible`, `prod-remembered` | the first writer is the one the full-run path uses |
| Wipe alternative (a bypass flow sets `borg_danger_wipe`) | none | also closes the fault, but see cost |

### 3. Controls that must not change (identical before and after)

`control-safe` (no monster: route exists, grids cached as safe), `control-dangerous` (mold present, fresh normal flow: first dangerous
grid becomes icky, no route), `control-blocked` (granite in the corridor: no route), `control-retreat` (desperate flow still reaches the
dangerous grid it is retreating from), `control-reuse` (a second normal flow evaluates 0 grids: honest cache entries are still reused),
`retreat-after-normal` (an icky verdict from an earlier **normal** flow still blocks a later retreat; that is existing behaviour, unchanged,
and not part of this fix). State and outcome are compared automatically; only danger-evaluation counts may differ.

### 4. Remembered-monster and line-of-sight state (checked separately, as asked)

Recomputation is **not** assumed to see the mold. Three states, using the production `borg_update()` where it matters:

| Mold at the second step | Cache wiped by the update? | Danger at the grid beside the mold | Original | Corrected |
| --- | --- | --- | --- | --- |
| Visible, unchanged | no | 16 | steps east | no step |
| Out of sight 60 turns, kill record kept (`RF_NEVER_MOVE` monsters are not "followed" and are not deleted) | no | **16** (`borg_danger_one_kill` has no line-of-sight test) | steps east | no step |
| Unseen > 2000 turns, record expired | **yes** (`borg_delete_kill` sets `borg_danger_wipe`) | **0** | steps east | steps east |

So a recomputation does see a remembered stationary monster, and the fault bites exactly when the set of remembered monsters is unchanged
(any kill creation or deletion already wipes the cache). Once the record is forgotten nothing remains to detect and both builds agree; the
correction does not and cannot restore a forgotten monster. A first attempt that let `borg_update()` re-observe the visible mold, or that
declared a spurious new level, wiped the cache for unrelated reasons; the fixture now hides the monster in the game state, sets the level
as unchanged, and clears the Borg's previous grid when it moves it (otherwise the game map marks two grids as the player's and `borg_update()` puts the Borg back beside the mold), and the notes in `*-cases.log` show what the update did.

### 5. Cost: extra danger evaluations

Counted per case with the link-time wrapper (`STATE …calls=` lines in `*-cases.log`, summary in `replay.html`).

- **What is added:** a normal flow now evaluates the grids an emergency flow merely passed through, as it would with an empty cache.
  Right after an emergency flow over the safe corridor and its unexplored frontier: 19 evaluations (original 0 to 9, the 9 being
  frontier grids that the retreat flow, which avoids unknown grids, did not reach). A fresh normal flow there also needs 19 (`control-safe`): the correction restores the evaluations the
  original skipped, no more.
- **What is not lost:** honest entries are not recomputed. Normal flow (depth 5), emergency flow over new ground, normal flow again:
  **0** evaluations in the last flow for the original and the correction, in all four modes.
- **What is saved:** the retreat path no longer makes the `borg_danger()` call whose verdict it always discarded: 0 instead of 1 per flow
  (`control-retreat`). `borg_danger()` is a pure read (scoped search of `borg-danger.c`: no assignment to any global, `borg.*`, or `kill->*` field,
  no random draws; the helpers it calls only read; confirmed by the RNG check above), so the flows in those modes are unchanged.
- **Alternative compared:** making a bypass flow force a wipe closes the fault too, but the same mixed sequence costs **15** evaluations in
  the last flow (the whole honest cache recomputed) in all four modes, so it is not proposed.
- An evaluation is `O(number of remembered monsters)`; flows evaluate each grid once per cache lifetime. No timing benchmark was taken;
  the claim is the count above, in this geometry.

### 6. Who writes the cache (scoped exhaustive search at the base)

`git grep borg_data_know e473720ed -- src` finds these writers and nothing else:

| Site | Writes | Danger assessed? | Touched by the fix |
| --- | --- | --- | --- |
| `borg-flow.c` `borg_flow_spread` (l.438) | `know=true` | **no, in the four modes** | yes |
| `borg-flow.c` `borg_flow_enqueue_grid` (l.519) | `know=true` | **evaluated, verdict discarded in the four modes** | yes |
| `borg-flow-dark.c` `borg_flow_direct`/reachable (l.264, 386) | `know=true` | yes, always, in the same block, with no bypass mode | no |
| `borg-flow-dark.c` border barrier (l.443–459) | `know = icky = stop` | temporary barrier set and cleared together | no |
| `borg-flow.c` `borg_flow_clear` (l.174) and `borg-update.c` (l.163, 600) | clear | n/a | no |

The mode test `desperate || lunal_mode || munchkin_mode || digging` appears in exactly the two writers fixed. A second reader of the cache, `borg_flow_dark`,
goes through `borg_flow_spread`/`borg_flow_direct`, so it needs no change: with no unchecked entries it evaluates the grid.

### 7. D0008 as a separate diagnostic

See [d0008-diagnostic/README.md](d0008-diagnostic/README.md). In short: the unmodified rebuild reproduces the recorded life with no difference
(replay verified; live trace byte-identical); the corrected engine diverges at turn 10837, the first command after the first retreat, and, run live, is alive at the
200000-turn cap. A 10-seed smoke check shows no stall or new death. None of that establishes a survival benefit.

## Limits

- The geometry is synthetic and small; the Borg's HP, the mold, the corridor and the unexplored frontier are placed by the fixture.
  The retreat decision itself is the production `borg_caution()`; the walk-back is the production `borg_flow_dark()`.
- The 60-turn and 2500-turn "unseen" states move the game clock directly; real play reaches them through ordinary turns.
- The cost figures are evaluation counts in one geometry, not a measured whole-game cost.
- One recorded life and a ten-seed smoke check do not measure survival. Source #156 (low-HP flee guard) and #152 (staircase flags) are separate and not changed here.

