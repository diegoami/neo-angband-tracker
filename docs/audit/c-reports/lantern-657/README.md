# Light maintenance can wield an empty lantern over a burning torch

This is **lantern-657 / issue #657**, a C Borg defect
(diegoami/neo-angband-tracker#657).

When the light slot is empty, borg_maintain_light() picks the replacement light. It took any matching lantern from the pack before any torch, with no fuel check: a spent lantern was wielded, and a fuel-dependent light with no fuel produces no light - the borg then faces an equipped-lantern refuel it cannot satisfy while a working torch sits in its pack.

## What to watch

The Borg's fuel-aware light radius after the maintenance action (BI_LIGHT); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `574b618c3`) | Corrected (fix `d149c192c`) |
| --- | --- | --- |
| The reported defect: spent lantern + burning torch, no oil | failed, bi_light=n/a | passed, bi_light=2 |
| Only a spent lantern, no fuel of any kind | failed, bi_light=n/a | passed, bi_light=0 |
| Control: fueled lantern + torch | passed, bi_light=3 | byte-identical log |
| Control: spent lantern + flask of oil | passed, bi_light=0 | byte-identical log |
| Control: only a burning torch | passed, bi_light=2 | byte-identical log |

- **The reported defect: spent lantern + burning torch, no oil.** The light slot is empty; the pack holds a spent lantern (0 turns), a burning wooden torch and no oil. The original borg wields the lantern - BI_LIGHT stays 0, the borg is dark - and next faces an equipped-lantern refuel with no fuel. The corrected borg wields the torch (BI_LIGHT 2).

- **Only a spent lantern, no fuel of any kind.** The only light is a spent lantern; nothing can produce light. The original wields it and reports the need met (a false success); the corrected borg reports the need unmet and presses nothing, so other flows (buy a torch, refuel elsewhere) can react.

- **Control: fueled lantern + torch.** A fueled lantern and a torch. Both variants wield the lantern - the correction only changes which lanterns qualify, not the lantern-over-torch preference for producing lights. The complete logs are byte-identical.

- **Control: spent lantern + flask of oil.** Only an empty lantern and usable fuel: wielding it is correct because oil can light it, and the next maintenance call queues the refuel ('F'). Both variants do exactly this; the complete logs are byte-identical.

- **Control: only a burning torch.** No lantern at all: both variants wield the torch. The complete logs are byte-identical.

## Conditions

Native Human Warrior, level 20 (experience forced), depth 5 in a synthetic open lit arena (SQUARE_GLOW everywhere, so sight is not light-limited). The birth kit's torches are dropped: the light slot starts empty in every mode, which is the recorded situation. Pack contents per mode; a torch in the pack always burns (the engine destroys torches at zero turns). The borg is initialized with the production borg_init()/borg_equipment()/borg_inventory()/borg_notice() sequence and the arrival borg_update() pass. The measured decision is borg_maintain_light() itself; the queued wield is then performed through the production inven_wield() so BI_LIGHT - the borg's fuel-aware light radius, which credits a fuel-dependent light only when its timeout is non-zero - reflects the action.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario to see the original and corrected runs side by side on the dungeon map, drawn with the game's own symbols (the legend under the maps explains them), press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lantern-657
git fetch https://github.com/diegoami/neo-angband-tracker.git task/lantern-657-evidence
git show 470528286648c592955ee61866315840647deace:docs/audit/c-reports/lantern-657/reproduce.py > /tmp/reproduce-lantern-657.py
python3 /tmp/reproduce-lantern-657.py --full-tests --fixtures-ref 470528286648c592955ee61866315840647deace --fix-ref d149c192c0e8aeb9b2375f161fb81d964940bc79
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-light.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix `d149c192c0e8aeb9b2375f161fb81d964940bc79` (branch `task/lantern-657`), fixtures
  `470528286648c592955ee61866315840647deace` (branch `task/lantern-657-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: fueledlantern, spentlantern_oil, torchonly
- full native gates: before 966/966 unit checks and 5/5 frontend tests;
  after 966/966 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

With an empty light slot the borg now wields a light source that actually produces light: a fueled (or everburning) lantern first, then a torch - which never arrives spent - and a spent lantern only when a flask of oil exists to light it. On the base commit it wielded the spent lantern, stayed dark (BI_LIGHT 0), and was then locked in the equipped-lantern refuel branch with no fuel while the torch sat in its pack; with no fuel anywhere it wielded the dark lantern and reported the need met. The lantern-over-torch preference for producing lights and the wear-then-refuel sequence with oil are unchanged (byte-identical controls).

The fixture measures the maintenance decision and the resulting fuel-aware light radius. The consumers of BI_LIGHT (the borg's light map, exploration, dark-grid danger) are identified in the source and their inputs change with the correction; no full dungeon walk is replayed here. The wield is performed through the production inven_wield; the queued keys are the production decision.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
