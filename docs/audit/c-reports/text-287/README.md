# The message line that never ended

The screen row holds eighty characters, one of them two bytes wide, so
the text needs eighty-one bytes plus its terminator — one byte past the
wide-count limit the conversion used.

**`borg_what_text()` converted the wide screen text with a byte limit
of one per wide character, although a multibyte character occupies
more. A line whose byte length lands exactly on the limit was truncated
without its terminating zero — and the caller's buffer, uninitialized,
was then trimmed and measured as a string.** Measured with the buffer
junked and no zero anywhere: the original measurement runs through all
1024 stale bytes; the correction bounds the conversion by the widest
encoding for the same character count, so the full text converts and
terminates at 81.

Control: a pure-ASCII row fits the old limit as well — both variants
terminate identically at 80 with byte-identical logs.

Consequence: in play, any multibyte message line hitting the exact byte
boundary (UTF-8 names such as Carn Dûm) leaves the message text
unterminated, and the Borg parses stale stack bytes as message text —
the supplied harness evidence recorded a phantom monster from exactly
this. No death or survival claim is made.

Canonical report: [issue287](https://github.com/diegoami/neo-angband-tracker/issues/287).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/text-287-evidence
git show c3c980b3462773810c560931e3ae51b4996c9316:docs/audit/c-reports/text-287/reproduce.py > /tmp/reproduce-text-287.py
python3 /tmp/reproduce-text-287.py --fixtures-ref c3c980b3462773810c560931e3ae51b4996c9316 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the bounded conversion,
runs two recorded situations per variant, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The message line that never ended**: the original reads 1024 stale
bytes; the correction terminates at 81, the full text.

Actual recorded output, original (`before-multibyte.log`):

```text
GEOMETRY rc=0 strnlen=1024 terminated=0
RATING terminated=0 strnlen=1024
OUTCOME terminated=0 strnlen=1024
```

Corrected (`after-multibyte.log`): `RATING terminated=1 strnlen=81`.
Control (`control`): `RATING terminated=1 strnlen=80` in both variants,
byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`3e7fe8a8399352c4fe67f5de2d8d48b5e0bd04c1`. One block in
`src/borg/borg-io.c`. `wcstombs(s, screen_str, ABS(n) + 1)` limits
bytes while `n` counts wide characters; when the converted byte length
reaches the limit the result has no terminating zero. The correction
bounds the conversion by `ABS(n) * MB_CUR_MAX` — the widest encoding
for the same character count, still within the callers' 1024-byte
buffers for every reachable `n` — and falls back to an empty string on
a conversion error.

```sh
git fetch --no-tags origin task/c-borg-what-text-terminates
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-io.c | sed -n '118,132p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 3e7fe8a8399352c4fe67f5de2d8d48b5e0bd04c1 -- src/borg/borg-io.c
```

### Conditions and limits

Native Human Warrior, seed one thousand, a UTF-8 locale (the multibyte
conversion only widens in a multibyte locale), a hand-painted screen
row of eighty characters with exactly one two-byte character, and a
caller buffer deliberately junked with no zero anywhere so an
unterminated conversion is visible as a full-buffer measurement. This
isolates the conversion bound, not the full message-parsing chain; the
phantom-monster consequence is the supplied harness evidence, not
claimed here. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).


## Re-verification (2026-11-08)

**Why.** The original run's phase logs were never committed: the repository's
`*.log` ignore rule dropped them although `receipt.json` above lists and hashes
them. This section records a second native run at the same pins; the earlier
text of this guide is unchanged.

Pins: base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `3e7fe8a8399352c4fe67f5de2d8d48b5e0bd04c1` (draft PR #335), fixtures
`b4f23f90723a46ad155d92127fdcad7c561cf65c` (the original receipt's `fixtures_ref`). The generators (`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt`) are byte-identical between the run pin `b4f23f907` and every later commit of this branch, so the earlier commands that pin a later evidence commit run the same fixture.

Every case reproduced the recorded result (pass/fail and every recorded
outcome field are equal to the original receipt):

| Case | Original: recorded → re-run | Corrected: recorded → re-run |
| --- | --- | --- |
| `multibyte` | passed=False `terminated=0 strnlen=1024` → passed=False, same | passed=True `terminated=1 strnlen=81` → passed=True, same |
| `control` | passed=True `terminated=1 strnlen=80` → passed=True, same | passed=True `terminated=1 strnlen=80` → passed=True, same |

- Controls: `control` byte-identical.
- Full native gates, parsed from the committed alltests logs: before: 965/965 and 5/5 (0 failed lines); after: 965/965 and 5/5 (0 failed lines).
- Byte identity with the original receipt's hashes: 9 of 14 re-run files are byte-identical
  to the hashes the original receipt recorded (`after-control.log`, `after-multibyte.log`, `after-results.json`, `after-scenarios.log`, `before-control.log`, `before-multibyte.log`, `before-results.json`, `before-scenarios.log`, `result.json`).
  The others (`after-alltests.log`, `after-build.log`, `before-alltests.log`, `before-build.log`, `configure.log`) carry build paths, timings and toolchain output:
  this run used gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 and cmake version 3.28.3; the original used gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 and cmake version 4.2.3.
- Render check of the committed `replay.html` (sha256 `bdb409d755e8…`) with
  `tools/c-evidence/render-smoke.js` (headless Chromium, clicks every scenario option and button):
  `RENDER-SMOKE actions=4 errors=0 OK`.

The logs, both results JSONs and `result.json` of this run are in
[`reverify-2026-11-08/`](reverify-2026-11-08/); its
[`receipt.json`](reverify-2026-11-08/receipt.json) records the pins, per-case results,
gate totals, toolchain, render result and the sha256 of every file there.

Repeat (fixtures and fix pinned; `reproduce.py` hard-codes the base and fix
commits and refuses any other source hunk, so it has no `--fix-ref` flag):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/text-287-evidence
git show b4f23f90723a46ad155d92127fdcad7c561cf65c:docs/audit/c-reports/text-287/reproduce.py > /tmp/reproduce-text-287.py
python3 /tmp/reproduce-text-287.py --full-tests --fixtures-ref b4f23f90723a46ad155d92127fdcad7c561cf65c
```

## Errata (2026-11-08)

- The "Source proposal" link near the top labelled `PR-SOURCE` points at an unfilled placeholder URL. The source proposal is draft PR [#335](https://github.com/diegoami/neo-angband-tracker/pull/335); this evidence is draft PR [#336](https://github.com/diegoami/neo-angband-tracker/pull/336).
- The repeat commands above pin `c3c980b3462773810c560931e3ae51b4996c9316` while `receipt.json` records `fixtures_ref` `b4f23f90723a46ad155d92127fdcad7c561cf65c`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `b4f23f90723a46ad155d92127fdcad7c561cf65c` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
- **AddressSanitizer variant (manual, 2026-11-08).** This is a memory-safety defect but the package had no asan variant, and `reproduce.py` at this pin cannot build one. A manual ASAN build of the same disposable tree (both variants, full tree, `-fsanitize=address -g`, leak detection on) ran both cases with the unchanged `run-cases.py`: the outcomes equal the normal build (`multibyte`: original `terminated=0 strnlen=1024`, fails; corrected `terminated=1 strnlen=81`, passes; `control` 80/80 with byte-identical logs) and **AddressSanitizer reported nothing in either variant**. That is expected and is a limit, not a clean bill: the fixture measures the text with `strnlen` bounded by its 1024-byte buffer, so the original's missing terminator shows in the measurement but is not an out-of-bounds access inside the fixture; the unbounded read happens in the production caller (`strlen` in `borg_get_messages()`), which this fixture does not exercise. Logs: `reverify-2026-11-08/asan-{before,after}-{multibyte,control}.log`, build logs `*-build-asan.log`, details under `asan_manual` in `reverify-2026-11-08/receipt.json`.
- **The bound claim under "Source explanation" is not correct as written.** The text says the new limit `ABS(n) * MB_CUR_MAX` stays "within the callers' 1024-byte buffers for every reachable `n`". The limit is not tied to the destination size: `borg_get_messages()` passes `char buffer[1024]` with `n = (Term->wid - 1) / tile_width`, so a terminal about 256 columns wide or more showing a line of 4-byte UTF-8 characters can make `wcstombs` write past the buffer. A standalone model of the corrected line (`reverify-2026-11-08/bound-model-wide.c`, not the game) under ASAN reports `stack-buffer-overflow` for n=256 (WRITE of size 1025) and n=300 (WRITE of size 1201). Not measured in the native game. The source fix is unchanged pending an owner decision between passing the buffer size and clamping to size-1 with forced termination (issue #287).
