# D0008 rerun with the corrected flow cache (separate diagnostic)

This is **not** part of the fixture proof and **not** a survival claim. It asks one
narrow question: if the consumer's recorded life D0008 is run again with only the
flow-cache correction added, what changes? One life, one seed.

The original evidence is untouched: the recorded bundle under
`better-borg-c/artifacts/death-workflow/collections/20261107T230736.548607Z/game-001-seed-1668846659`
was only read, and every run below wrote to new directories.

## Identities

| Item | Value |
| --- | --- |
| Consumer source | `5c23742ced30ad76ab9778cfa8bf23f04fc0eb1f` (prepared tree `builds/5c23742ced30-98c9f105ebd4/source`) |
| `borg-flow.c` in that tree | `0a8933b10f249e70be7b82603e97b9ee494cdfabdc7d60011cc8eee668270c1f`, byte-identical to pristine `e473720ed` |
| Corrected `borg-flow.c` | `0566d8bdb5105d9246d571c0774dbb4675a108153c728b5e038b1093e51bd30b` (the fix commit's file) |
| Only difference between the two trees | `src/borg/borg-flow.c` (checked file by file) |
| Original recorded engine | `eefdc67133daaad42b6610402b3fc323b3a5abf55595c247fb449b937cd7285f` |
| Control engine (unmodified tree, rebuilt here) | `a175f67c51e64689c63bbb5ad74a1d42bcb2e4abc607cb93521471263e92430d` |
| Corrected engine (rebuilt here) | `abc613cd4098ed3003f96869e7d0dbf220a08e6e4aa4ea4defec789a8c2119d6` |
| Recording manifest | `cc2503d16acf44f12459a5b3e3cdcb9f4608c19ca5a6843715fcd06caca39b81` |
| Build | consumer's recipe: `-DSUPPORT_BORG=ON -DSUPPORT_TEST_FRONTEND=ON -DSUPPORT_X11_FRONTEND=ON -DCMAKE_BUILD_TYPE=Debug`, target `OurExecutable` |

The control engine's hash differs from the original's because it was rebuilt in a
different directory; its behaviour is what matters and is checked below.

## What was run, and what it shows

1. **Control, replay** (`run_d0008.py`, the consumer's own `verify()` with an alternate engine):
   the unmodified rebuilt engine reproduces the recorded life with **no difference in any stream**
   (`replay-control-verification.json`: `verified: true`, `first_difference: null`).
2. **Control, live** (`run_live.py`, same seed, same recorded birth input, no replay channels):
   dies to the grey mold at turn 10940 and its `trace.txt` is **byte-identical** to the recorded
   original (SHA-256 `290597573d5d3148ea37ec6cc31a28d547f2bbeb79657ca3de54c87a2d556a73`). So a live run is a valid A/B here and the system
   clock does not matter for this life.
3. **Corrected, replay**: first difference at **turn 10837**, `actions.txt` row 746, the first command
   after the first retreat: the original queued key `54` (`6`, east); the corrected engine queued
   `57344` (`ESCAPE`) and went on to other actions (`replay-corrected-verification.json`).
   The replay then stopped with `Replay input exhausted or malformed` at turn 10937: the recorded
   input channels are finite and the diverged run asked for more. **That is a harness limit, not a game outcome.**
4. **Corrected, live**: the same life continues and, at the 200000-turn cap, is **alive**
   (Human Priest CL5, depth 2, 56 HP). Its `trace.txt` SHA-256 is `d9d54dafcd846066c25cedbac8084f301cd50757d9d6979b3780151f2d85d864`
   (full file kept in the run directory, not committed: 696 KB).

## What the corrected Borg did instead (`decisions-*-10800-10940.txt`)

| Turn | Original | Corrected |
| --- | --- | --- |
| 10827 | `Desperate for Stairs (two)`, steps west onto the up staircase | identical |
| 10837 | `Standing on up-stairs`, `Flowing toward dark-3 at cost 3`, steps **east** next to the mold | `heading up…`, `Bored but unable to flow to up stairs`, **`Boosting bravery (1) from 7 to 14`**, `Considering Phase (twitchy)`, **`Reading Scrolls of Phase Door`** |

So the east step is gone, but the corrected life gets out through the Borg's existing
bravery-boost and Phase Door fallback, **not** by taking the staircase and not through any designed escape.
That the Borg stands on a known `<` and still reports "unable to flow to up stairs" is a different matter
(source #152, kept separate). RNG, monsters and HP diverge from turn 10837, so nothing after that point is comparable
turn by turn.

## Smoke check on other seeds (`sweep.sh`, `sweep-summary.txt`)

Ten seeds (the D0008 seed plus nine arbitrary ones), 30000-turn cap, unmodified vs corrected, same recorded birth input:

- 6 of 10 seeds: identical outcome (including one pair of identical grey-mold deaths at turn 19355 and one
  cutpurse death at 3927, which the correction does not touch).
- 2 of 10 differ, both in favour of the corrected build: D0008's seed (death at 10940 vs alive at the cap) and seed 404
  (soldier death at 14472 vs alive).
- 0 of 10: a stall, a timeout, or a death the unmodified build avoided.

This is a **regression smoke check, not an evaluation**: ten lives, a short cap, one character, no matched
long-run measurement. It supports "no obvious new failure" and nothing about the size of any survival benefit.
Prospective survival and long-term progress evaluation remains the consumer's separate adoption work
(better-borg-c#212).

## Repeat

The scripts use absolute paths to the consumer's local bundle and to build directories of the author's machine;
adjust `ORIGINAL`/`O` at the top of `run_d0008.py` and `run_live.py` and the build paths in `sweep.sh`.
The consumer's tools at the recorded harness commit (`tools/`, `replay-borg.py`) are required next to `run_d0008.py`.
