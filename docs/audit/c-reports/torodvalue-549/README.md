# The Borg never zaps a Rod of Teleport Other at a dangerous monster

This is **torodvalue-549 / issue #549**, a C Borg defect
(diegoami/neo-angband-tracker#549).

When a monster next to the Borg is dangerous enough, the Borg's defensive step can teleport the monster away with Teleport Other. It accepts the Teleport Other spell, an item that activates for it, or a charged Wand of Teleport Other, but it never looks for a Rod of Teleport Other. A Borg that cannot cast the spell (for example a Warrior) and carries only the rod therefore never uses it: a Warrior down to a third of its hit points next to a hill giant keeps hitting the giant in melee, while the same Warrior with the wand aims it and the giant is sent across the room. After the correction the rod is accepted like the wand (if the Borg's device skill is good enough to zap it), and the Warrior zaps the rod at the giant; the real zap command teleports the giant away and the rod starts recharging. A wand-only Warrior and a Warrior next to a harmless soldier ant behave exactly as before.

## What to watch

Uses its Teleport Other device on the hill giant (1 = yes); the suite line is the scenario's own pass/fail check.

| Scenario | Original (base `e473720ed`) | Corrected (fix `d4071b06e`) |
| --- | --- | --- |
| Warrior with a Rod of Teleport Other next to a hill giant | failed, used_teleport_other=0 | passed, used_teleport_other=1 |
| Control: the same Warrior with a Wand of Teleport Other | passed, used_teleport_other=1 | byte-identical log |
| Control: rod carried, harmless soldier ant adjacent | passed, used_teleport_other=0 | byte-identical log |

- **Warrior with a Rod of Teleport Other next to a hill giant.** Grid danger 64 against 106 hit points. Original: borg_caution() chooses nothing (the defence never considers the rod), and the attack step queues melee on the giant (keys +6); the rod is not used. Corrected: borg_caution() chooses the Teleport Other defence and queues *p65zb5 (target the giant, zap the rod in slot b, use the target). The real zap command fails three times (the game's device failure chance for this Warrior is 24.2%) and succeeds on the fourth attempt: the giant is moved from (11,12) to (38,22), 33 grids away, and the rod starts recharging.

- **Control: the same Warrior with a Wand of Teleport Other.** The same scene with the wand instead of the rod. Both versions choose the defence, aim the wand (keys *p65ab5), and the real aim command teleports the giant to (38,22) at the first attempt (10 charges left). This case runs through both changed lines (the wand is accepted before the rod check and aimed before the zap). The logs are byte-identical.

- **Control: rod carried, harmless soldier ant adjacent.** The same Warrior with the rod, next to a soldier ant (grid danger 2 against 106 hit points). No danger to speak of, so borg_caution() chooses nothing and neither version zaps the rod; both queue melee (keys +6). The logs are byte-identical. This control shows the rod is not zapped at harmless monsters; it does not reach the changed lines.

## Conditions

Native Human Warrior, seed 549, character level 25 (device skill 36), depth 25, in a lit open 22x38 arena; the starting kit's carried items are removed and the Warrior carries only a lit Wooden Torch (worn), 5 Rations of Food and the one device of the case (a charged Rod of Teleport Other, or in the wand control a Wand of Teleport Other with 11 charges); no Teleport Other spell, no escapes, no healing potions. Hit points are set to 33% (106/322) and borg.avoidance to the current hit points, as the Borg does. A stock hill giant (or, in the weak control, a stock soldier ant) is placed adjacent to the east, awake and aware, and registered through the production first-sighting path observe_kill_diff(). The Borg's map is filled from the real terrain. The Borg then runs its production 'try not to die' step borg_caution() (which calls borg_defend()); if that chooses nothing, the next fight step of borg_think_dungeon(), borg_attack(false), is run and its keys recorded. Queued keys are read back; when they are a zap ('z') or aim ('a') of the case's device, the fixture carries it out with the real CMD_USE_ROD / CMD_USE_WAND command at the monster's grid (the target the Borg's '*' keys select), one attempt per command with the game's auto-repeat disabled, up to five attempts, the monster not acting between attempts; game messages are printed as MSG lines. The melee keys chosen by the original Borg in the rod case are recorded, not carried out. This isolates one decision turn; it does not play out the fight.

[replay.html](replay.html) is the offline page for the captured run (download
it and open it locally): choose a scenario, press **Show decision** for the
original and corrected results side by side, and **Show raw logs** for the
complete native output.

## Repeat it

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/torodvalue-549
git fetch https://github.com/diegoami/neo-angband-tracker.git task/torodvalue-549-evidence
git show f3323c2f010ef803424e0a3548728e03b576e94b:docs/audit/c-reports/torodvalue-549/reproduce.py > /tmp/reproduce-torodvalue-549.py
python3 /tmp/reproduce-torodvalue-549.py --full-tests --fixtures-ref f3323c2f010ef803424e0a3548728e03b576e94b --fix-ref d4071b06e2d298a17e1a55559daceccf9663f9f3
```

The fetches bring the fix commit and the evidence branch; both the fixtures
commit and the fix commit are pinned by sha. The runner asserts the
correction is exactly the expected change in `src/borg/borg-fight-defend.c`
and nothing else, builds the original and corrected native C game in a
disposable worktree, runs every scenario, checks each expected result and
that every control log is byte-identical, runs both full native gates and
writes `replay.html`. Heavy steps wait on `/tmp/angband-validation.lock`.

## Recorded receipt (this run)

- base `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`, fix `d4071b06e2d298a17e1a55559daceccf9663f9f3` (branch `task/torodvalue-549`), fixtures
  `f3323c2f010ef803424e0a3548728e03b576e94b` (branch `task/torodvalue-549-evidence`). The native run executed at
  the fixtures pin; later commits on the evidence branch only add these
  captured artifacts (README, receipt, logs, replay).
- controls byte-identical: wand, weak
- full native gates: before 965/965 unit checks and 5/5 frontend tests;
  after 965/965 and 5/5
- replay: generated script passes `node --check`; Playwright Chromium headless
  render check of the committed page, no console errors

## What this proves, and what it does not

borg_defend_aux_tele_away() (borg-fight-defend.c:1155-1159 and 1272-1273 at e473720ed) accepted and used the Teleport Other spell, the tele_other activation and a charged Wand of Teleport Other, but not the Rod of Teleport Other (sv_rod_teleport_other is looked up in borg-item-val.c:417 and used only by the shopping code). A Borg without the spell that carried only the rod never chose this defence: a wounded Warrior next to a hill giant queued melee instead. The correction accepts the rod through borg_equips_rod() (charged, and the Borg's own device-skill check) and zaps it with borg_zap_rod() after the spell, activation and wand; the '5' target key that follows is unchanged. The wand control (both changed lines run) and a harmless-monster control are byte-identical.

Measured: one decision turn of borg_caution()/borg_defend() and the real zap or aim command, for a level-25 Warrior next to a hill giant at 33% hit points. Not measured: whole fights or survival, how often a Borg carries this rod (the original Borg never buys it: the rod is not counted in BI_ATPORTOTHER at borg-trait.c:2454/2719, so borg_power() does not rise for it; that valuation and buying are separate and unchanged here, as is the Black Market class check of #17), the Morgoth-level Teleport Other defence (spell only, unchanged), and the other rod effects. The real zap failed three times before succeeding (24.2% failure per attempt for this Warrior); in a real game each failed attempt is a game turn in which the monster acts, and the fixture does not let the monster act between attempts. borg_equips_rod() allows the rod only while the Borg's device estimate is under 50% failure; for low-skill characters it is not offered, as for other rods. No memory-safety aspect, so no sanitizer variant.

## Files

`scenario.c`, `run-cases.py`, `make_replay.py`, `reproduce.py`, `borg.txt` (+
`profile/borg.txt`) are the generators. `before-*.log` and `after-*.log` (one per
scenario), `before-results.json`, `after-results.json`, `result.json`,
`run-configure.log`, `run-{before,after}-build.log`,
`run-{before,after}-scenarios.log`, `run-{before,after}-alltests.log`,
`replay.html`, `replay-data.json` and `receipt.json` (sha256 of every other
file) are the captured run.
