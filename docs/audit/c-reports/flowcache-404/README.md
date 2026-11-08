# A retreat left a dangerous grid marked "checked"

This is **flowcache-404 / issue [#404](https://github.com/diegoami/neo-angband-tracker/issues/404)**,
the Borg's flow danger cache. Origin: **C Borg**, pristine
`e473720ed88a0f0c7ca66815cef3000eb4a38bc3` (consumer case: better-borg-c D0008).

## In plain words

When the Borg plans a walk it checks how dangerous each square is and refuses
squares that are too dangerous. It remembers "I already checked this square" so
it does not repeat the work. But when the Borg is in an emergency mode
(retreating, lunal, munchkin, digging) it deliberately **skips** the danger
check, yet it still wrote "already checked" into the memory. Afterwards, back in
normal mode, the next walk found that note, trusted it, and walked straight
through a square that had never been checked. In the real game this sent a
priest back next to a grey mold five times in a row.

The correction writes "already checked" only when the check was really made.

## What this evidence shows, and what it does not

Shown, in native C, from the original game code:

- The memory entry is written without a danger check, in all four modes and in both
  places that write it (`borg_flow_spread`, `borg_flow_enqueue_grid`).
- A later normal-mode flow reuses it and routes through a square whose danger is
  16 against a limit of 1. With the correction it does not.
- The same happens through the production path: the real `borg_caution()` picks the
  retreat ("Desperate for Stairs"), then the real `borg_flow_dark()` steps back
  toward the mold. With the correction `borg_flow_dark()` finds no step.
- Safe flows, genuinely dangerous flows, blocked routes and emergency retreats behave
  identically before and after.

**Not shown:** that this priest (D0008) would have survived, or that overall
survival improves. The fix removes one wrong decision; later decisions, RNG and
other faults (source #156 low-HP guard, #152 staircase flags) are separate and are
not part of this change.

RESULTS_PLACEHOLDER
