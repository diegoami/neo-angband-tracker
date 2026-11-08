# encumb-186 triage fixture (not a fix proposal)

Native observer used to triage issue #186. It measures the production
`borg_think_shop_buy_useful()` decision for an encumbered Human Warrior
(level 20, Full Plate Armour worn, Dagger wielded, pack loaded with flasks
of oil and rations to a chosen weight) when the only ware in any store is a
black-market Scroll of Enchant Weapon To-Hit (or a Potion of Strength).

Modes (`EN186_MODE`): `edge` (the scroll's 0.5 lb would cost a point of
speed), `mid` (inside the second encumbrance step), `str` (same weight,
Potion of Strength), `strlight` / `light` (unencumbered).

Runs: `pristine.log` on `e473720ed`; `scan-from-last-pack-slot.log` with the
reported change (`PACK_SLOTS - 1` at borg-power.c:1838). Both with
`diagnostic-prints.diff` applied (two printf lines, no logic change).
Built as an extra unit-test executable as in `tools/c-evidence/devbuild.sh`;
run with `EN186_PROFILE=<dir holding borg.txt> EN186_MODE=<mode>` from
`build/game`.
