# The clock-overflow abort fired before the graceful reset

This is **clockreset-246 / issue246**, an unreachable-handler defect. The
graceful clock reset in `borg_think()` fires at `BORG_TIME_MAX - 100`, but
`borg_think_dungeon()` aborts the Borg with `borg_oops("clock overflow")` at
`BORG_TIME_MAX - 3000` — the abort always won about 2,900 turns earlier, so
the graceful reset was unreachable in dungeon play: an aborted Borg never
thinks again, the clock never advances, and the only handler the Borg ever
exercised was the panic.

Per the owner's decision (reset first, abort as failsafe), the correction
resets the clock in place at the threshold — the same `borg.time.now = 1`
assignment the `borg_think()` overflow check performs — and keeps playing.
The `borg_think()` overflow check remains as a failsafe in case the reset is
somehow missed.

## What to watch

| Version | Clock after the threshold pass | Borg still active |
| --- | --- | --- |
| Original | frozen at the threshold (2147480647) | **no — aborted** ("Aborting (clock overflow).") |
| Corrected | **reset to 1** | **yes** — keeps playing |

Fixture: Native Human Warrior, seed 246, depth 9, a glowing arena with no
monsters. The borg clock is set to the exact abort threshold and one
production `borg_think_dungeon()` pass runs; the clock value, the active
flag and the borg notes are read back directly.

[replay.html](replay.html) is the offline page for the captured run.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/clockreset-246
git fetch https://github.com/diegoami/neo-angband-tracker.git task/clockreset-246-evidence
git show FETCH_HEAD:docs/audit/c-reports/clockreset-246/reproduce.py > /tmp/reproduce-clockreset-246.py
python3 /tmp/reproduce-clockreset-246.py --full-tests --fix-ref 7cc0ebbd2016afeaaf57c5e976251b67f7c0e147
```

The first fetch brings the fix commit; the second sets `FETCH_HEAD` to the
evidence revision. `--fix-ref` is required: the runner pins the corrected
source by that commit and asserts the correction is exactly the graceful
reset replacing the panic in the single touched file.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `7cc0ebbd` (branch
  `task/clockreset-246`), fixtures `85ff2254` (branch
  `task/clockreset-246-evidence`)
- before: suite 0/1 — clock frozen at 2147480647, borg aborted
  (`active=0`), note "Aborting (clock overflow)."
- after: suite 1/1 — clock reset to 1, borg still active, overflow warning
  recorded
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Chrome headless

## Scope and limits

The fixture isolates the overflow handling at the threshold for one think
pass. It does not measure autonomous play across the following thousands of
turns. The reset semantics (`borg.time.now = 1`) are identical to the
original `borg_think()` overflow handler, moved to the point where they can
actually run.
