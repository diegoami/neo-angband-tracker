# Vampire Strike was judged by a bystander's distance

A Necromancer with Vampire Strike faces a kobold five grids away while
a hill giant stands twenty grids away. The Borg's strike routine scans
its target list for the closest monster and picks the kobold — but its
"require relatively close" test afterwards reads a distance variable
that the scan leaves holding the last entry it examined, including
entries it skipped.

**One distant bystander anywhere in the target list kept the spell
from ever being cast, even with a perfect target five grids away.**
Since the chosen entry is always the minimum distance, the mistaken
test only ever over-restricted the spell. The correction judges the
chosen monster's distance; the strike is cast (the chooser queues the
real spell keys).

Measured transition: with the kobold at five and the giant at twenty,
the original attack chooser does nothing (acted 0, no keys); the
corrected chooser casts Vampire Strike ('m' plus book and spell keys).
Two situations stay byte-identical before and after: with both
monsters close, both versions cast; with only the distant monster,
both refuse — the chosen distance really is twenty.

Consequence: a working attack spell went unused in exactly the
situations it is made for. No damage, death or survival claim beyond
the recorded decision.

Canonical report: [issue257](https://github.com/diegoami/neo-angband-tracker/issues/257).
Lead supplied by borg-rs; independently validated and reproduced
natively here through the full production attack chooser. Source
proposal [#293](https://github.com/diegoami/neo-angband-tracker/pull/293).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/vampire-257-evidence
git show e41e44d9652c38266dcee5e9e5b4b363f3813212:docs/audit/c-reports/vampire-257/reproduce.py > /tmp/reproduce-vampire-257.py
python3 /tmp/reproduce-vampire-257.py --fixtures-ref e41e44d9652c38266dcee5e9e5b4b363f3813212 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the one-token guard,
runs three recorded situations per variant, checks the fail-before/
pass-after matrix and byte-identical controls, records both full gates
and writes an offline `replay.html`. The native verification ran with
these generators except the replay generator's syntax and wording,
completed for the captured package; no native log, source variant or
gate differs.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A close target and a distant bystander**, then **The target list** and
**The decision**. The original refuses the cast; the correction casts.
The other two situations are controls whose logs match byte for byte.

Actual recorded output, original (`before-near-far.log`):

```text
GEOMETRY near=5 far=20 max_range=20
DECISION acted=0 keys=0 sequence=
OUTCOME acted=0 keys=0 cast_spell=0
```

Corrected (`after-near-far.log`): `DECISION acted=1 keys=3
sequence=mbb`, `OUTCOME acted=1 keys=3 cast_spell=1`.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`0119d5daee9b8a07a8096331eea78dbbb6c26050`. One token in
`src/borg/borg-fight-attack.c`. The closest-target scan assigns `dist`
for every entry before deciding to skip it, so after the loop `dist`
holds the distance of the last entry examined — not the chosen one
(`b_i`, whose distance is tracked in `best_dist`). The guard
`if (dist >= 20) return 0;` therefore tested an arbitrary bystander.
The correction tests `best_dist`, the chosen monster's distance.

```sh
git fetch --no-tags origin task/c-borg-vampire-strike-distance
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '2331,2350p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 0119d5daee9b8a07a8096331eea78dbbb6c26050 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Necromancer level 26, depth 30, seed 1000, lit
constructed 40x40 arena, the Fear and Torment shadow book, Vampire
Strike learned as the only spell, full mana, a torch. The production
attack chooser acts on its own filled target list; the cast is shown
by its queued keys, not executed. The bystander sits at distance
twenty, the closeness limit itself, within the spell's range. This
isolates the closeness guard, not full autonomous play. No damage or
survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).


## Re-verification (2026-11-08)

Why: the original run's phase, build and gate logs were listed and hashed in
`receipt.json` but never committed (the repository's `*.log` ignore rule
dropped them), so the receipt could not be checked against published bytes.
The same pinned runner was run again on 2026-11-08 at the receipt's own pins
(base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `0119d5daee9b8a07a8096331eea78dbbb6c26050`, fixtures `545213726d6d0156ea1cea931b212a9895c5b77b`) with `--full-tests`.
Every case reproduced the recorded outcome; the runner's per-case results
(`result.json` `cases`) are identical to the committed original.

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `near-far` (chosen monster 5 grids, last scanned 20) | `acted=0`, no Vampire Strike; 0/1 | `acted=1 keys=3`, native cast; 1/1 |
| `near-near` (control) | `acted=1 keys=3`; 1/1 | byte-identical log |
| `far` (control) | `acted=0`; 1/1 | byte-identical log |

Gates from the re-run's alltests logs: before Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines); after Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines).
Re-run toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, cmake version 3.28.3 (the original receipt records
gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3; the toolchain differs, and the files listed
below as byte-identical are identical nonetheless).
Corrected source sha256 matches the original receipt.
Render check of the committed `replay.html` with `tools/c-evidence/render-smoke.js`
(Playwright Chromium, every option and button clicked): `RENDER-SMOKE actions=9 errors=0 OK`.
The original receipt records its own headless check under the `browser` key.
At the receipt pin `545213726` the runner finished every native step, assertion and gate and wrote `result.json`, then stopped with exit 1 in the last step because `make_replay.py` at that pin does not parse (see Errata and `reverify-2026-11-08/runner-stdout.txt`). The fixed generator from the guide pin `e41e44d96` regenerates the committed `replay.html` and `replay-data.json` byte-identically from this re-run's logs, so the repeat command below pins `e41e44d96`.

Where the logs are: [`reverify-2026-11-08/`](reverify-2026-11-08/) holds every log,
`result.json` and per-mode results of the re-run, with
[`reverify-2026-11-08/receipt.json`](reverify-2026-11-08/receipt.json) (pins, per-case
results, gate lines, toolchain, render result, sha256 of every file there).
12 re-run files are byte-identical to the hashes in the original
`receipt.json`; those not already committed were also placed at their original paths in this directory, so the original receipt now resolves for them: `after-build.log`, `after-far.log`, `after-near-far.log`, `after-near-near.log`, `after-scenarios.log`, `before-far.log`, `before-near-far.log`, `before-near-near.log`, `before-scenarios.log`.
The rest (`after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`) differ in
run-specific content (temporary paths, timings, build output of the different toolchain) and are
published only under `reverify-2026-11-08/`.

Repeat (fixtures pinned by sha; the fix sha is fixed inside `reproduce.py` at that pin, and the runner
refuses any other source diff):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/vampire-257-evidence
git show e41e44d9652c38266dcee5e9e5b4b363f3813212:docs/audit/c-reports/vampire-257/reproduce.py > /tmp/reproduce-vampire-257.py
python3 /tmp/reproduce-vampire-257.py --full-tests --fixtures-ref e41e44d9652c38266dcee5e9e5b4b363f3813212
```

## Errata (2026-11-08)

- `make_replay.py` at the receipt's `fixtures_ref` `545213726` does not parse, so a run at that pin ends with exit 1 after all native checks and gates. The capture commit `746e63c85` fixed it; it is the only generator change up to the guide pin `e41e44d96`.
- The source-proposal link near the top of this README was a placeholder; it now points to the source PR [#293](https://github.com/diegoami/neo-angband-tracker/pull/293) (evidence PR #294, canonical issue #257).
- `receipt.json` was amended on 2026-11-08 for this only: its `README.md` hash now covers the README with these sections and the link fix, and a `render_check` key records the smoke check above; the old→new values are listed under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
