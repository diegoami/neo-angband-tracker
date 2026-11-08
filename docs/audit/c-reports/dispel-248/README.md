# Dispel Life was rated against nobody

A Necromancer considers Dispel Life against a hill giant five grids
away. The game's drain hits every living monster in line of sight, and a
native cast removes 66 HP from the giant.

**The Borg's rating kept damage only for monsters flagged both
nonliving and undead — the exact inverse of the game's rule — and since
no monster in the game data carries the nonliving flag, the spell was
rated below doing nothing against every enemy.** The original values it
at −21 (the launch failure minus the spell's mana cost); the corrected
filter, which zeroes damage for undead or nonliving monsters like the
game does, values it at 19.

Two situations stay unchanged: an explicitly undead vampire bat is
immune in the game (a native cast removes nothing) and both versions
refuse the cast; and the class's primary nether bolt, rated against the
same giant, produces a byte-identical complete log before and after the
correction.

Consequence: the Borg never considered casting a spell that works, and
mis-ranked it in every comparison that included it. No death or
survival claim is made.

Canonical report: [issue248](https://github.com/diegoami/neo-angband-tracker/issues/248).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/291).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dispel-248-evidence
git show 6df50d2e5ad605ec8d3898ca1b653985a373480c:docs/audit/c-reports/dispel-248/reproduce.py > /tmp/reproduce-dispel-248.py
python3 /tmp/reproduce-dispel-248.py --fixtures-ref 6df50d2e5ad605ec8d3898ca1b653985a373480c --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the living-monster
filter, runs three recorded situations per variant, checks the
fail-before/pass-after matrix and the byte-identical control, records
both full gates and writes an offline `replay.html`. The native
verification ran with these exact generators except the replay
generator's syntax, which was completed afterwards; no native log,
source variant or gate differs.

Open [replay.html](replay.html) (download raw and open locally). Choose
**A living giant the spell really hurts**, then **The rating** and **The
native cast**. The original values the spell at −21 while the recorded
native cast removes 66 HP; the correction values it at 19. The other
two situations are controls.

Actual recorded output, original (`before-living.log`):

```text
GATE spell_okay=1 legal=1 temp_n=1
LAUNCH direct=-1
RATING value=-21 expected_average=39 clevel=26
OUTCOME hp_before=225 hp_after=159 hp_lost=66 undead=0
```

Corrected (`after-living.log`): `LAUNCH direct=39`,
`RATING value=19 ...`; the native cast line is unchanged (the game is
untouched by the fix). Undead control: value ≤ 0 in both versions and
`hp_lost=0`. Bolt control: byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`6912b85cb7b902616ad786dfc20afe3b4cf28b10`. One case in
`src/borg/borg-fight-attack.c`. The original zeroed the damage twice —
for monsters without the NONLIVING flag and again for those without
UNDEAD — leaving damage only for monsters with both. The game's drain
(project-mon.c MON_DRAIN with monster_is_nonliving = undead or
nonliving) damages each living monster; the correction zeroes the
damage for undead or nonliving monsters, matching it.

```sh
git fetch --no-tags origin task/c-borg-dispel-life-filter
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-fight-attack.c | sed -n '580,596p'
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/mon-predicate.c | sed -n '35,55p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 6912b85cb7b902616ad786dfc20afe3b4cf28b10 -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Necromancer level 26, depth 30, seed 1000, lit constructed
40x40 arena, the Fear and Torment shadow book, Dispel Life and Nether
Bolt learned, full mana, a torch, one awake monster. The production
rating call and its damage expression are used unchanged; the native
cast goes through the game's own command and its damage is measured on
the monster's hit points. Damage rolls are random; the rating is the
average expression. This isolates the rating filter, not full
autonomous play. No death or survival claim.

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
(base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `6912b85cb7b902616ad786dfc20afe3b4cf28b10`, fixtures `77bddc19421eaff33c70f6cebc2588c8ca632ce8`) with `--full-tests`.
Every case reproduced the recorded outcome; the runner's per-case results
(`result.json` `cases`) are identical to the committed original.

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `living` (hill giant) | rating `value=-21` (expected average 39), launch `direct=-1`; native cast removes 66 HP; 0/1 | `value=19`, `direct=39`; same 66 HP cast; 1/1 |
| `undead` (unchanged case) | `value=-21`, cast removes 0 HP; 1/1 | same values, 1/1 |
| `bolt` (control) | `bolt_control=11`; 1/1 | byte-identical log |

Gates from the re-run's alltests logs: before Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines); after Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines).
Re-run toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, cmake version 3.28.3 (the original receipt records
gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3; the toolchain differs, and the files listed
below as byte-identical are identical nonetheless).
Corrected source sha256 matches the original receipt.
Render check of the committed `replay.html` with `tools/c-evidence/render-smoke.js`
(Playwright Chromium, every option and button clicked): `RENDER-SMOKE actions=9 errors=0 OK`.
The original receipt records its own headless check under the `browser` key.
At the receipt pin `77bddc194` the runner finished every native step, assertion and gate and wrote `result.json`, then stopped with exit 1 in the last step because `make_replay.py` at that pin has a Python syntax error (see Errata and `reverify-2026-11-08/runner-stdout.txt`). Running the fixed generator from `6df50d2e5` on this re-run's logs regenerates `replay.html` and `replay-data.json` byte-identical to the committed ones. The repeat command below therefore pins `6df50d2e5`, the guide's own pin, whose generators differ from `77bddc194` only by that fix.

Where the logs are: [`reverify-2026-11-08/`](reverify-2026-11-08/) holds every log,
`result.json` and per-mode results of the re-run, with
[`reverify-2026-11-08/receipt.json`](reverify-2026-11-08/receipt.json) (pins, per-case
results, gate lines, toolchain, render result, sha256 of every file there).
12 re-run files are byte-identical to the hashes in the original
`receipt.json`; those not already committed were also placed at their original paths in this directory, so the original receipt now resolves for them: `after-bolt.log`, `after-build.log`, `after-living.log`, `after-scenarios.log`, `after-undead.log`, `before-bolt.log`, `before-living.log`, `before-scenarios.log`, `before-undead.log`.
The rest (`after-alltests.log`, `before-alltests.log`, `before-build.log`, `configure.log`) differ in
run-specific content (temporary paths, timings, build output of the different toolchain) and are
published only under `reverify-2026-11-08/`.

Repeat (fixtures pinned by sha; the fix sha is fixed inside `reproduce.py` at that pin, and the runner
refuses any other source diff):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dispel-248-evidence
git show 6df50d2e5ad605ec8d3898ca1b653985a373480c:docs/audit/c-reports/dispel-248/reproduce.py > /tmp/reproduce-dispel-248.py
python3 /tmp/reproduce-dispel-248.py --full-tests --fixtures-ref 6df50d2e5ad605ec8d3898ca1b653985a373480c
```

## Errata (2026-11-08)

- `make_replay.py` at the receipt's `fixtures_ref` `77bddc194` does not parse (a `}` closes a `(` on line 7), so a run at that pin ends with exit 1 after all native checks. The capture commit `466cdf8db` fixed it; `git diff 77bddc194 6df50d2e5` touches no other generator. Use the guide's pin `6df50d2e5`.
- Committed file that differs from its receipt hash (before this amendment): `README.md`. The receipt was hashed at `efd476b45`; commit `98292a983` then replaced the `pull/SOURCE` placeholder with `pull/291` in the README without re-hashing. No log, generator or replay file was affected.
- `receipt.json` was amended on 2026-11-08 for this only: its `README.md` hash now covers the README with these sections, and a `render_check` key records the smoke check above; the old→new values are listed under `receipt_amendments` in `reverify-2026-11-08/receipt.json`.
