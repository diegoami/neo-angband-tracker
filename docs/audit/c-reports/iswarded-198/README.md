# The Borg crashed reading a rune message after Word of Recall

This is **iswarded-198 / issue198**, a C Borg defect
(diegoami/neo-angband-tracker#198).

When a monster breaks one of the Borg's Runes of Protection, or hits the Borg while it is tracking runes, the Borg forgets its rune list and checks whether a rune is still under its own position. If Word of Recall fires in the same game turn, the game shows that message with -more- just before it leaves the level. The Borg stores the line and parses it at its next keypress, which is the first prompt on the new level, before its map update has moved its own position there. Its position is still the old level's grid, (113,48) here, which lies outside the 66x22 town, and the rune check reads outside the town's map. In a normal debug build the game aborts on an assertion; in a release-style build (asserts off) AddressSanitizer reports a heap-buffer-overflow read. The correction skips the rune check when the position is not on the current level; the rune list is still cleared, and the Borg goes on to make its first town move.

## What to watch

First new-level keypress completed (1); n/a means the run aborted before it finished; the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `62242ac26`) |
| --- | --- | --- |
| Sea of Runes, rune broken, recall to town | failed, survived=n/a | passed, survived=1 |
| Hit while tracking a rune, recall to town | failed, survived=n/a | passed, survived=1 |
| Control: rune broken, old grid inside the town | passed, survived=1 | byte-identical log |
| Control: hit while tracking a rune, old grid inside the town | passed, survived=1 | byte-identical log |
| Control: hit with no rune tracked | passed, survived=1 | byte-identical log |

- **Sea of Runes, rune broken, recall to town.** Line stored at -more-: 'The rune of protection is broken! You feel yourself yanked upwards!'. Original: at the first town prompt the parse checks the old grid (113,48) in the 66x22 town: assertion failure in square_istrap (debug), heap-buffer-overflow read (AddressSanitizer). Corrected: the rune list is cleared (25 to 0), the check is skipped, the map update moves the Borg to its town grid and it chooses a move (key 50).

- **Hit while tracking a rune, recall to town.** Line stored at -more-: 'The skeleton human hits you. You feel yourself yanked upwards!'. Original: the hit-by branch checks the old grid (113,48) in the town and aborts the same way. Corrected: the rune list is cleared (1 to 0) and the Borg chooses a town move (key 51).

- **Control: rune broken, old grid inside the town.** Same sequence from old grid (30,10), which exists in the town: the check reads a real town grid (no rune) in both versions; byte-identical logs.

- **Control: hit while tracking a rune, old grid inside the town.** Same hit sequence from old grid (30,10): the hit-by branch runs the guarded check on a real town grid in both versions; byte-identical logs.

- **Control: hit with no rune tracked.** Same hit sequence from (113,48) with no rune tracked: the hit-by branch skips the rune check in both versions; byte-identical logs. A hit alone never reaches the read.

## Conditions

Native Human Warrior, seed 198, character level 30, depth 40, on a full-size open level (198x66), standard 80x24 screen. Fixture interventions: the arena and the runes the Borg laid earlier (placed with the Borg's own rune records, as its rune-reading code writes them); an awake skeleton human (level 12, one blow, no spells, never afraid); Word of Recall set to fire on the next world tick; Escape answers at the old-level prompts between monster rounds; the EVENT_MESSAGE_FLUSH signal and the panel recentring that the game's static leave-level and new-level display code performs. Real code: monster turns (process_monsters), the recall (process_world), the game's own message line and -more- prompt (display_message and message_flush from ui-input.c), town generation (prepare_next_level) and the Borg's keypress hook, including its think and map update at the first town prompt. rune-*: a 5x5 Sea of Runes with the Borg on the centre rune; the monster breaks an outer rune. hit-*: one rune west of the Borg; the monster hits the Borg. -town: old position (113,48), outside the town; -inside: old position (30,10), also a town grid. The AddressSanitizer variant builds with -DNDEBUG (as a release build does), so the assertion that stops a debug build does not mask the read.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output of both the debug and the AddressSanitizer build.

## AddressSanitizer variant

The debug build stops at `square_istrap()`'s bounds assertion before the
bad read happens. Builds with assertions off (for example
`./configure --enable-release`, which adds `-DNDEBUG`) go on to read
`cave->squares[48]` from a town that has 22 rows. The runner therefore
also builds the same source with
`-fsanitize=address -fno-omit-frame-pointer -g -DNDEBUG` and runs every
scenario there (`ASAN_OPTIONS=detect_leaks=0:intercept_strcmp=0`; the
strcmp interceptor is off because the pristine base trips an unrelated
report in the Borg's own string sort, `borg-util.c`).

| Scenario | Original, AddressSanitizer build | Corrected, AddressSanitizer build |
| --- | --- | --- |
| Sea of Runes, rune broken, recall to town | failed: heap-buffer-overflow READ, square_istrap <- borg_parse_aux borg-messages.c:1188 | passed, no sanitizer report |
| Hit while tracking a rune, recall to town | failed: heap-buffer-overflow READ, square_istrap <- borg_parse_aux borg-messages.c:625 | passed, no sanitizer report |
| Control: rune broken, old grid inside the town | passed, no sanitizer report | passed, no sanitizer report (byte-identical log) |
| Control: hit while tracking a rune, old grid inside the town | passed, no sanitizer report | passed, no sanitizer report (byte-identical log) |
| Control: hit with no rune tracked | passed, no sanitizer report | passed, no sanitizer report (byte-identical log) |

The runner asserts the original out-of-level cases stop with
`heap-buffer-overflow` / `READ of size 8` in `square_istrap`, called through
`square_iswarded` from `borg_parse_aux` at the expected line
(`borg-messages.c:1188` for the broken rune, `:625` for the hit), and that
every corrected case and every control completes without a sanitizer report.
The control logs are byte-identical in this build too. The report
(original, Sea of Runes case):

```text
==22478==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x50d000029230 at pc 0x556648f8623f bp 0x7fff6372ca80 sp 0x7fff6372ca70
READ of size 8 at 0x50d000029230 thread T0
    #0 0x556648f8623e in square_istrap /tmp/angband-iswarded-198-w4l6xu9o/source/src/cave-square.c:491
    #1 0x5566491d3e58 in square_trap_specific /tmp/angband-iswarded-198-w4l6xu9o/source/src/trap.c:73
    #2 0x556648f86ebe in square_iswarded /tmp/angband-iswarded-198-w4l6xu9o/source/src/cave-square.c:753
    #3 0x55664940bd42 in borg_parse_aux /tmp/angband-iswarded-198-w4l6xu9o/source/src/borg/borg-messages.c:1188
    #4 0x55664940ce8f in borg_parse /tmp/angband-iswarded-198-w4l6xu9o/source/src/borg/borg-messages.c:1342
    #5 0x5566494a0550 in generate_keypress /tmp/angband-iswarded-198-w4l6xu9o/source/src/borg/borg.c:311
    #6 0x55664949f6ef in borg_entry_point /tmp/angband-iswarded-198-w4l6xu9o/source/src/borg/borg.c:78
    #7 0x556648f7d667 in observed_hook /tmp/angband-iswarded-198-w4l6xu9o/evidence/scenario.c:90
    #8 0x556648f801c1 in scenario /tmp/angband-iswarded-198-w4l6xu9o/evidence/scenario.c:286
    #9 0x5566494a19dc in main /tmp/angband-iswarded-198-w4l6xu9o/source/src/tests/unit-test.c:53
    #10 0x7ff92dc2a1c9 in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #11 0x7ff92dc2a28a in __libc_start_main_impl ../csu/libc-start.c:360
    #12 0x556648f7ce44 in _start (/tmp/angband-iswarded-198-w4l6xu9o/build-asan/game/ISW198Observer+0x120e44) (BuildId: 80796b2855e69ec65bee2b8a2cc0ce55c1ff811d)

0x50d000029230 is located 28 bytes after 132-byte region [0x50d000029190,0x50d000029214)
allocated by thread T0 here:
    #0 0x7ff92e0fd9c7 in malloc ../../../../src/libsanitizer/asan/asan_malloc_linux.cpp:69
    #1 0x5566492faaaf in mem_alloc /tmp/angband-iswarded-198-w4l6xu9o/source/src/z-virt.c:38
    #2 0x5566492faaeb in mem_zalloc /tmp/angband-iswarded-198-w4l6xu9o/source/src/z-virt.c:46
    #3 0x556648f91057 in cave_new /tmp/angband-iswarded-198-w4l6xu9o/source/src/cave.c:361
    #4 0x55664904d49a in cave_generate /tmp/angband-iswarded-198-w4l6xu9o/source/src/generate.c:1249
    #5 0x55664904fd0d in prepare_next_level /tmp/angband-iswarded-198-w4l6xu9o/source/src/generate.c:1617
```

How the read goes wrong (checked under gdb in a local build with the same flags and source): `square()`
first reads the row pointer `squares[48]` from the town's 22-entry row
array. That address lies past the array but inside another live heap block,
so AddressSanitizer cannot flag it; the value read there is used as a row,
and the grid read through it (`[113].info`) lands 28 bytes past a 132-byte
block, which is what the report names. Without the sanitizer the result
depends on heap layout: a wrong answer or a crash. The full reports are in
`asan-before-rune-town.log` and `asan-before-hit-town.log`.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/iswarded-198
git fetch https://github.com/diegoami/neo-angband-tracker.git task/iswarded-198-evidence
git show 563a26167a52131d3b8ab387af4fbfd4b5b7226b:docs/audit/c-reports/iswarded-198/reproduce.py > /tmp/reproduce-iswarded-198.py
python3 /tmp/reproduce-iswarded-198.py --full-tests --fixtures-ref 563a26167a52131d3b8ab387af4fbfd4b5b7226b --fix-ref 62242ac26bab4f128793d767f0f502bf4ca4d820
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-messages.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, repeats every scenario in a second
AddressSanitizer build (`-DNDEBUG`) and checks the expected report and its
absence, runs both full native gates and writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `62242ac26bab4f128793d767f0f502bf4ca4d820` (branch `task/iswarded-198`), fixtures
  `563a26167a52131d3b8ab387af4fbfd4b5b7226b` (branch `task/iswarded-198-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: rune-inside, hit-inside, hit-noglyph (in both
  the debug and the AddressSanitizer build)
- AddressSanitizer build: original rune-town and hit-town report a
  heap-buffer-overflow READ of size 8 in `square_istrap` from
  `borg_parse_aux` (`borg-messages.c:1188` and `:625`); every corrected case
  and every control runs clean
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

The Borg parses a message stored at the last -more- prompt of the old level at the first keypress of the new level, before borg_update() moves borg.c. Both rune checks in borg_parse_aux() (the hit-by branch with runes tracked, and 'The rune of protection is broken!') then call square_iswarded(cave, borg.c) with the old level's position. When that position is outside the new level, a debug build aborts on square_istrap()'s bounds assertion and a release-style build reads outside the level's row array (AddressSanitizer: heap-buffer-overflow READ in square_istrap). Guarding both checks with square_in_bounds() removes the read; the rune list is still cleared, and inside-level and no-rune controls are byte-identical.

Reachability is narrow. The message must be in the last -more- line before the level change, so the level change must come from the game turn itself (Word of Recall or Deep Descent firing, or a monster's teleport-level), not from a Borg command; on a freshly generated level monsters never act before the player's first command. The rune-broken check needs a rune broken in that same turn (Sea of Runes or a defensive rune); the hit-by check needs runes tracked and a hit in that last line. The old position must also lie outside the new level: the town (66x22) or a smaller cavern, labyrinth or moria level. The fixture forces the coincidence (recall at one tick); it does not measure how often a full game reaches it, and no live game crash has been observed.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`asan-{before,after}-*.log`, `asan-{before,after}-results.json`,
`run-configure-asan.log`, `run-asan-{before,after}-build.log`,
`run-asan-{before,after}-scenarios.log`, `replay.html`, `replay-data.json`
and `receipt.json` (sha256 of every other file) are the captured run.
