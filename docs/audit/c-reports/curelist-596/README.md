# A blind Borg ate Waybread, which does not cure blindness

This is **curelist-596 / issue596**, a C Borg defect (diegoami/neo-angband-tracker#596), found during the strategy-manual comparison (C-FIX-POSTING §8.1).

When blind, `borg_recover()` tries a list of cures in order. **Elvish Waybread** was second, ahead of Cure Light Wounds (and its activation form was on the list too). In 4.2 Waybread nourishes, heals 4d8 and cures **poison only** (`object.txt`), so the Borg spent its turn and a Waybread and stayed blind. Its stun lists likewise included the **HEAL1** and **HEAL2** activations, which heal and cure cuts but **not stun** (`activation.txt`; only HEAL3 and the Rod of Healing do). The correction drops those entries; the Waybread stays on the poison list, where it belongs.

## What to watch

Out of 100 decisions, how often the blind Borg eats Waybread; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `42cd156ac`) | Corrected (fix `ac214e76b`) |
| --- | --- | --- |
| Blind, carrying Waybread and Cure Light Wounds | failed: eats Waybread in **25** of 100 decisions, never Cure Light Wounds | passed: drinks **Cure Light Wounds** in 25, Waybread in 0 |
| Control: blind, Cure Light Wounds only | passed: Cure Light Wounds in 25 | byte-identical log |

`borg_recover()` tries the blindness cures only on about a quarter of its decisions (`q = randint0(100)`, `q < 25`); the other 75 do nothing in both builds.

## Conditions

Native Human Warrior, seed 596, character level 10, dungeon level 10, lit room, no monster, blind. Carried: three Pieces of Elvish Waybread and three Potions of Cure Light Wounds (Cure Light Wounds only in the control), plus the starting rations. Fixture interventions: the blindness timer is set; the items are made directly. The production `borg_recover()` decides 100 times with the random number generator reset to seeds 1–100; each decision is classified by the key it queues (`E` eat, `q` quaff), because the message log merges repeated notes. The stun half is checked against the data files only, not run (it needs one of four artifacts).

[replay.html](replay.html) is the offline page (download raw, open locally): choose a scenario and press **Show decision**; **Show raw logs** gives the complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/curelist-596
git fetch https://github.com/diegoami/neo-angband-tracker.git task/curelist-596-evidence
git show c74322650791a20c8cdca6692160d22818f4039c:docs/audit/c-reports/curelist-596/reproduce.py > /tmp/reproduce-curelist-596.py
python3 /tmp/reproduce-curelist-596.py --full-tests --fixtures-ref c74322650791a20c8cdca6692160d22818f4039c --fix-ref ac214e76bc9560ce21d2e1c9479d20436544226a
```

The runner asserts the correction is exactly `fix.patch` (`src/borg/borg-recover.c` only), builds the original and corrected native C game in a disposable worktree, runs both scenarios, checks each expected result and the byte-identical control, runs both full native gates and writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`; the frontend tests need a UTF-8 locale (for example `LANG=C.UTF-8`).

## Recorded receipt (this run)

- base `42cd156ac9f95ddf4311e88dd7ad462292d6d1d6` (angband/master), fix `ac214e76bc9560ce21d2e1c9479d20436544226a` (branch `task/curelist-596`), fixtures `c74322650791a20c8cdca6692160d22818f4039c` (branch `task/curelist-596-evidence`); later commits only add these artifacts.
- control byte-identical: control
- full native gates: before and after 965/965 unit checks and 5/5 frontend tests
- corrected source sha256 (`src/borg/borg-recover.c`): `aa675746ab097fca3ac356419e7228ddb94323c8e43bdd079c5e0012be247b8c`
- replay: Playwright Chromium headless render check, no console errors

## What this proves, and what it does not

A blind Borg carrying Waybread ate it instead of the Cure Light Wounds that cures blindness, every time it tried to cure blindness; with the correction it drinks the potion. Without Waybread nothing changes.

Not measured: the stun half (HEAL1/HEAL2, artifact-only), fights, or how often a blind Borg carries Waybread (it is found, not bought). Impact is low.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `fix.patch`, `borg.txt` (+ `profile/borg.txt`) are the generators; `before-*.log`, `after-*.log`, `*-results.json`, `result.json`, `run-*.log`, `replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other file) are the captured run.
