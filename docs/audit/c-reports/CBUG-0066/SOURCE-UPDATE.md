# Source updates

Current proposal head: `b6bdf554a7ca401018912ad951846efdf2f9c56f` (branch
`task/c-borg-safer-swap`, draft PR226). Reshaped to the form upstream review
preferred for the same change (angband/angband#6809): initialize both
unavailable-candidate scores to an out-of-range sentinel, drop the early
"no swap" return, and compare plainly. Its complete C diff against the
pristine base:

```diff
-    int32_t b_p1 = 0L;
-    int32_t b_p2 = 0L;
+    int32_t b_p1 = 9999L;
+    int32_t b_p2 = 9999L;
```

```diff
-    /* make sure we have an appropriate swap */
-    if (!armour_swap && !weapon_swap)
-        return false;
```

```diff
-    if (b_p1 <= b_p2) {
+    if (b_p2 <= b_p1) {
```

`borg_danger` caps at 2000, so 9999 is a safe out-of-range sentinel: an absent
candidate can never win the pick, and with both absent the acceptance gate
rejects. Equivalence with the previously validated guarded head (`ebcd255ba`,
digest `96dd28c7…`) is verified natively in the expanded geometry: all nine
case logs byte-identical, all nine suites pass under both shapes — see
[the simple-comparison record](expanded/SIMPLE-COMPARISON.md). Pinned digest
of the corrected source at this head:
`3a9219e11f4de32a070652a326ffc67def6100d094d7eff78939879922c2f25b`.

[Preferred cumulative patch](../../../../patches/upstream-c/CBUG-0066-safer-swap-clean.patch).
The original verification/log receipt at66e10121 is preserved unchanged.
Between that tested revision and this head the selection logic was reshaped;
the measured case results and damage numbers are unchanged because the two
shapes are natively equivalent (verified above). Hosted checks belong to their
exact head; the earlier passing runs do not establish the new head. The full
native gate was repeated for this head; see the expanded README receipt.

Verification:

```sh
git diff ebcd255ba9064fc44a35d693f42f9df3dc00bd88 b6bdf554a7ca401018912ad951846efdf2f9c56f -- src/borg/borg-item-wear.c
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 b6bdf554a7ca401018912ad951846efdf2f9c56f -- src/borg/borg-item-wear.c
```

The first command shows exactly the reshape above. The second shows the whole
proposal against the pristine base: the three-hunk sentinel change. Internal
catalogue labels belong in tracking records, not in the submitted source
comment.
