# Resist-electricity defence logs the acid note

This is **resielec-174 / issue #174**, a C Borg defect
(diegoami/neo-angband-tracker#174).

A Warrior carrying an elemental ring stands next to an awake monster whose bites are electric. The Borg's defence chooser picks the resist-electricity defence and, on the acting pass, writes its note into the log. In the original the note says Attempting to cast RAcid, a copy of the acid routine's text one function above; the corrected routine names the resistance it actually attempts, RElec. The queued keys are identical in both variants.

## What to watch

1: the note names RElec; 0: it does not; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `d7e2377dc`) |
| --- | --- | --- |
| Electric bites: the note must name RElec | failed, note_elec=0 | passed, note_elec=1 |
| Plain melee (black ogre): unchanged control | passed, note_elec=0 | byte-identical log |

- **Electric bites: the note must name RElec.** The Borg chooses the resist-electricity defence against the energy hound. The original writes the acid routine's RAcid text; the correction writes RElec. Keys are Ac*5 in both variants.

- **Plain melee (black ogre): unchanged control.** The same ring and chooser against a monster with no electric damage: the electricity routine is entered, finds no improvement, and no cast note is written. The complete log is byte-identical between the variants.

## Conditions

Native arena fixture (24x40), Human Warrior at character level 33, full hit points (avoidance 424), a worn Ring of Lightning (its RES_ELEC value derives after player_learn_all_runes), no heal items, and one awake energy hound (bite 2d6 electric) adjacent south-east; depth 18, seed 174, borg_rand_local 1000. The danger at the Borg's grid is 96 against a temporary-resistance danger of 88, so the routine's three improvement gates hold (p1 > p2, p2 <= avoidance/2, p1 > avoidance/7). The control wears the same ring next to a black ogre, whose plain melee gives the routine no improvement, so no cast note is written at all. Measured: the production borg_caution() -> borg_defend() decision, the Borg notes it wrote, and the queued keys. Not measured: the real game's cast roll for the ring activation, other Borg turns, and any effect of the note on later Borg decisions.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/resielec-174
git fetch https://github.com/diegoami/neo-angband-tracker.git task/resielec-174-evidence
git show ee22d19ebb3cfa07fc0bbe271a167f77362d79cc:docs/audit/c-reports/resielec-174/reproduce.py > /tmp/reproduce-resielec-174.py
python3 /tmp/reproduce-resielec-174.py --full-tests --fixtures-ref ee22d19ebb3cfa07fc0bbe271a167f77362d79cc --fix-ref d7e2377dcf9c4e6d690872e55db9e6138028a362
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-fight-defend.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6`, fix `d7e2377dcf9c4e6d690872e55db9e6138028a362` (branch `task/resielec-174`), fixtures
  `ee22d19ebb3cfa07fc0bbe271a167f77362d79cc` (branch `task/resielec-174-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: control
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

Next to an awake electric biter, a Borg carrying a lightning ring prepares its electricity defence through the production caution and defence chooser, and the note it writes named the acid routine's text (RAcid) on the original and the electricity text (RElec) with the correction, with identical queued keys. The plain-melee control, where the routine finds no improvement and writes no note, is byte-identical between the variants, and both full native gates pass at 965/965 and 5/5 in each variant.

The fixture measures the note text and the queued keys of the defence decision, not the game's later roll for the ring activation or any other Borg turn. The note is log text: no damage, movement or death is claimed either way.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
