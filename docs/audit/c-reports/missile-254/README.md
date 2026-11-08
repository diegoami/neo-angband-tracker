# The Borg can try to fire with no target chosen

A Warrior with a bow and arrows knows a kobold it cannot shoot from
where it stands. The Borg's shot-lining movement flow asks whether a
distance attack exists; that probe reaches the missile launch routine
with the target list empty (the Teleport Other defense resets it, and
the launch routine itself does not refill it) and with the simulation
flag off (any acted attack or defense leaves it off; it also starts
off).

**No missile finds a target, so the chosen-missile index stays -1 — but
the routine's "nothing to use" guard tests the value, which never goes
negative once initialized to zero. The real call falls through to the
firing path: it reads the description of missile slot -1 (outside the
heap-allocated inventory array) and queues a fire key, a quiver
character computed from index -1, and the target key.** Under
AddressSanitizer the original aborts with a heap-buffer-overflow READ
at the note line (borg-fight-attack.c:2148); the corrected guard
returns "nothing to fire" and queues nothing.

Measured transition: original queues 3 keys (return 0); corrected
queues 0 keys (return 0). Two controls stay byte-identical before and
after: with a real target both versions queue the genuine firing keys
and value the shot at 4, and in simulation both value it and queue
nothing.

Consequence: three junk keys enter the Borg's input stream (the game
would see a fire command with an invalid quiver slot), and every probe
of this kind reads outside the inventory array. No death or survival
claim is made.

Canonical report: [issue254](https://github.com/diegoami/neo-angband-tracker/issues/254).
Lead supplied by borg-rs (fuzz cross-check); independently validated
and reproduced natively here, including the ordinary-play reachability
through the shot-lining flow (borg_think_dungeon → borg_flow_kill_aim →
borg_has_distance_attack → borg_attack_aux_launch).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/missile-254-evidence
git show 8b30eb5845ae542b41f2601d4370c044f13c21ac:docs/audit/c-reports/missile-254/reproduce.py > /tmp/reproduce-missile-254.py
python3 /tmp/reproduce-missile-254.py --fixtures-ref 8b30eb5845ae542b41f2601d4370c044f13c21ac --full-tests
```

The runner builds two normal variants (original, corrected) and two
AddressSanitizer variants from pristine C `e473720`, verifies the
source diff is exactly the chosen-index guard, runs three recorded
situations on each, checks the fail-before/pass-after matrix and
byte-identical controls, records both full gates and writes an offline
`replay.html`. Your checkout and character/profile are not used or
edited.

Open [replay.html](replay.html) (download raw and open locally). Choose
**Shot-lining flow asks about firing with no target**, then **The
probe** and **Keys queued**. The original queues three keys computed
from missile index -1; the correction queues nothing. The other two
situations are controls whose logs match byte for byte.

Actual recorded output, original (`before-no-target.log`):

```text
DECISION no-target return=0 keys=3 sequence=f5
OUTCOME return=0 keys=3
```

The middle queued key is the non-printing quiver character computed
from index -1. Corrected (`after-no-target.log`):

```text
DECISION no-target return=0 keys=0 sequence=
OUTCOME return=0 keys=0
```

ASAN original (`asan-before-no-target.log`): `heap-buffer-overflow READ`,
`borg_attack_aux_launch` at `borg-fight-attack.c:2148`, through
`my_strcpy` while formatting the note for missile slot -1. Corrected
asan: 1/1 passed, no sanitizer report. Scenario runs disable the strcmp
interceptor and leak check because the pristine base otherwise reports
an unrelated strcmp overread during init name sorting; the tracked
defect is a compiler-instrumented read and needs no interceptors.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`405e4fd4880e7bef27d6a6328765a904638c8257`. One guard in
`src/borg/borg-fight-attack.c`. The quiver scan keeps `b_k` at -1
whenever every candidate's `borg_launch_bolt()` returns -1, and the
existing guard `if (b_n < 0)` can never be true because `b_n` starts at
zero and candidates only raise it. The correction guards on the chosen
index: `if (b_k < 0) return 0;`.

Reachability: `borg_think_dungeon`'s movement flow calls
`borg_flow_kill_aim(true)`, whose probe `borg_has_distance_attack()`
calls `borg_attack_aux_launch()` directly. The target list is not
refilled there (the attack chooser normally fills it), and the
simulation flag is whatever the previous evaluation left — false after
any acted attack or defense, and false by initial value. borg-rs found
the read by fuzzing; this reproduction shows the ordinary-play path.

### Conditions and limits

Native Human Warrior level 10, depth 10, seed 1000, lit constructed
40x40 arena, short bow, twenty arrows, torch, one awake kobold five
grids away and known. The production launch routine is called
unchanged; the empty target list and the off simulation flag reproduce
the state earlier evaluations leave behind. This isolates the launch
guard, not full autonomous play. The junk keys are shown queued, not
executed against the game. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs including the asan
variants. [Full receipt](receipt.json).


## Re-verification (2026-11-08)

Why: the original run's phase, build and gate logs were listed and hashed in
`receipt.json` but never committed (the repository's `*.log` ignore rule
dropped them), so the receipt could not be checked against published bytes.
The same pinned runner was run again on 2026-11-08 at the receipt's own pins
(base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `405e4fd4880e7bef27d6a6328765a904638c8257`, fixtures `8b30eb5845ae542b41f2601d4370c044f13c21ac`) with `--full-tests` (normal and AddressSanitizer builds).
Every case reproduced the recorded outcome; the runner's per-case results
(`result.json` `cases`) are identical to the committed original.

| Case | Original (before) | Corrected (after) |
| --- | --- | --- |
| `no-target` (normal) | `DECISION return=0 keys=3 sequence=f5` (fire keys queued with no target); 0/1 | `keys=0`, nothing queued; 1/1 |
| `no-target` (asan) | heap-buffer-overflow `READ of size 1` (strlen ← format ← `borg_attack_aux_launch`, `borg-fight-attack.c:2148`); 0/1 | 1/1, no sanitizer report |
| `live-shot` (control) | `return=4 keys=11`; 1/1 | byte-identical log (normal and asan) |
| `simulated` (control) | `return=4 keys=0`; 1/1 | byte-identical log (normal and asan) |

Gates from the re-run's alltests logs: before Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines); after Total: 965/965 passed (100.0%), Total: 5/5 (0 `... Failed` lines).
Re-run toolchain: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, cmake version 3.28.3 (the original receipt records
gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0, cmake version 4.2.3; the toolchain differs, and the files listed
below as byte-identical are identical nonetheless).
Corrected source sha256 matches the original receipt.
Render check of the committed `replay.html` with `tools/c-evidence/render-smoke.js`
(Playwright Chromium, every option and button clicked): `RENDER-SMOKE actions=9 errors=0 OK`.
The original receipt records its own headless check under the `browser` key.
The regenerated `replay.html` and `replay-data.json` are byte-identical to the committed ones.

Where the logs are: [`reverify-2026-11-08/`](reverify-2026-11-08/) holds every log,
`result.json` and per-mode results of the re-run, with
[`reverify-2026-11-08/receipt.json`](reverify-2026-11-08/receipt.json) (pins, per-case
results, gate lines, toolchain, render result, sha256 of every file there).
20 re-run files are byte-identical to the hashes in the original
`receipt.json`; those not already committed were also placed at their original paths in this directory, so the original receipt now resolves for them: `after-asan-build.log`, `after-asan-scenarios.log`, `after-live-shot.log`, `after-no-target.log`, `after-normal-scenarios.log`, `after-simulated.log`, `asan-after-live-shot.log`, `asan-after-no-target.log`, `asan-after-simulated.log`, `asan-before-live-shot.log`, `asan-before-simulated.log`, `before-asan-scenarios.log`, `before-live-shot.log`, `before-no-target.log`, `before-normal-scenarios.log`, `before-simulated.log`.
The rest (`after-alltests.log`, `after-normal-build.log`, `asan-before-no-target.log`, `before-alltests.log`, `before-asan-build.log`, `before-normal-build.log`, `configure-asan.log`, `configure.log`) differ in
run-specific content (temporary paths, timings, build output of the different toolchain, sanitizer addresses and process ids) and are
published only under `reverify-2026-11-08/`.

Repeat (fixtures pinned by sha; the fix sha is fixed inside `reproduce.py` at that pin, and the runner
refuses any other source diff):

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/missile-254-evidence
git show 8b30eb5845ae542b41f2601d4370c044f13c21ac:docs/audit/c-reports/missile-254/reproduce.py > /tmp/reproduce-missile-254.py
python3 /tmp/reproduce-missile-254.py --full-tests --fixtures-ref 8b30eb5845ae542b41f2601d4370c044f13c21ac
```

## Errata (2026-11-08)

- `receipt.json` hashes `README.md` as it stood when the receipt was last written; this appended section changes the README, so that one hash no longer matches by design.
