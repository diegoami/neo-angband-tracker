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
natively here. Source proposal [#335](https://github.com/diegoami/neo-angband-tracker/pull/335).

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

- The "Source proposal" link near the top was labelled `PR-SOURCE` and pointed at an unfilled placeholder URL; on 2026-11-08 it was corrected in place to the real link. The source proposal is draft PR [#335](https://github.com/diegoami/neo-angband-tracker/pull/335); this evidence is draft PR [#336](https://github.com/diegoami/neo-angband-tracker/pull/336).
- The repeat commands above pin `c3c980b3462773810c560931e3ae51b4996c9316` while `receipt.json` records `fixtures_ref` `b4f23f90723a46ad155d92127fdcad7c561cf65c`, the commit the native run actually executed at. Both carry the same generators (see the re-verification note); prefer the `b4f23f90723a46ad155d92127fdcad7c561cf65c` pin given in the Re-verification section.
- `receipt.json` hashes phase logs that were never committed (the `*.log` ignore rule). The re-verification run's logs, under `reverify-2026-11-08/`, replace them; where the original hash is reproduced byte for byte, that is listed above. Its `browser` entry records the original Chrome-for-Testing render check; the re-verification adds the render-smoke result above.
- **AddressSanitizer variant (manual, 2026-11-08).** This is a memory-safety defect but the package had no asan variant, and `reproduce.py` at this pin cannot build one. A manual ASAN build of the same disposable tree (both variants, full tree, `-fsanitize=address -g`, leak detection on) ran both cases with the unchanged `run-cases.py`: the outcomes equal the normal build (`multibyte`: original `terminated=0 strnlen=1024`, fails; corrected `terminated=1 strnlen=81`, passes; `control` 80/80 with byte-identical logs) and **AddressSanitizer reported nothing in either variant**. That is expected and is a limit, not a clean bill: the fixture measures the text with `strnlen` bounded by its 1024-byte buffer, so the original's missing terminator shows in the measurement but is not an out-of-bounds access inside the fixture; the unbounded read happens in the production caller (`strlen` in `borg_get_messages()`), which this fixture does not exercise. Logs: `reverify-2026-11-08/asan-{before,after}-{multibyte,control}.log`, build logs `*-build-asan.log`, details under `asan_manual` in `reverify-2026-11-08/receipt.json`.
- **The bound claim under "Source explanation" is not correct as written.** The text says the new limit `ABS(n) * MB_CUR_MAX` stays "within the callers' 1024-byte buffers for every reachable `n`". The limit is not tied to the destination size: `borg_get_messages()` passes `char buffer[1024]` with `n = (Term->wid - 1) / tile_width`, so a terminal about 256 columns wide or more showing a line of 4-byte UTF-8 characters can make `wcstombs` write past the buffer. A standalone model of the corrected line (`reverify-2026-11-08/bound-model-wide.c`, not the game) under ASAN reports `stack-buffer-overflow` for n=256 (WRITE of size 1025) and n=300 (WRITE of size 1201). Not measured in the native game. The source fix is unchanged pending an owner decision between passing the buffer size and clamping to size-1 with forced termination (issue #287).
- Re-run files whose bytes reproduce the original receipt's sha256 exactly are also committed at their original paths in this directory, so the original receipt's hashes check against them: `after-control.log`, `after-multibyte.log`, `after-scenarios.log`, `before-control.log`, `before-multibyte.log`, `before-scenarios.log`. The original run's build, configure and gate logs cannot be reproduced byte for byte; the complete re-run set is under `reverify-2026-11-08/`.
- `receipt.json` was amended on 2026-11-08 for this append: its `README.md` hash now matches this README, and a `render_check` entry points at the re-verification render result. The old and new values are recorded under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.

## Revision (2026-11-08): owner-decided shape

**What changed and why.** Review found that the earlier correction's bound,
`ABS(n) * MB_CUR_MAX`, is derived from the character count, not from the size
of the destination buffer (see the Errata above). The owner decided
([issue comment](https://github.com/diegoami/neo-angband-tracker/issues/287#issuecomment-6721286609)):
**pass the buffer size.** `borg_what_text()` now takes the destination size,
converts at most `size - 1` bytes and always terminates the result (an empty
string on a conversion error); every caller passes `sizeof` of its buffer.
`wcstombs()` does not terminate a result it had to truncate, so the terminator
is written at the returned length.

**These pins supersede every earlier pin in this guide** (fix `3e7fe8a83`,
fixtures `b4f23f907` / `c3c980b34` / `8d844e379`). The earlier receipt is kept
as [`superseded-2026-11-08/receipt-fix-3e7fe8a.json`](superseded-2026-11-08/receipt-fix-3e7fe8a.json);
the top-level logs, results, `result.json`, `replay.html` and `replay-data.json`
are now this run's, and [`receipt.json`](receipt.json) is its receipt.

- Base (pristine C): `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`
- Fix: `e7f670fa4d4f0672dc346637fc7be20c814a4b62` (draft PR #335; whole branch
  against the base: `src/borg/borg-io.c`, `borg-io.h`, `borg-messages.c`,
  `borg-think.c`, 20 insertions, 11 deletions)
- Earlier fix shape, measured only for comparison: `3e7fe8a8399352c4fe67f5de2d8d48b5e0bd04c1`
- Fixtures: `f9706564e0a667670fb024e9e1f87da137f21c9c`

`reproduce.py` now asserts every hunk of the four-file change exactly and the
source scope, builds three variants (original, earlier fix shape, correction)
in a normal and an AddressSanitizer tree (full tree each, `-fsanitize=address
-g`, leak detection on) and runs every case in both.

New case **`wide`**: a 301-column terminal with a message line of 300
characters, each four bytes in UTF-8, read the way `borg_get_messages()` reads
it (`n = (Term->wid - 1) / tile_width` into a 1024-byte buffer). In the normal
build the buffer is followed by a guard area that counts writes past it; under
AddressSanitizer the buffer stands alone so such a write is reported.

| Case | Original | Earlier fix shape | Correction |
| --- | --- | --- | --- |
| `multibyte` | `terminated=0 strnlen=1024` (fails) | `terminated=1 strnlen=81` | `terminated=1 strnlen=81` (passes) |
| `control` | `terminated=1 strnlen=80` | `terminated=1 strnlen=80` | `terminated=1 strnlen=80`, log byte-identical to the original's |
| `wide` | `terminated=0 strnlen=1024 guard_overwritten=0` (fails: unterminated, no overflow) | `terminated=0 strnlen=1024 guard_overwritten=177` (fails: overflow) | `terminated=1 strnlen=1020 guard_overwritten=0` (passes) |
| AddressSanitizer, all three cases | no report | `wide`: `stack-buffer-overflow`, `WRITE of size 1201` in `wcstombs` from `borg_what_text`; the others no report | no report |

**Said plainly: the original pin cannot exhibit the overflow.** Its limit is
`n + 1` bytes, which never exceeds the 1024-byte buffer for a reachable `n`; on
the wide terminal it stops after 75 characters with no terminator (the same
missing-terminator defect as `multibyte`). The overflow was introduced by the
earlier fix shape, not by base C, and the correction removes it: 255 whole
characters (1020 bytes) and a terminator, nothing past the buffer.

Supporting evidence (kept, unchanged): the standalone program
[`reverify-2026-11-08/bound-model-wide.c`](reverify-2026-11-08/bound-model-wide.c)
with its log [`bound-model-wide-asan.log`](reverify-2026-11-08/bound-model-wide-asan.log)
copies only the earlier fix shape's `wcstombs` line into a 1024-byte stack
buffer and reports `stack-buffer-overflow` at n=256 (WRITE of size 1025) and
n=300 (WRITE of size 1201). It is a model, not the game; the `wide` case above
measures the same overflow through the real `borg_what_text()`.

Full native gates, parsed from `before-alltests.log` and `after-alltests.log`:
**965/965 unit checks and 5/5 frontend tests** for the original and the
correction (the earlier fix shape is not gated). Render check of the committed
`replay.html` with `tools/c-evidence/render-smoke.js`:
`RENDER-SMOKE actions=6 errors=0 OK`.

Limits: the fixture calls `borg_what_text()` directly the way the caller does;
it does not run `borg_get_messages()` or the message parser, and the
`strlen` read in that caller is not exercised (AddressSanitizer reports nothing
for the original because the fixture measures with `strnlen` bounded by the
buffer). No death or survival claim.

Repeat:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/text-287-evidence
git show f9706564e0a667670fb024e9e1f87da137f21c9c:docs/audit/c-reports/text-287/reproduce.py > /tmp/reproduce-text-287.py
python3 /tmp/reproduce-text-287.py --full-tests --fixtures-ref f9706564e0a667670fb024e9e1f87da137f21c9c
```

Final line of the recorded run:

```text
VERIFIED: multibyte - the original leaves the text unterminated (1024 stale bytes), the correction terminates at 81; wide (301 columns, 4-byte text) - the original is unterminated without overflow, the earlier character-count bound writes 177 bytes past the 1024-byte buffer (AddressSanitizer: stack-buffer-overflow, WRITE of size 1201), the buffer-size bound terminates at 1020 with nothing past the buffer; control identical; no sanitizer report for the original or the correction.
```
