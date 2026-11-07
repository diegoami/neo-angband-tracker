# Test the simpler comparison too

These tests call the actual native `borg_backup_swap()` with the same real
inventory, monster and danger setup. The assertions are unchanged. Only the
production comparison is changed between the four tested variants, all
measured in one experiment by `compare_simple.py`.

| Native case | Original comparison | Simple reversal | Guarded fix | Sentinel fix (accepted) |
| --- | --- | --- | --- | --- |
| Cold shield helps | Fail | Pass | Pass | Pass |
| Frost weapon helps | Fail | Pass | Pass | Pass |
| Only cold shield available | Pass | **Fail: selects missing weapon** | Pass | Pass |
| Only frost weapon available | Pass | **Fail: selects missing armour** | Pass | Pass |
| Equal scores | Pass | Pass | Pass | Pass |
| Neither item improves protection | Pass | Pass | Pass | Pass |
| Fire shield helps | Fail | Pass | Pass | Pass |
| Lightning weapon helps | Fail | Pass | Pass | Pass |
| Both help; shield protects against both attackers | Fail | Pass | Pass | Pass |
| **Total** | **4/9** | **7/9** | **9/9** | **9/9** |

The simple reversal is:

```c
if (b_p2 <= b_p1) {
```

It fixes the two-item choice. But the missing item's uncomputed score stays
zero, making it look safer than the real item's score49. The branch then
queues `w` followed by an invalid inventory selection in each single-item case.
The native tests catch that before attempting to apply an invalid wield.

Actual output from **both** single-item cases with the simple reversal:

```text
DECISION swapped=1 command=119 letter=0 pack_index=-1
    native-c/CBUG-0066:236: requirement 'index>=0' failed
native-c/CBUG-0066 finished: 0/1 passed
```

`command=119` is the character `w` (wear/wield). `letter=0` is not an inventory
letter, and `pack_index=-1` means the decoder could not identify a real item.
This establishes an invalid equipment command, not a claim that the full game
crashed. The `DANGER ...=-1` printed earlier in the observer output is merely
its display marker for an absent item; the production function's absent-item
score starts at **0**.

## The sentinel fix

Upstream review of the emergency-swap proposal converged on a different shape
that avoids comparing absent candidates the same way: initialize both
unavailable-candidate scores to an out-of-range sentinel and compare plainly:

```c
int32_t b_p1 = 9999L;
int32_t b_p2 = 9999L;
```

with the pick:

```c
if (b_p2 <= b_p1) {
```

and no early `!armour_swap && !weapon_swap` return. Danger is capped at 2000
(`borg_danger`), so 9999 is a safe out-of-range sentinel: an absent candidate
can never win the pick against a real one, and when both are absent the final
acceptance gate (`p > b_p`) rejects the swap. The accepted proposal head is
`b6bdf554a7`; its complete diff against the pristine base is the three-hunk
sentinel change recorded in [SOURCE-UPDATE.md](../SOURCE-UPDATE.md).

Both non-reversal shapes were rebuilt and rerun in the same experiment: the
sentinel fix passes 9/9 and every one of the nine native case logs is
**byte-identical** to the guarded fix's logs (same decisions, same damage),
so the reshape to the upstream-preferred form changes no measured behaviour.

## Repeat the comparison

From a clone of this repository:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/cbug-0066-evidence
git show FETCH_HEAD:docs/audit/c-reports/CBUG-0066/expanded/reproduce.py > /tmp/reproduce-cbug-0066.py
python3 /tmp/reproduce-cbug-0066.py --compare-simple
```

This runs the ordinary before/after comparison and then measures the three
alternative conditions in the disposable source tree: the simple reversal
(expect exactly the two single-item regressions), the guarded fix (9/9) and
the sentinel fix (9/9, byte-identical logs to the guarded run). It writes
`simple-reversal-result.json`, `guarded-result.json`,
`sentinel-shape-result.json` and the per-case native logs, then restores and
rebuilds the corrected version. Add `--full-tests` to also run the general
native regression gates for the original and corrected variants.
