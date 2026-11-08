# Half out of lunal mode

The lunal mode — the Borg's fast, reckless rush — is entered and left
as a pair of flags. The stair-scum bail cleared only one of them.

**`borg_think_stair_scum()`'s town-or-weak bail announced "Leaving
Scumming Mode" and cleared `borg.lunal_mode`, but left the companion
flag `borg.goal.fleeing_lunal` set — the caution phase and the
flow-recovery rest check kept treating the Borg as lunal while the
mode flag said otherwise.** Measured natively with both lunal flags
set through the production bail in town: the original leaves
`lunal_mode=0 fleeing_lunal=1` (split); the correction clears the
pair, as the normal lunal exit does.

Control: the full-pack bail returns before any flag is touched — both
halves stay set in both variants, byte-identical logs.

Consequence: the Borg behaves half-lunal after bailing — the caution
phase still skips its caution while the mode flag says the mode is
over. The measured fact is the flag state; the downstream behaviour is
the report's mechanism, not claimed here. No death or survival claim.

Canonical report: [issue314](https://github.com/diegoami/neo-angband-tracker/issues/314).
Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lunal-314-evidence
git show 0995f346e3b7ff050576376836b4bd625ddd9f00:docs/audit/c-reports/lunal-314/reproduce.py > /tmp/reproduce-lunal-314.py
python3 /tmp/reproduce-lunal-314.py --fixtures-ref 0995f346e3b7ff050576376836b4bd625ddd9f00 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the pair-clearing bail,
runs the town bail and the full-pack control per variant through the
production stair-scum routine, checks the fail-before and pass-after
matrix and the byte-identical control, records both full gates and
writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Half out of lunal mode**: the original leaves the state split; the
correction clears the pair.

Actual recorded output, original (`before-town.log`):

```text
GEOMETRY lunal_mode=0 fleeing_lunal=1
RATING lunal_mode=0 fleeing_lunal=1
```

Corrected (`after-town.log`): `RATING lunal_mode=0 fleeing_lunal=0`.
Control (`pack`): `RATING lunal_mode=1 fleeing_lunal=1` in both
variants, byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`a26da54d61be83236c2c5859141b801521d6c6d5`. One line in
`src/borg/borg-think-dungeon-util.c`: the town-or-weak bail also
clears `borg.goal.fleeing_lunal`, matching the paired lunal exit in
`borg-think-dungeon.c`.

```sh
git fetch --no-tags origin task/c-borg-scum-exit-clears-pair
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-think-dungeon-util.c | sed -n '360,370p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 a26da54d61be83236c2c5859141b801521d6c6d5 -- src/borg/borg-think-dungeon-util.c
```

### Conditions and limits

Native Human Warrior, seed one thousand, with both lunal flags set,
running the production stair-scum bail in town (or the full-pack bail
at depth 2). The measured fact is the flag state after the bail. No
death or survival claim.

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
at the same pins (base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `a26da54d61be83236c2c5859141b801521d6c6d5`, fixtures
`8df23d716a774195df59ad7b8134c489b020f3ae`) with `--full-tests`, and every case reproduced the recorded
outcome exactly (the `cases` block of the new `result.json` equals the
original receipt's):

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `town` | fail: `lunal_mode=0 fleeing_lunal=1` | pass: `lunal_mode=0 fleeing_lunal=0` |
| `pack` (control) | pass: `lunal_mode=1 fleeing_lunal=1` | pass: `lunal_mode=1 fleeing_lunal=1` |

Controls byte-identical before/after: `pack`. Full native gates from the
`*-alltests.log` summaries: before: Total: 965/965 passed (100.0%), Total: 5/5; after: Total: 965/965 passed (100.0%), Total: 5/5. Toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0; cmake version 3.28.3.

These re-run files have the same sha256 as the original receipt lists, i.e.
they are byte-identical to the original run's: `after-build.log`, `after-scenarios.log`, `before-scenarios.log`, `result.json`, `before-pack.log`, `before-results.json`, `before-town.log`, `after-pack.log`, `after-results.json`, `after-town.log`. Files that differ
only by the run's temporary paths and build timing: `after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`.

Render check: `node tools/c-evidence/render-smoke.js replay.html` on the
committed page (every scenario option and visible button):
`RENDER-SMOKE actions=2 errors=0 OK`.

Logs, `result.json` and a receipt with the sha256 of every re-run file are
in [`reverify-2026-11-08/`](reverify-2026-11-08/). Repeat, both pins fixed
(the fix sha is the runner's built-in `FIX` constant, checked against the
branch diff before building):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lunal-314-evidence
git show 8df23d716a774195df59ad7b8134c489b020f3ae:docs/audit/c-reports/lunal-314/reproduce.py > /tmp/reproduce-lunal-314.py
python3 /tmp/reproduce-lunal-314.py --full-tests --fixtures-ref 8df23d716a774195df59ad7b8134c489b020f3ae
```

The earlier text of this guide is left as written.

## Errata (2026-11-08)

- **Source proposal link.** The opening's "Source proposal" link above points to a placeholder URL. The source proposal is [#355](https://github.com/diegoami/neo-angband-tracker/pull/355) (branch `task/c-borg-scum-exit-clears-pair`); the evidence PR is [#356](https://github.com/diegoami/neo-angband-tracker/pull/356); the canonical issue is [#314](https://github.com/diegoami/neo-angband-tracker/issues/314).
- **Fixtures pins.** The receipt's `fixtures_ref` is `8df23d716a774195df59ad7b8134c489b020f3ae` (the commit the native run executed at). The commands above pin `0995f346e3b7ff050576376836b4bd625ddd9f00` and the issue's commands pinned `d50baba77bd2101732e7db3413e7dab25ba72cb4`; all three commits carry byte-identical `scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py` and `borg.txt` (`git diff --stat 8df23d716a774195df59ad7b8134c489b020f3ae d50baba77bd2101732e7db3413e7dab25ba72cb4 -- docs/audit/c-reports/lunal-314/` lists only README/receipt/replay/result artifacts), so any of them reproduces the same run. The re-verification above used `8df23d716a774195df59ad7b8134c489b020f3ae`.
- **Missing logs and render record.** The file inventory and receipt list phase logs that were not committed with the original package; the re-run's logs are in `reverify-2026-11-08/`. The receipt records its browser check under `browser`; the generic render-smoke result above is the 2026-11-08 record.
- **README hash.** Because this section was appended, `README.md` no longer matches the `README.md` hash in `receipt.json` (that hash is for the guide before this section was appended); every other hashed file is unchanged.
- **Restored original logs.** The re-run files whose sha256 equals the value in the original `receipt.json` (`after-build.log`, `after-scenarios.log`, `before-scenarios.log`, `before-pack.log`, `before-town.log`, `after-pack.log`, `after-town.log`) are byte-for-byte the original run's files, so they are also committed here at the paths the receipt names. `after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log` differ by temporary paths and timing and stay only in `reverify-2026-11-08/`.
- **Receipt amendment (2026-11-08).** `receipt.json`'s `README.md` sha256 entry now hashes this guide as amended; the old value and the change are recorded in `reverify-2026-11-08/receipt.json` under `receipt_amendments`; a `render_check` entry records the 2026-11-08 render smoke of the committed replay (the original browser record stays under `browser`). The "README hash" note above describes the state before this amendment; every other hashed entry is unchanged.
