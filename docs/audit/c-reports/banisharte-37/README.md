# The Borg never banished evil monsters with the artifact that does it

This is **banisharte-37 / issue37**, a C Borg defect
(diegoami/neo-angband-tracker#37).

The Banishment defence can activate the Great Axe of Eönwë, which banishes evil monsters, but only while the Borg has more than 100 hit points. The test read borg.has[BI_CURHP]; borg.has[] is indexed by object kind, so this counts how many of some unrelated object kind the Borg carries and is almost always zero. The artifact route was therefore never taken. The correction tests borg.trait[BI_CURHP].

## What to watch

The Borg banishes the trolls with the artifact (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `caa6c6acc`) |
| --- | --- | --- |
| Three cave trolls next to a healthy Warrior wielding the Axe of Eönwë | failed, banishes_evil=0 | passed, banishes_evil=1 |
| Control: the same scene with 90 hit points | passed, banishes_evil=0 | byte-identical log |

- **Three cave trolls next to a healthy Warrior wielding the Axe of Eönwë.** A level-35 Warrior with 431 hit points wields the Great Axe of Eönwë, whose activation banishes evil monsters, and three awake cave trolls stand next to it (danger 138). The production defence routine chooses a defence. Original: the artifact test reads an unrelated object-kind count, is false, and the Banishment defence is not available; the Borg quaffs a Potion of Berserk Strength (value 5). Corrected: the Borg activates the axe and banishes the three trolls (value 138).

- **Control: the same scene with 90 hit points.** The same Warrior with 90 hit points. The artifact route needs more than 100, so it is refused before and after, and the routine does nothing in both.

## Conditions

Native Human Warrior, seed 37, character level 35, 431 hit points (control: 90), depth 5, a lit open arena. The Warrior wields the Great Axe of Eönwë (made from the artifact definition and fully known) and three awake cave trolls stand on the adjacent grids to the east. The production borg_defend() chooses and carries out the defence; the keypresses are read, not played. Fixture interventions: the artifact is created and wielded by the fixture; the monsters are placed and observed by the fixture; the Borg's map update is not run.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/banisharte-37
git fetch https://github.com/diegoami/neo-angband-tracker.git task/banisharte-37-evidence
git show 97bcf2a980749472b5ce1dba44ac7cb054ca274d:docs/audit/c-reports/banisharte-37/reproduce.py > /tmp/reproduce-banisharte-37.py
python3 /tmp/reproduce-banisharte-37.py --full-tests --fixtures-ref 97bcf2a980749472b5ce1dba44ac7cb054ca274d --fix-ref caa6c6acc70c35ced4bfaaf2781036d99b5d32e5
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-fight-defend.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `caa6c6acc70c35ced4bfaaf2781036d99b5d32e5` (branch `task/banisharte-37`), fixtures
  `97bcf2a980749472b5ce1dba44ac7cb054ca274d` (branch `task/banisharte-37-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: hurt
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With three cave trolls adjacent, the original Borg could not use the Eönwë axe (its hit-point test read the wrong array) and quaffed a Potion of Berserk Strength; with the current hit points tested it activates the axe and banishes the trolls. With 90 hit points both versions refuse the artifact route, and that control is byte-identical.

Priority medium: this measures which defence the routine chooses in one constructed fight, not the fight. The artifact is rare (it needs depth 30 and above to appear); the same test shape is not changed anywhere else. The partial danger figures printed in the banishing notes are the existing estimate.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
