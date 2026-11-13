# The weapon whose slays vanished

A level thirty-one warrior wields a Broad Sword carrying the animals
slay, with a lit torch in the later light slot, and faces an adjacent
white harpy.

**The slay traits were assigned, not accumulated, for every equipment
slot in the Borg's inventory scan. The last occupied slot — usually the
light or the boots, which carry no slays — therefore erased whatever
the wielded weapon provided, and every melee valuation saw an unslaying
weapon.** The Borg rated the Slay Animal blade exactly like a plain
one: the thrust against the harpy is worth 17, identical to the same
attack with the slay removed by hand. Restoring the accumulation rates
it at 20 — exactly the reading with the slay forced on.

This runs on every `borg_notice` call in real play: every weapon with a
slay or brand was systematically undervalued against the races it
slew, biasing weapon swaps and melee decisions. The correction also
derives the never-set kill traits from the same race flags, where the
race-destroying slays are stored with the largest multiplier.

Control: against a small kobold, which carries no animal flag, all
three readings coincide (19) and the complete log is byte-identical
before and after — the correction changes only the case where the slay
applies. The forced-on reading is identical in both variants, so the
correction restores exactly the value the slay is worth.

Consequence: every slay weapon's melee valuation against matching
races. No death or survival claim is made.

Canonical report: [issue239](https://github.com/diegoami/neo-angband-tracker/issues/239).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [#304](https://github.com/diegoami/neo-angband-tracker/pull/304).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/slay-239-evidence
git fetch https://github.com/diegoami/neo-angband-tracker.git task/c-borg-slay-traits-max
git show 7e8af1f97572f6dd11467ab1bb18ef12391be062:docs/audit/c-reports/slay-239/reproduce.py > /tmp/reproduce-slay-239.py
python3 /tmp/reproduce-slay-239.py --fixtures-ref 7e8af1f97572f6dd11467ab1bb18ef12391be062 --full-tests
```

These commands pin the current correction `720ab3c4f` (the `MAX()` form
accepted in upstream review; see the 2026-11-13 section at the end).

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the `MAX()` slays
plus the kill-trait derivation, runs two recorded situations per
variant, checks the fail-before and pass-after matrix and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A Slay Animal weapon against an animal**: the original values the
thrust at 17 — the no-slay reading; the correction values it at 20.

Actual recorded output, original (`before-animal.log`):

```text
TRAIT ws_animal=0
RATING auto=17 without=17 with=20
OUTCOME auto=17 without=17 with=20
```

Corrected (`after-animal.log`): `TRAIT ws_animal=2`,
`RATING auto=20 without=17 with=20`. Control (`nonanimal`):
`RATING auto=19 without=19 with=19` in both variants, byte-identical
logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`720ab3c4f796c79d6108dd9d27a37fa573ca5917` (earlier form `9ab78ae60`, see below). One block in
`src/borg/borg-trait.c`. Inside the equipment loop of
`borg_notice_equipment()`, the eight slay traits were plain
assignments (`borg.trait[BI_WS_ANIMAL] = item->slays[RF_ANIMAL];`), so
each later slot overwrote whatever an earlier slot provided — and
since the weapon slot comes before the light and boots, which have no
slays, the result was always zero. The brand traits directly below
already used the accumulate-safe form (`if (item->brands[...]) trait
= true`). The correction keeps the largest multiplier across slots
with `MAX()` (first proposed as `|=`; the review preferred `MAX()`
because `item->slays` holds the multiplier) and derives
the kill traits (whose slay-table entries share the race flags but
carry the largest multiplier) for the multiplier ladder in
`borg_thrust_damage_one()` (×2 animal/evil, ×3 other slays and brands,
×5 kills), whose kill branches were dead because the traits were never
set.

```sh
git fetch --no-tags origin task/c-borg-slay-traits-max
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-trait.c | sed -n '1440,1470p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 720ab3c4f796c79d6108dd9d27a37fa573ca5917 -- src/borg/borg-trait.c
```

### Conditions and limits

Native Human Warrior level thirty-one, depth five, seed one thousand,
lit constructed 40x40 arena, a Broad Sword carrying the animals slay
from the slay table (race flag ANIMAL, the base multiplier of two)
wielded in the weapon slot, the birth torches wielded in the later
light slot, an adjacent awake white harpy (or small kobold in the
control). The measurement calls the public thrust valuation
(`borg_calculate_attack_effectiveness(BF_THRUST)`) with the slay trait
as the inventory scan computed it, then with the trait forced off and
on. This isolates the trait accumulation, not full autonomous play.
No death or survival claim.

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
(base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `9ab78ae605ca4463268a669121d69b216285c2f1`, fixtures `65fa24e6c650aab2a9bbeef75800fa068a602429`) with `--full-tests`.
Every case reproduced the recorded outcome; the runner's per-case results
(`result.json` `cases`) are identical to the committed original.

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `animal` (Slay Animal sword vs white harpy) | `TRAIT ws_animal=0`, `RATING auto=17 without=17 with=20`; 0/1 | `ws_animal=2`, `auto=20`; 1/1 |
| `nonanimal` (control, small kobold) | `auto=19 without=19 with=19`; 1/1 | byte-identical log |

Gates from the re-run's alltests logs: before Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines); after Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines).
Re-run toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, cmake version 3.28.3 (the original receipt records
gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3; the toolchain differs, and the files listed
below as byte-identical are identical nonetheless).
Corrected source sha256 matches the original receipt.
Render check of the committed `replay.html` with `tools/c-evidence/render-smoke.js`
(Playwright Chromium, every option and button clicked): `RENDER-SMOKE actions=4 errors=0 OK`.
The original receipt records its own headless check under the `browser` key.
The regenerated `replay.html` and `replay-data.json` are byte-identical to the committed ones. The repeat command below pins the new fixtures commit `cafe3462c` (next section), which supersedes `65fa24e6c` and reproduces these two cases unchanged.

Where the logs are: [`reverify-2026-11-08/`](reverify-2026-11-08/) holds every log,
`result.json` and per-mode results of the re-run, with
[`reverify-2026-11-08/receipt.json`](reverify-2026-11-08/receipt.json) (pins, per-case
results, gate lines, toolchain, render result, sha256 of every file there).
10 re-run files are byte-identical to the hashes in the original
`receipt.json`; those not already committed were also placed at their original paths in this directory, so the original receipt now resolves for them: `after-animal.log`, `after-build.log`, `after-nonanimal.log`, `after-scenarios.log`, `before-animal.log`, `before-nonanimal.log`, `before-scenarios.log`.
The rest (`after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`) differ in
run-specific content (temporary paths, timings, build output of the different toolchain) and are
published only under `reverify-2026-11-08/`.

Repeat (fixtures pinned by sha; the fix sha is fixed inside `reproduce.py` at that pin, and the runner
refuses any other source diff):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/slay-239-evidence
git show cafe3462c68445753b6eec99542a7788b7d9afec:docs/audit/c-reports/slay-239/reproduce.py > /tmp/reproduce-slay-239.py
python3 /tmp/reproduce-slay-239.py --full-tests --fixtures-ref cafe3462c68445753b6eec99542a7788b7d9afec
```

## Errata (2026-11-08)

- The source-proposal link near the top of this README was a placeholder; it now points to the source PR [#304](https://github.com/diegoami/neo-angband-tracker/pull/304) (evidence PR #305, canonical issue #239).
- The opening above says the correction "also derives the never-set kill traits"; the original package did not test that half. The next section adds a case that does.
- `receipt.json` was amended on 2026-11-08 for this only: its `README.md` hash now covers the README with these sections and the link fix, and a `render_check` key records the smoke check above; the old→new values are listed under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.

## Kill-trait case (2026-11-08, fixtures `cafe3462c`)

The correction has a second half the original package never exercised:
it sets the race-destroying "kill" traits (`BI_WK_UNDEAD`, `BI_WK_DEMON`,
`BI_WK_DRAGON`) from a wielded multiplier-five slay. Fixtures commit
`cafe3462c68445753b6eec99542a7788b7d9afec` adds the `kill-undead` mode:
the same level-31 warrior wields a Broad Sword carrying the undead
multiplier-five slay (`UNDEAD_5`, as a *Slay Undead* weapon has) with the
torch in the later light slot, and faces an adjacent zombified orc. The
scenario prints the traits as `borg_notice()` computed them and the thrust
value as computed, with only the plain undead slay forced on (`slay_only`,
three times) and with the kill trait forced on (`kill`, five times).

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `kill-undead` | `TRAIT ws_undead=0 wk_undead=0`, `RATING auto=11 slay_only=24 kill=36`; 0/1 | `ws_undead=5 wk_undead=1`, `auto=36`; 1/1 |
| `animal` | `auto=17`; 0/1 (as above) | `auto=20`; 1/1 |
| `nonanimal` (control) | 1/1 | byte-identical log |

What this shows: with the correction the kill trait is set and the attack
is valued at the five-times reading (36). A correction that only
accumulated the plain slays (`|=`) without the kill derivation would stop
at the slay-only reading (24); the original reaches neither (11), because
the light slot also erases the plain slay. Only `BI_WK_UNDEAD` is
exercised; the demon and dragon lines are the same shape and are verified
by reading only. Gates at this pin: 965/965 unit checks and 5/5 frontend
tests per phase. The `animal`/`nonanimal` logs equal the receipt-pin re-run
except one line in `before-animal.log`: the failed assertion's source line
number (221 → 251, because `scenario.c` grew).

Run output, receipt (pins, cases, gates, render check, sha256 of every
file) and the regenerated three-situation replay are in
[`reverify-2026-11-08-kill-undead/`](reverify-2026-11-08-kill-undead/)
(`replay.html` there: render-smoke `actions=6 errors=0 OK`, generated
script passes `node --check`). **This fixtures pin supersedes
`65fa24e6c`** as the package's repeat pin; the top-level `replay.html`,
logs and `receipt.json` remain the original two-situation run.

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/slay-239-evidence
git show cafe3462c68445753b6eec99542a7788b7d9afec:docs/audit/c-reports/slay-239/reproduce.py > /tmp/reproduce-slay-239.py
python3 /tmp/reproduce-slay-239.py --full-tests --fixtures-ref cafe3462c68445753b6eec99542a7788b7d9afec
```

## Review shape: `MAX()` (2026-11-13, fix `720ab3c4f`, fixtures `7e8af1f97`)

Upstream review of [angband/angband#6817](https://github.com/angband/angband/pull/6817)
suggested combining each slot's slay with `MAX()` rather than `|=`, as the
combat code does for multipliers from different items. `item->slays` holds
the multiplier (2, 3 or 5), so `|=` could merge two different multipliers
for one race into a value that is neither (3 | 5 = 7); `MAX()` keeps the
real one. Every current reader of `BI_WS_*` only tests for non-zero, so
behaviour does not change. The kill-trait derivation reads each item's own
multiplier and is unaffected.

New correction commit `720ab3c4f796c79d6108dd9d27a37fa573ca5917` sits on top of `9ab78ae60` (no rebase,
same base `e473720ed`); `git diff e473720ed 720ab3c4f796c79d6108dd9d27a37fa573ca5917` is still the single
file `src/borg/borg-trait.c`. Fixtures commit
`7e8af1f97572f6dd11467ab1bb18ef12391be062` changes only the runner's pinned fix and
expected source text and the replay narration (it now names the `MAX()`
shape); `scenario.c`, `run-cases.py` and `borg.txt` are unchanged.

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `animal` | `ws_animal=0`, `auto=17`; 0/1 | `ws_animal=2`, `auto=20`; 1/1 |
| `kill-undead` | `ws_undead=0 wk_undead=0`, `auto=11`; 0/1 | `ws_undead=5 wk_undead=1`, `auto=36`; 1/1 |
| `nonanimal` (control) | `auto=19`; 1/1 | byte-identical log |

Every case log, per-phase result and scenario log is byte-identical to the
`|=` run in `reverify-2026-11-08-kill-undead/`; in the regenerated replay
data only the two explanation strings differ. Gates per phase: 965/965
unit checks and 5/5 frontend tests. Corrected source sha256
`5bd3aa8a0c8711d0c021c3e1fccfeae982545da9da398b0af2935ec84a9f1cfc`.
Replay render check (headless Chromium, every option and button):
`actions=4 errors=0`, page names `MAX()`. Run output, replay and receipt
(pins, cases, gates, toolchain, sha256 of every file):
[`reverify-2026-11-13-max/`](reverify-2026-11-13-max/).
**This pin supersedes `cafe3462c`** as the package's repeat pin.

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/slay-239-evidence
git fetch https://github.com/diegoami/neo-angband-tracker.git task/c-borg-slay-traits-max
git show 7e8af1f97572f6dd11467ab1bb18ef12391be062:docs/audit/c-reports/slay-239/reproduce.py > /tmp/reproduce-slay-239.py
python3 /tmp/reproduce-slay-239.py --full-tests --fixtures-ref 7e8af1f97572f6dd11467ab1bb18ef12391be062
```
