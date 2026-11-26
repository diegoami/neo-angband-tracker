# Vein flow commits toward an undiggable quartz vein

This is **veinmode-280 / issue #280**, a C Borg defect
(diegoami/neo-angband-tracker#280).

When the Borg decides to mine a vein for treasure, it checks once whether it can dig the minimum vein kind; after enough twitching that minimum drops to magma, which admits a borg whose digging covers magma only. The scan then enqueues every tracked vein without a per-target check, so the Borg can commit its flow toward a near quartz vein it has no hope of digging - and the dig it later attempts is refused, leaving the flow making no progress. The correction checks each vein's diggability while building the target list, exactly as the donor fork's patch does. A capable borg targets quartz as before.

## What to watch

6: stepped east to the diggable magma vein; 8: stepped north to the undiggable quartz vein; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `083dd7b0a`) |
| --- | --- | --- |
| Near quartz, far magma: the flow must take the diggable vein | failed, keys=n/a | passed, keys=n/a |
| Only the magma vein: unchanged control | passed, keys=n/a | byte-identical log |
| Every vein diggable: unchanged control | passed, keys=n/a | byte-identical log |

- **Near quartz, far magma: the flow must take the diggable vein.** The borg can dig magma but not quartz, and has twitched past the threshold that lowers the vein gate to magma. The original commits toward the near quartz vein it can never dig (step 8); the correction skips it and commits toward the magma vein (step 6).

- **Only the magma vein: unchanged control.** With the quartz vein removed, both variants commit to the diggable magma vein (step 6) and the complete log is byte-identical.

- **Every vein diggable: unchanged control.** A borg with a wielded Shovel and digging 60 passes the per-target check on both variants, mines the near quartz vein (step 8) as before, and the complete log is byte-identical.

## Conditions

Native arena fixture (24x40), Human Warrior at character level 23, digging 13 (no swap digger, no Stone-to-Mud), twitch counter 27 so the vein flow's own gate runs at the lowered magma threshold and admits this borg (threshold 13 exactly); a quartz vein with treasure three grids north and a magma vein with treasure four grids east, both registered as seen; depth 14, seed 280, borg_rand_local 1000, no monsters. The production vein decision runs with the dungeon brain's arguments (borg_flow_vein(true, 35)). The magma-only control removes the quartz vein; the capable control carries a wielded Shovel and a digging skill of 60, making every vein diggable. Measured: the flow commitment, the queued step key and the borg's flow note, plus the diggability states. Not measured: the real dig command the committed flow would later queue and refuse, further turns of the twitch loop, and any spell-based digging beyond the capability states printed.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/veinmode-280
git fetch https://github.com/diegoami/neo-angband-tracker.git task/veinmode-280-evidence
git show 855046c85d77b24ed9575a3842f3b749661b13fa:docs/audit/c-reports/veinmode-280/reproduce.py > /tmp/reproduce-veinmode-280.py
python3 /tmp/reproduce-veinmode-280.py --full-tests --fixtures-ref 855046c85d77b24ed9575a3842f3b749661b13fa --fix-ref 083dd7b0af3097aec2ed47759dead7224f52a8f8
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-flow-misc.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6`, fix `083dd7b0af3097aec2ed47759dead7224f52a8f8` (branch `task/veinmode-280`), fixtures
  `855046c85d77b24ed9575a3842f3b749661b13fa` (branch `task/veinmode-280-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: magma-only, capable
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

A magma-capable, quartz-incapable borg standing three grids from an undiggable quartz vein and four from a diggable magma vein commits its vein flow toward the quartz vein on the original (step key 8) and toward the magma vein with the per-target diggability check (step key 6). The magma-only and every-vein-diggable controls behave identically on both variants with byte-identical logs, and both full native gates pass at 965/965 and 5/5 in each variant.

The fixture measures the production vein-flow commitment and the diggability states, not the later dig command and its refusal message, and not the twitch loop's long-run behaviour. The twitch counter, digging skill and vein placement are disclosed fixture interventions that set up the reported state.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
