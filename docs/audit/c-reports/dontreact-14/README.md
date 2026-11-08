# The borg stayed deaf after knockout recovery

This is **dontreact-14 / issue14**, the knockout reaction bug. When the
message "You have been knocked out." arrives, the borg sets a flag that
gates every further reaction. The flag was a file-static copy
(`borg_dont_react`) that nothing ever clears, so after the knockout
recovery the borg remained deaf to combat messages for the rest of the
session. Upstream intended the flag to be the borg's own `dont_react`
field (`borg.h`), which the production pump clears on every pass
(`borg.c`: `borg_parse(NULL); borg.dont_react = false;`). The correction
renames all five sites to that field; during the knockout the gating is
unchanged, and the borg resumes reacting on the first pump after
recovery.

## What to watch

| Version | During knockout | After recovery pump |
| --- | --- | ---: |
| Original (static flag) | gated (queue stays 1) | **still gated (queue 1)** |
| Corrected (`borg.dont_react`) | gated (queue stays 1) | **resumes (queue 2)** |

Queued counts are read back from the borg's message buffer
(`borg_msg_num`) around a baseline reaction, the knockout parse and the
recovery pump. The during-knockout gating is identical in both versions;
the correction changes only the post-recovery behaviour.

[replay.html](replay.html) is the offline page for the captured run
(regenerate it with the commands below).

## Repeat it

From a clone of `diegoami/neo-angband-tracker`:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dontreact-14
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dontreact-14-evidence
git show FETCH_HEAD:docs/audit/c-reports/dontreact-14/reproduce.py > /tmp/reproduce-dontreact-14.py
python3 /tmp/reproduce-dontreact-14.py --full-tests --fix-ref 13590ee6ea748c4bae8f6ed86df041efe810993a
```

The first fetch brings the fix commit; the second sets `FETCH_HEAD` to the
evidence revision the script is extracted from. `--fix-ref` is required: the
runner pins the corrected source by that commit. The script builds the
native C game before and after the correction, runs the knockout case for
both, checks the recorded flip, runs both full native regression gates,
and generates `replay.html`. It prints the path to open. Heavy commands
wait on the shared validation lock and print their log paths.

Prerequisites: Git, Python3, CMake, a C compiler/build toolchain, `flock`,
and native platform dependencies (X11 development headers/libraries on
this Linux configuration).

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `13590ee6ea`,
  fixtures `1006af0916d95a6ebd6c415326afd77f7d828d03`
- before: fails (`after_recovery=1`, deaf), after: passes
  (`after_recovery=2`, resumes)
- full native gates per phase: 965/965 unit checks, 5/5 frontend tests
- replay render check: Chrome headless, both cards render (1 / 2)
- corrected-source digest pins are asserted by the runner for all four
  touched files (the delta is exactly the rename)

The fixture isolates the flag lifecycle through the production
parse/flush/pump path; it does not claim full autonomous play or
encounter-frequency effects. The knockout gating itself is intended
behaviour and is unchanged.


## Re-verification with a control (2026-11-08)

Why: the original run's phase logs (configure, builds, full gates) were not
committed (the repository's `*.log` ignore rule dropped them), the receipt had
no sha256 map, and the only unchanged-situation check (the during-knockout gating) sat inside the changed case, so no separate control log could be compared byte for byte.

A separate control mode was added (`KO14_MODE=no-knockout`): the same baseline reaction, parse, pump and reaction sequence with an ordinary message ("You feel less thirsty.") where the knockout was. The knockout flag is never raised, so both versions must queue every reaction, and the runner now asserts the control passes in both versions with a byte-identical complete log.

**New fixtures pin `f89af81271aacf9e18125f61a34b58e2b12dffcc` supersedes `1006af0916d95a6ebd6c415326afd77f7d828d03`** for
every repeat command. It adds only the mode selection and the new case(s); the
main case's code path is unchanged, and the run at the new pin reproduced the
recorded main outcome exactly (the `Failed` line of the original's main log
now cites a later source line, the only difference in that log). Fix
`13590ee6ea748c4bae8f6ed86df041efe810993a`, base `e473720ed`.

| Case | Original (base) | Corrected (fix) | Logs |
| --- | --- | --- | --- |
| `main`: knockout, then recovery pump | queue after recovery **1** (still deaf), 0/1 | queue **2** (resumes), 1/1 | `before-main.log`, `after-main.log` |
| `no-knockout` (control) | queues 1, 2, 3; 1/1 | queues 1, 2, 3; 1/1 | **byte-identical** `before-no-knockout.log` / `after-no-knockout.log` |

Full native gates on both variants: 965/965 unit checks and 5/5 frontend tests
(parsed from `reverify-2026-11-08/before-alltests.log` and `after-alltests.log`,
no failed lines). Toolchain: GNU 13.3.0, cmake version 3.28.3.
Render check: `node tools/c-evidence/render-smoke.js` (Playwright headless) on
the committed `replay.html`: `RENDER-SMOKE actions=0 errors=0 OK` (no console errors, no
`undefined`/`NaN`); the page now has a section per extra case with both raw logs.

The top-level `before-*.log`, `after-*.log`, results files, `replay.html` and
`replay-data.json` are from this run. Configure, build, scenario-runner and
full-gate logs and `result.json` are in [reverify-2026-11-08/](reverify-2026-11-08/)
with their own [receipt](reverify-2026-11-08/receipt.json). The root
`receipt.json` now records this run (the earlier pin and outcomes are kept
under `superseded_run`) and a sha256 map of every file in this directory.

Repeat, with both shas pinned:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dontreact-14
git fetch https://github.com/diegoami/neo-angband-tracker.git task/dontreact-14-evidence
git show f89af81271aacf9e18125f61a34b58e2b12dffcc:docs/audit/c-reports/dontreact-14/reproduce.py > /tmp/reproduce-dontreact-14.py
python3 /tmp/reproduce-dontreact-14.py --full-tests --fixtures-ref f89af81271aacf9e18125f61a34b58e2b12dffcc --fix-ref 13590ee6ea748c4bae8f6ed86df041efe810993a
```
