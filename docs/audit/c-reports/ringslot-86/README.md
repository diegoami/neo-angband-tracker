# The Borg took off its good ring instead of its weak one when it found a better ring

This is **ringslot-86 / issue #86**, a C Borg defect
(diegoami/neo-angband-tracker#86).

At home the Borg works out its best set of equipment by trying every candidate item in every slot, the left ring slot before the right one. A check meant to stop the same ring going on both hands was also applied to the left slot, against the right-slot entry, which at that point still holds the last ring tried on the right hand earlier in the search. So a ring that had just been tried on the right hand was never tried on the left. A level-21 Warrior wearing a Ring of Feather Falling (left) and a Ring of See Invisible (right), with a Ring of Free Action in the pack, therefore never considered Free Action in place of Feather Falling: it took off See Invisible and put Free Action on, keeping the Feather Falling ring (Borg power 591441). The correction drops the left-slot check (the right-slot check already prevents wearing one ring twice); the Borg takes off Feather Falling instead and ends with See Invisible and Free Action (power 595941).

## What to watch

Still wearing the Ring of See Invisible at the end (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `aa3bf51c9`) |
| --- | --- | --- |
| Weak ring on the left hand, good ring on the right, better ring in pack | failed, wears_kept_ring=0 | passed, wears_kept_ring=1 |
| Control: good ring on the left hand, weak ring on the right | passed, wears_kept_ring=1 | byte-identical log |
| Control: no ring worn | passed, wears_kept_ring=0 | byte-identical log |

- **Weak ring on the left hand, good ring on the right, better ring in pack.** Original: the left-slot search skips Free Action (it was the last ring tried on the right), so the only plan containing it puts it on the right hand; the Borg takes off See Invisible and wears Free Action, keeping Feather Falling. Corrected: Free Action is also tried on the left; the Borg takes off Feather Falling and wears Free Action, keeping See Invisible.

- **Control: good ring on the left hand, weak ring on the right.** The weak ring is on the right hand, which the search fills last and checks correctly; both versions take off Feather Falling and wear Free Action. The left-slot check still runs (and skips Free Action in the original) but the best plan is the same, and the logs are identical.

- **Control: no ring worn.** Both ring slots are empty; trying Free Action on the left hand gives the same power as on the right, so the plan does not change. Both versions wear Free Action in one step, and the logs are identical.

## Conditions

Native Human Warrior, seed 86, character level 21, in town (depth 0); the starting kit is discarded. Worn: a Wooden Torch, a Dagger and Soft Leather Armour, plus rings by mode. Pack: a Ring of Free Action and five Rations of Food. All runes known; every store, including the home, is emptied. The game's first ring slot is the Borg's INVEN_RIGHT, the second its INVEN_LEFT. The fixture calls the production borg_clear_best() as the Borg does on arriving in town, sets shop_num to the home, then repeats the production borg_best_stuff() (the first step of borg_think_store()) until it declines to act. Each queued command is checked (exactly 't' or 'w' plus the letter of the item the Borg named) and carried out with the game's own inven_takeoff() / inven_wield(); the Borg then re-reads its gear with borg_find_all_items() and borg_notice(). 'swap': See Invisible in the first ring slot (right), Feather Falling in the second (left). 'mirror': the same two rings on the other hands. 'empty': no ring worn. This isolates the home equipment step; it does not walk to the home or play other store steps.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/ringslot-86
git fetch https://github.com/diegoami/neo-angband-tracker.git task/ringslot-86-evidence
git show 5a12c8ad5535219fb532c17236814169648d5812:docs/audit/c-reports/ringslot-86/reproduce.py > /tmp/reproduce-ringslot-86.py
python3 /tmp/reproduce-ringslot-86.py --full-tests --fixtures-ref 5a12c8ad5535219fb532c17236814169648d5812 --fix-ref aa3bf51c9dec12cf55ca2e7e8f3e38e07a55baf7
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-item-wear.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `aa3bf51c9dec12cf55ca2e7e8f3e38e07a55baf7` (branch `task/ringslot-86`), fixtures
  `5a12c8ad5535219fb532c17236814169648d5812` (branch `task/ringslot-86-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: mirror, empty
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_best_stuff_aux() skipped a left-ring candidate whenever it equalled the stale right-slot entry left over from the previous subtree, so the last ring tried on the right hand was never tried on the left. A level-21 Warrior at home wearing Feather Falling (left) and See Invisible (right), with Free Action in the pack, took off See Invisible instead of Feather Falling; corrected, it keeps See Invisible and wears Free Action in place of Feather Falling. Controls with the weak ring on the right hand and with no ring worn are byte-identical.

Not measured: how often the Borg reaches home with a weaker ring on the left hand than on the right and a better ring in the pack or home; the home-item path (a ring stored in the home, encoded as index + 100), which uses the same check and is not exercised here; whether a later borg_wear_stuff() or home visit would recover the lost ring (it is now in the pack; not run). The comparison is by the Borg's own borg_power() value, not by survival.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
