# The call-light scan never visited the Borg's own grid

A Warrior with a burned-out torch stands in an open doorway. The game
reports the doorway grid as lit; the Borg's own map of that grid
carries no glow mark — exactly the disagreement the out-of-light
routine's door case exists to repair: it cheats the truth from the
game (`square_isglow`) and sets the mark.

**The call-light scan iterated only the eight neighbouring grids
(`ii < 8`), while the door case inside it tests the Borg's own grid
(`y == borg.c.y && x == borg.c.x`). The case could never fire, and the
is-the-door-lit cheat never ran.** Measured on the exact situation
(seed 1000, production `borg_think_dungeon_light()`, the own grid
planted without the glow mark as the case's repair state): the
original leaves the doorway unmarked; the corrected scan includes
index 8 — the Borg's own grid (`ddx_ddd[8] == ddy_ddd[8] == 0`) — the
door case fires, and the mark is repaired from the game.

A control with a plain lit floor underfoot stays byte-identical (both
variants leave the marks alone). Including the own grid also lets the
routine light a dark own grid like any other grid, which is the
section's stated purpose ("illuminate the entire dungeon, looking for
stairs"). No damage or survival claim beyond the tracked mark is made.

Canonical report: [issue231](https://github.com/diegoami/neo-angband-tracker/issues/231).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal: PR (see the issue).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/calllight-231-evidence
git show f7058cd56f58938d68c37523816a99020efd8f75:docs/audit/c-reports/calllight-231/reproduce.py > /tmp/reproduce-calllight-231.py
python3 /tmp/reproduce-calllight-231.py --fixtures-ref f7058cd56f58938d68c37523816a99020efd8f75 --full-tests
```

The runner builds two variants (original, corrected) from pristine C
`e473720` in fresh temporary directories, verifies the source diff is
exactly the own-grid inclusion, runs two recorded situations on each,
compares controls and gates, and writes `replay.html` for offline
review. Your checkout and character/profile are not used or edited.
The disposable source and binary finish on the correction.

The native run recorded in this directory's logs executed at fixtures
pin `f7058cd56f58938d68c37523816a99020efd8f75`. The captured replay,
its data and this guide were then added on top; no generator change of
any kind followed the run (commit list: pin, then one artifact
commit).

Open [replay.html](replay.html) (download raw and open locally).
Choose **The Borg stands in an open doorway**, then **The doorway**
and **After the scan**: the original leaves the doorway unmarked, the
corrected repairs the mark. Choose **Control: the own grid is a plain
lit floor**: the logs match byte for byte between versions.

Actual recorded output, original (`before-own-open-door.log`; the
scenario asserts the repaired mark, so the original fails it — that is
the fail-before contract):

```text
SETUP ownfeat=3 ownglow_before=0 isglow=1
STATE ownglow=0
native-c/calllight-231 finished: 0/1 passed
```

Corrected (`after-own-open-door.log`):

```text
STATE ownglow=1
native-c/calllight-231 finished: 1/1 passed
```

Control (`before-floor.log` / `after-floor.log` — byte-identical
pair):

```text
STATE ownglow=1
```

Full native gates on both variants: 965/965 unit checks and 5/5
frontend tests (`before-alltests.log`, `after-alltests.log`).

### Conditions and limits

Native Human Warrior, depth 30, seed 1000, glowing constructed arena
(the doorway grid is lit in the game), a burned-out torch putting the
routine into its out-of-light path. The Borg's own grid is planted
without a glow mark — recorded as a fixture intervention, the repair
state the door case exists for. Spare torches and the recall scroll
are removed from the pack (recorded fixture intervention) so the
wear-stuff, refuel and recall branches have nothing to act on before
the scan. The production routine runs directly; the tracked mark is
read back, nothing is cast.

### File inventory

- `scenario.c` — the native observer (two modes via `CL231_MODE`).
- `run-cases.py` — runs each mode, strips ANSI, writes per-mode logs
  and `*-results.json`.
- `make_replay.py` — builds `replay.html` + `replay-data.json`.
- `reproduce.py` — the pinned runner described above.
- `borg.txt` — Borg config for the disposable profile.
- `before-own-open-door.log`, `after-own-open-door.log`,
  `before-floor.log`, `after-floor.log` — captured scenario logs (the
  control pair is byte-identical).
- `before-results.json`, `after-results.json` — parsed per-mode rows.
- `configure.log`, `before-build.log`, `after-build.log`,
  `before-scenarios.log`, `after-scenarios.log`,
  `before-alltests.log`, `after-alltests.log` — run-root logs.
- `result.json` — pins, per-case results, identical controls, gates,
  fixed-source sha256.
- `replay.html`, `replay-data.json` — the offline demo.
- `receipt.json` — sha256 map of every file in this directory.
