# One recently stored home ware stopped the Borg clearing later junk from its home

This is **homeskip-29 / issue #29**, a C Borg defect
(diegoami/neo-angband-tracker#29).

In town the Borg's store step 5 takes items that add nothing to its home back out, so it can sell them. It skips any home item its sale history says it has just stored at home, so it does not take that straight back. The flag that marks an item to skip was cleared only once, before the loop over the home, so after the first skipped item every later item in the home was skipped too. A level-11 Warrior whose home held a Potion of Cure Critical Wounds it had just stored, followed by a Scroll of Light that adds nothing to the home, chose no store action at all and left the scroll at home. The correction clears the flag for each item; the Borg then sets out to take the Scroll of Light from its home to sell and the real retrieve command moves it into the pack.

## What to watch

Sets out to take the useless Scroll of Light from the home (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `3f35efd71`) |
| --- | --- | --- |
| Just-stored potion before a useless scroll | failed, grabs_scroll_of_light=0 | passed, grabs_scroll_of_light=1 |
| Control: just-stored shovel after the scroll | passed, grabs_scroll_of_light=1 | byte-identical log |
| Control: no recent home sale | passed, grabs_scroll_of_light=1 | byte-identical log |

- **Just-stored potion before a useless scroll.** Original: step 4 and step 5 both skip the potion because it was just stored; step 5 leaves its skip flag set, so it also skips the Scroll of Light, no step sets a goal and the scroll stays at home. Corrected: the flag is cleared for the scroll, removing it leaves the home power unchanged (1514), so step 5 picks home slot 1 and the real retrieve moves the Scroll of Light into the pack.

- **Control: just-stored shovel after the scroll.** The just-stored item comes after the scroll, so the scroll is evaluated before the flag is ever set. Both versions take the Scroll of Light (home slot 0) and skip the shovel; the logs are identical.

- **Control: no recent home sale.** With an empty sale history the flag is never set; neither item adds to the home, and step 5 takes the later one, the Scroll of Light (home slot 1), in both versions. The logs are identical.

## Conditions

Native Human Warrior, seed 29, character level 11, maximum depth 10, 3000 gold, standing on the home entrance in town (depth 0) with the starting kit after its Word of Recall scroll and Berserk Strength potion are removed (as after recalling to town and drinking the potion), all runes known. Every store is emptied and the home holds two items per mode. 'stored-first': Potion of Cure Critical Wounds, then Scroll of Light. 'stored-last': Scroll of Light, then Shovel. 'no-history': Potion of Infravision, then Scroll of Light. Fixture intervention: the Borg's sale history is given one entry, the fields borg_think_shop_sell() writes for a home sale (tval, sval, store = home), naming the potion ('stored-first') or the shovel ('stored-last'); 'no-history' leaves the history at its start-up value. A count of one is what the history holds after one recorded sale with an entry-counting counter, and after two sales with the original counter (issue #20), so this case does not depend on #20. The Borg's items and stores are loaded with the production borg_find_all_items(); the production borg_choose_shop() picks the next store action (steps 1 to 4 find nothing to do here and step 5, borg_think_home_grab_useless, decides); when it chooses a home ware, one production borg_think_store() turn queues the keys, the ware letter is checked and the real do_cmd_retrieve() runs.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/homeskip-29
git fetch https://github.com/diegoami/neo-angband-tracker.git task/homeskip-29-evidence
git show 0dc78372b4423896d2da513f2d7405a972b109c3:docs/audit/c-reports/homeskip-29/reproduce.py > /tmp/reproduce-homeskip-29.py
python3 /tmp/reproduce-homeskip-29.py --full-tests --fixtures-ref 0dc78372b4423896d2da513f2d7405a972b109c3 --fix-ref 3f35efd71787a00293d5f14e65c94abbdb760942
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-store-buy.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `3f35efd71787a00293d5f14e65c94abbdb760942` (branch `task/homeskip-29`), fixtures
  `0dc78372b4423896d2da513f2d7405a972b109c3` (branch `task/homeskip-29-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: stored-last, no-history
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_think_home_grab_useless() cleared its skip_it flag only at declaration, so after the first home ware matching the recent-sale history every later ware was skipped too. With a just-stored Potion of Cure Critical Wounds before a Scroll of Light that adds nothing to the home, the original Borg chose no store action and left the scroll at home; clearing the flag per ware makes it choose the scroll and the real retrieve moves it into the pack. Controls with the just-stored ware after the scroll and with no history are byte-identical.

Measured: one borg_choose_shop() decision and the one store turn that carries it out (keys and the real retrieve). Not measured: walking to a shop, the sale of the scroll and its price, or later turns; a Scroll of Light sells for little, so the measured cost is a home slot kept holding junk and a small unrealised sale, not a dangerous decision. The sale history entry is a disclosed fixture write (see conditions), not the result of a played sale. Not measured: how often the history names a ware that sorts before junk in real games; the history ring holds at most nine entries and is reset on each new level.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
