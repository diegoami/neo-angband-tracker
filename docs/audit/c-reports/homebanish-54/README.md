# The Borg's home score ignored Scrolls of Banishment

This is **homebanish-54 / issue54**, a C Borg defect
(diegoami/neo-angband-tracker#54).

The home score rewards stocked Scrolls of Banishment (genocide) and Mass Banishment, 'mainly used for Morgoth'. The scan that counts the home's scrolls only counted Mass Banishment, so the Banishment reward was always zero and the Borg never offered a Banishment scroll to the home. The correction counts them.

## What to watch

The Borg offers a Banishment scroll to the home (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `e49ab9480`) |
| --- | --- | --- |
| Pack holds a Scroll of Banishment | failed, stores_banishment_scroll=0 | passed, stores_banishment_scroll=1 |
| Control: pack holds a Scroll of Mass Banishment | passed, stores_banishment_scroll=1 | byte-identical log |

- **Pack holds a Scroll of Banishment.** A level-26 Mage with an empty home carries a Scroll of Banishment, a Scroll of Word of Recall, a ration and a spell book. The production home-store routine decides what to put in the home. Original: the home score gives a Banishment scroll nothing, so the Borg leaves it in the pack. Corrected: the scroll is worth the same as other stocked scrolls and the Borg offers it to the home.

- **Control: pack holds a Scroll of Mass Banishment.** The same Mage with a Scroll of Mass Banishment, which the home score already counted. The Borg offers it to the home before and after.

## Conditions

Native Human Mage, seed 54, character level 26, 5000 gold, depth 0, a lit open arena. All shops and the home are emptied; the pack holds the starting spell book, a ration, a Scroll of Word of Recall and one scroll under test. The production borg_think_home_sell_useful() decides; nothing is walked or stored. Fixture interventions: the stores are emptied and the item runes are known.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/homebanish-54
git fetch https://github.com/diegoami/neo-angband-tracker.git task/homebanish-54-evidence
git show 8484955b27f253e0d9c99526be334fe7346888e9:docs/audit/c-reports/homebanish-54/reproduce.py > /tmp/reproduce-homebanish-54.py
python3 /tmp/reproduce-homebanish-54.py --full-tests --fixtures-ref 8484955b27f253e0d9c99526be334fe7346888e9 --fix-ref e49ab94809d6f18d0b1d155ff2b1bc6ba8d3db78
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-home-notice.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `e49ab94809d6f18d0b1d155ff2b1bc6ba8d3db78` (branch `task/homebanish-54`), fixtures
  `8484955b27f253e0d9c99526be334fe7346888e9` (branch `task/homebanish-54-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: mass
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With an empty home, the original Borg left a Scroll of Banishment in its pack because the home score gave it no value, while it offered a Scroll of Mass Banishment. With the scroll counted it offers the Banishment scroll too. The Mass Banishment control is byte-identical.

Valuation only, low priority: this changes whether one scroll is offered to an empty home. It does not measure the genocide defence, a later fight, or whether keeping the scroll in the home rather than the pack helps; the home score treats Banishment like the other stocked scrolls, as the code comment intends.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
