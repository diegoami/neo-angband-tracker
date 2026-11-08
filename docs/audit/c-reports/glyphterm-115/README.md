# Drop the always-zero glyph count from the outlying rune score

This is **glyphterm-115 / issue #115**, a C Borg cleanup with no behaviour change (owner decision)
(diegoami/neo-angband-tracker#115).

Cleanup, no behaviour change; owner decision. Before fighting Morgoth the Borg builds a Sea of Runes, a 5x5 block of Runes of Protection around a centre. Once the centre is laid, borg_flow_glyph() scores each unglyphed grid of the block as 500 + (tmp_glyph * 500) - cost. tmp_glyph is only counted when a new sea is being placed, so in this branch it is always 0 and the score is simply 500 - cost: the Borg glyphs the nearest unglyphed grid next. The change writes the score as 500 - cost, one line, which makes that nearest-first choice explicit.

## What to watch

Outlying-rune progress (1 = the Borg chose an outlying grid and read a rune on it); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Changed (fix `87323a76f`) |
| --- | --- | --- |
| Control: on an unglyphed grid next to the laid centre | passed, progress=1 | byte-identical log |
| Control: as above, with twelve runes already laid in the block | passed, progress=1 | byte-identical log |

- **Control: on an unglyphed grid next to the laid centre.** Both versions score the Borg's own grid in the old-centre branch (cost 0, score 500), choose it and read a Rune of Protection on the next call. The logs are byte-identical.

- **Control: as above, with twelve runes already laid in the block.** Same choice with a partly built sea: the existing runes do not enter the old-centre score in either version. The logs are byte-identical.

## Conditions

Native Human Warrior, seed 115, level 50, depth 99, a full-size open level (198x66), 40 known Rune of Protection scrolls. The production borg_flow_glyph() runs as the Borg would call it: the first call chooses the Borg's own grid as the centre, the second arrives and reads a Rune of Protection (the fixture then lays that rune on the grid, the result of the read). The fixture then moves the Borg one grid east, onto an unglyphed grid of the 5x5 block, and up to two further calls must choose a grid in the changed old-centre branch and read a rune there. 'ring': twelve grids of the block west of the Borg already carry runes (laid by the fixture). Fixture interventions: the arena, lighting and the Borg's map are set directly; runes are laid by the fixture after a successful read; the player is moved by the fixture.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glyphterm-115
git fetch https://github.com/diegoami/neo-angband-tracker.git task/glyphterm-115-evidence
git show 8611139ae2e6822711152950aae96572090770a7:docs/audit/c-reports/glyphterm-115/reproduce.py > /tmp/reproduce-glyphterm-115.py
python3 /tmp/reproduce-glyphterm-115.py --full-tests --fixtures-ref 8611139ae2e6822711152950aae96572090770a7 --fix-ref 87323a76f8e2aa75524482a6f2668673fc174ad9
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
change is exactly the expected change in `src/borg/borg-flow-glyph.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `87323a76f8e2aa75524482a6f2668673fc174ad9` (branch `task/glyphterm-115`), fixtures
  `8611139ae2e6822711152950aae96572090770a7` (branch `task/glyphterm-115-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: offcenter, ring
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

In borg_flow_glyph()'s old-centre branch tmp_glyph is never incremented (only the new-sea branch counts glyphs), so 500 + (tmp_glyph * 500) - cost equals 500 - cost. Two native Sea of Runes scenarios that reach the changed line (the Borg on an unglyphed grid of the block, with and without twelve runes already laid) give byte-identical logs before and after, and both full native gates pass on both builds.

This is a cleanup with no failing mode: the scenarios show unchanged behaviour, they cannot show a difference. At the pristine base the reverse flow used by this function only assigns a cost to the Borg's own grid (tracked separately as #345, pair #424), so each call scores a single candidate in the old-centre branch; the ordering between several candidates is not exercised natively here and equivalence for it rests on the source argument (tmp_glyph is 0 in this branch). Not measured: a whole Sea of Runes or a Morgoth fight.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
