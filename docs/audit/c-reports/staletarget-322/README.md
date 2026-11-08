# The phantom in his own boots

After a teleport plus a blindness transition, a tracked monster entry
can still point at the grid the player now occupies — and the melee
chooser accepted it.

**`borg_attack_aux_thrust()` accepted candidate monsters at distance
zero as "adjacent". No real monster can share the player's grid, so
such an entry is stale tracking — and selecting it queued the
center-square key '5' for the attack direction, which is not a
direction and trips the unexpected-direction abort that stops the
Borg.** Measured natively through the production attack chooser in
acting mode: with the tracked entry on the player's own grid the
original attacks (`acted=1 keys="+5"`); the correction requires
distance exactly one, queues nothing, and the run continues.

Control: a genuinely adjacent kobold one grid south is attacked
identically in both variants (`+2`, byte-identical logs) — the
correction rejects only the impossible own-grid case.

Consequence: the recorded in-game abort — the Borg stopped after a
teleport with a stale monster on its grid (the supplied extended-run
evidence). The fixture measures the decision that produces it, not
the full replay.

Canonical report: [issue322](https://github.com/diegoami/neo-angband-tracker/issues/322).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staletarget-322-evidence
git show c969dc2493865d934826c69823ad83c3b4dbb8d0:docs/audit/c-reports/staletarget-322/reproduce.py > /tmp/reproduce-staletarget-322.py
python3 /tmp/reproduce-staletarget-322.py --fixtures-ref c969dc2493865d934826c69823ad83c3b4dbb8d0 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the adjacency check,
runs the stale-target and control situations per variant through the
production attack chooser in acting mode, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The phantom in his own boots**: the original queues +5 — an attack on
the player's own grid; the correction queues nothing.

Actual recorded output, original (`before-stale.log`):

```text
GEOMETRY target=(11,11) player=(11,11) dist=0
RATING acted=1 keys="+5" center_attack=1
```

Corrected (`after-stale.log`): `RATING acted=0 keys=""
center_attack=0`. Control (`control`): `RATING acted=1 keys="+2"
center_attack=0` in both variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`1ab30ef0bc9a12caa5b1cf48e60c204ef564e9de`. One check in
`src/borg/borg-fight-attack.c`. The candidate filter required distance
at most one, accepting zero; `borg_extract_dir()` maps a same-grid
target to the center square 5, which is not an attack direction. The
correction requires distance exactly one.

```sh
git fetch --no-tags origin task/c-borg-thrust-adjacent
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '288,292p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 1ab30ef0bc9a12caa5b1cf48e60c204ef564e9de -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Warrior on a lit arena at depth 30, seed one thousand, a
Broad Sword wielded, one observed awake Kobold. The stale situation
moves the tracked entry onto the player's grid — exactly the
post-teleport state the supplied run evidence recorded; the abort
itself happens later, when the empty direction prompt is answered, so
this fixture measures the decision that produces it. No death claim;
the recorded consequence is the stopped Borg.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).


## Re-verification (2026-11-08)

Why: the receipt above hashes the original run's phase logs, but they were
never committed (the repository's `*.log` ignore rule dropped them), so the
package could not be checked against its own receipt. The run was repeated
at the same pins (base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `1ab30ef0bc9a12caa5b1cf48e60c204ef564e9de`, fixtures
`808d018553a6869a4e8024a33e8a1e1852e2d7ea`) with `--full-tests`, and every case reproduced the recorded
outcome exactly (the `cases` block of the new `result.json` equals the
original receipt's):

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `stale` | fail: `acted=1 keys="+5" center_attack=1` | pass: `acted=0 keys="" center_attack=0` |
| `control` (control) | pass: `acted=1 keys="+2" center_attack=0` | pass: `acted=1 keys="+2" center_attack=0` |

Controls byte-identical before/after: `control`. Full native gates from the
`*-alltests.log` summaries: before: Total: 965/965 passed (100.0%), Total: 5/5; after: Total: 965/965 passed (100.0%), Total: 5/5. Toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0; cmake version 3.28.3.

These re-run files have the same sha256 as the original receipt lists, i.e.
they are byte-identical to the original run's: `after-build.log`, `after-scenarios.log`, `before-scenarios.log`, `result.json`, `before-control.log`, `before-results.json`, `before-stale.log`, `after-control.log`, `after-results.json`, `after-stale.log`. Files that differ
only by the run's temporary paths and build timing: `after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`.

Render check: `node tools/c-evidence/render-smoke.js replay.html` on the
committed page (every scenario option and visible button):
`RENDER-SMOKE actions=2 errors=0 OK`.

Logs, `result.json` and a receipt with the sha256 of every re-run file are
in [`reverify-2026-11-08/`](reverify-2026-11-08/). Repeat, both pins fixed
(the fix sha is the runner's built-in `FIX` constant, checked against the
branch diff before building):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staletarget-322-evidence
git show 808d018553a6869a4e8024a33e8a1e1852e2d7ea:docs/audit/c-reports/staletarget-322/reproduce.py > /tmp/reproduce-staletarget-322.py
python3 /tmp/reproduce-staletarget-322.py --full-tests --fixtures-ref 808d018553a6869a4e8024a33e8a1e1852e2d7ea
```

The earlier text of this guide is left as written.

## Errata (2026-11-08)

- **Source proposal link.** The opening's "Source proposal" link above points to a placeholder URL. The source proposal is [#343](https://github.com/diegoami/neo-angband-tracker/pull/343) (branch `task/c-borg-thrust-adjacent`); the evidence PR is [#344](https://github.com/diegoami/neo-angband-tracker/pull/344); the canonical issue is [#322](https://github.com/diegoami/neo-angband-tracker/issues/322).
- **Fixtures pins.** The receipt's `fixtures_ref` is `808d018553a6869a4e8024a33e8a1e1852e2d7ea` (the commit the native run executed at). The commands above pin `c969dc2493865d934826c69823ad83c3b4dbb8d0` and the issue's commands pinned `929764f4d12cece8181a22106c1d808bdd3c2881`; all three commits carry byte-identical `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py` and `borg.txt` (`git diff --stat 808d018553a6869a4e8024a33e8a1e1852e2d7ea 929764f4d12cece8181a22106c1d808bdd3c2881 -- docs/audit/c-reports/staletarget-322/` lists only README/receipt/replay/result artifacts), so any of them reproduces the same run. The re-verification above used `808d018553a6869a4e8024a33e8a1e1852e2d7ea`.
- **Missing logs and render record.** The file inventory and receipt list phase logs that were not committed with the original package; the re-run's logs are in `reverify-2026-11-08/`. The receipt records its browser check under `browser`; the generic render-smoke result above is the 2026-11-08 record.
- **README hash.** Because this section was appended, `README.md` no longer matches the `README.md` hash in `receipt.json` (that hash is for the guide as first committed); every other hashed file is unchanged.
