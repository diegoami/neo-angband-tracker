# 3. Magic and Spells

Everything about spellcasting in Angband 4.2.x (upstream master `e473720ed`): the four realms, how
spells are learned, the exact failure and mana formulas, every book and where it comes from, and
every spell of every casting class with its resolved numbers, alternatives and advice. It ends with
a cross-class "I need X" lookup and a casting decision procedure for an automated player.

**Contents**

1. [Magic in one screen](#1-magic-in-one-screen)
2. [Realms, casting stats and class magic parameters](#2-realms-casting-stats-and-class-magic-parameters)
3. [Learning spells](#3-learning-spells)
4. [Casting: requirements, energy, over-exertion](#4-casting-requirements-energy-over-exertion)
5. [Spell failure](#5-spell-failure)
6. [Mana: maximum, encumbrance, regeneration](#6-mana-maximum-encumbrance-regeneration)
7. [How spell effects work (geometry, dice, monster saves)](#7-how-spell-effects-work)
8. [Books: town vs dungeon, prices, depths, who uses which](#8-books)
9. [Mage](#9-mage)
10. [Druid](#10-druid)
11. [Priest](#11-priest)
12. [Necromancer](#12-necromancer)
13. [Paladin](#13-paladin)
14. [Rogue](#14-rogue)
15. [Ranger](#15-ranger)
16. [Blackguard](#16-blackguard)
17. [Shapechanges](#17-shapechanges)
18. [Cross-class lookup: "I need X"](#18-cross-class-lookup-i-need-x)
19. [Casting decision procedure for a borg](#19-casting-decision-procedure-for-a-borg)
20. [Things to try (untested)](#things-to-try-untested)

Conventions: **L** = character level, **SP** = spell points (mana). "Fail" in spell tables is the
*base* fail from `lib/gamedata/class.txt`; your real chance is computed by the formula in
[section 5](#5-spell-failure). "XP" is the experience for the *first* successful cast
(= `exp` field × spell level, see [3.4](#34-first-cast-experience)). Dice expressions such as
`$Dd4` with `expr:D:PLAYER_LEVEL:- 1 / 5 + 3` are evaluated **strictly left to right with integer
division** (`src/z-expression.c:expression_evaluate()`), so `- 1 / 5 + 3` means `(L-1)/5 + 3`.
Full raw spell tables are generated in [Appendix A7](appendices/A7-class-spells.md); this chapter
interprets them. Warriors have no spells (`NO_MANA`).

The official manual covers the commands (`b`rowse, `G`ain a spell, `m` cast) in
[command](https://angband.readthedocs.io/en/latest/command.html) (vendored:
[user-manual/command.rst](user-manual/command.rst)) and the class descriptions in
[birth](https://angband.readthedocs.io/en/latest/birth.html) ([user-manual/birth.rst](user-manual/birth.rst)).
It says only that failure "starts out fairly large but decreases as you gain levels", that casting
without enough mana risks paralysis and CON damage, and that you need light (except necromancers)
and must not be blind or confused. Everything below goes further, with the numbers.

---

## 1. Magic in one screen

FACT (all from `src/player-spell.c`, `src/player-calcs.c`, `src/cmd-obj.c`):

- Each casting class uses one **realm** (arcane/INT, divine/WIS, nature/WIS, shadow/INT). The realm
  stat drives number of spells, max mana and failure.
- **Spells allowed** = `(adj_mag_study[stat] × (L − first_spell_level + 1) + 50) / 100`. You pick the
  spell (`CHOOSE_SPELLS` classes) or get a random one from the chosen book (Priest, Paladin).
- **Max SP** = `1 + adj_mag_mana[stat] × (L − first_spell_level + 1) / 100`, minus 1 SP per pound of
  armour above the class weight limit. **Gloves do not matter in this version.**
- **Fail** = base − 3×(L − spell level) − adj_mag_stat[stat], (+5 per missing SP, +25 on a lit square
  for necromancers, +20 if afraid), floored at the stat minimum (5% minimum for classes without
  `ZERO_FAIL`), **capped at 50%**, then +15/+25 if stunned, then amnesia `50 + chance/2`, final cap 95%.
- Casting costs one full turn (100 energy; 75 under *Mana Channel*). You cannot cast while blind,
  confused, or (non-necromancers) without light on your square, and you cannot cast while
  shapechanged without first reverting.
- **First successful cast** of each spell gives `exp × spell_level` experience — cast every new spell
  once as soon as it is safe.
- Mana regenerates per 10 game turns (not per player turn): ~0.30% of max SP each normal-speed turn,
  doubled by resting and again by *Regeneration*.

---

## 2. Realms, casting stats and class magic parameters

FACT: `lib/gamedata/realm.txt` defines four realms:

| Realm | Stat | Verb | Spell noun | Book noun |
|---|---|---|---|---|
| arcane | INT | cast | spell | magic book |
| divine | WIS | recite | prayer | prayer book |
| nature | WIS | chant | verse | nature book |
| shadow | INT | perform | ritual | shadow book |

FACT: class magic parameters (`magic:first:weight:books` and `player-flags` in `lib/gamedata/class.txt`):

| Class | Realm (stat) | First spell level | Armour weight limit | Books | Spells | Relevant flags |
|---|---|---|---|---|---|---|
| Mage | arcane (INT) | 1 | 30 lb | 5 | 30 | `ZERO_FAIL`, `BEAM`, `CHOOSE_SPELLS` |
| Druid | nature (WIS) | 1 | 35 lb | 5 | 27 | `ZERO_FAIL`, `CHOOSE_SPELLS`, `CHARM` |
| Priest | divine (WIS) | 1 | 35 lb | 5 | 28 | `ZERO_FAIL`, `BLESS_WEAPON` (random learning) |
| Necromancer | shadow (INT) | 1 | 30 lb | 5 | 26 | `ZERO_FAIL`, `CHOOSE_SPELLS`, `UNLIGHT`, `EVIL` |
| Paladin | divine (WIS) | 1 | 40 lb | 3 | 16 | `BLESS_WEAPON`, `SHIELD_BASH` (random learning) |
| Rogue | arcane (INT) | 5 | 35 lb | 2 | 10 | `CHOOSE_SPELLS`, `STEAL` |
| Ranger | nature (WIS) | 3 | 40 lb | 2 | 11 | `CHOOSE_SPELLS`, `FAST_SHOT` |
| Blackguard | shadow (INT) | 1 | 60 lb | 3 | 15 | `CHOOSE_SPELLS`, `COMBAT_REGEN`, `SHIELD_BASH`; `IMPAIR_HP` |
| Warrior | — | — | — | 0 | 0 | `NO_MANA` |

What the magic flags do (FACT, with source):

- `ZERO_FAIL` — your minimum fail can drop below 5% down to the stat table minimum (0% at 18/200+).
  Without it the floor is 5% (`spell_chance()`). Paladins, Rogues, Rangers and Blackguards always
  keep at least 5% failure.
- `BEAM` — beam chance for `BOLT_OR_BEAM` spells is L% instead of L/2% (`beam_chance()`). Only Mage
  spells use `BOLT_OR_BEAM`, so in practice this is a Mage feature.
- `CHOOSE_SPELLS` — `G` lets you pick the spell; without it `G` asks for a book and learns a random
  eligible spell from it (`do_cmd_study_book()`).
- `CHARM` (Druid) — conf/slow/hold/sleep/polymorph *durations* against `ANIMAL` monsters are ×1.5
  (`src/project-mon.c`).
- `UNLIGHT` (Necromancer) — +25% fail when *your* square is lit; see in the dark without light;
  resist dark in the dungeon; can cast without light ([section 12](#12-necromancer)).
- `EVIL` (Necromancer) — resist nether, vulnerable to holy orb (`calc_bonuses()`).
- `COMBAT_REGEN` (Blackguard) — mana comes from fighting and drains away at rest
  ([section 16](#16-blackguard)).

The same physical book can carry **different spells for different classes**: e.g. Mage, Rogue
both read [First Spells] but see different lists; Necromancer and Blackguard share the shadow books;
Priest and Paladin share prayer books; Druid and Ranger share nature books. A book is only usable by
a class if `class.txt` lists it for that class.

---

## 3. Learning spells

### 3.1 How many spells you may know

FACT (`src/player-calcs.c:calc_spells()`):

```
levels       = L - first_spell_level + 1          (0 if negative)
num_allowed  = (adj_mag_study[stat_index] * levels + 50) / 100
new_spells   = num_allowed - spells_known          (capped at the number of spells
                                                     of level <= L that you don't know)
```

For classes with two realms the stat is averaged (none exist in 4.2: every class has one realm).
`adj_mag_study` (spells per 100 levels) by casting-stat value:

| Stat | 8 | 10 | 12 | 14 | 16 | 17 | 18/00 | 18/20 | 18/50 | 18/70 | 18/80 | 18/90 | 18/100 | 18/120 | 18/150+ |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| spells/level ×100 | 40 | 60 | 80 | 90 | 100 | 105 | 110 | 120 | 150 | 170 | 180 | 190 | 200 | 220 | 250 |

Consequences:

- With a casting stat of 16 you get exactly one new spell per level; at 18/50, 1.5; at 18/150+, 2.5.
- Spells are only "short" at low level with a low stat. A Mage with INT 13 (85) at L5 may know
  `(85×5+50)/100 = 4` spells while 8 are available — choose carefully. By mid-game every caster with a
  decent stat can know every spell of its level; at L50 a stat of 16 already allows 50 spells, more
  than any class has (max 30, Mage).
- **Stat drain makes you forget spells.** If `num_allowed` drops below what you know, the most
  recently learned spells are marked *forgotten* (they cannot be cast). Restoring the stat restores
  them automatically, in learning order ("You have remembered..."). Spells above your level are
  also forgotten (only possible after experience drain lowers your level).
- The "Study" indicator on the status bar means `new_spells > 0` and a book in your pack has a
  learnable spell.

### 3.2 Choosing (`CHOOSE_SPELLS`) vs random (Priest, Paladin)

FACT (`src/cmd-obj.c:do_cmd_study_spell()`, `do_cmd_study_book()`):

- Choosers press `G` (or browse and select) and pick any spell with level ≤ L that they do not know,
  from any carried (or floor) book. Learning takes one turn.
- Priests and Paladins pick only the **book**; the game picks uniformly among that book's learnable
  spells (reservoir sampling over eligible spells). The trick (ADVICE): if you want a particular
  prayer, study from the book in which it is the *only* eligible prayer, or delay studying until the
  book you want has few eligible prayers. Because study slots are never lost, delaying a study costs
  nothing except the time the spell is unavailable.
- You must be able to cast (not blind/confused, have light) to study.

### 3.3 Which spell to learn first (general rules)

ADVICE, valid for all choosers:

1. An attack spell you can afford to spam (Magic Missile, Nether Bolt, Stinking Cloud).
2. An escape (Phase Door; Shadow Shift; Fox Form) and a detection (Find Traps/Doors/Stairs, Detect
   Monsters, Detect Life, Read Minds, Seek Battle).
3. Anything that saves consumables you'd otherwise buy every trip (Remove Hunger, Light Room,
   Identify Rune, Recharging).
4. Skip spells you will never cast (see each class's "traps" list); with a good stat you'll get them
   later anyway for the first-cast experience.

### 3.4 First-cast experience

FACT (`src/player-spell.c:spell_cast()`): the first time a spell *succeeds* you gain
`sexp × slevel` experience (raw points; your race/class experience factor applies to the level
thresholds, not to the gain). Notable values:

| Spell (class) | Level | XP for first cast |
|---|---|---|
| Fox Form (Druid) | 1 | 10 (= the whole of level 2 for a 100% expfact character) |
| Magic Missile + Light Room + Find Traps (Mage, L1) | 1 | 4 + 4 + 2 = 10 |
| Identify Rune (Mage) | 8 | 64 |
| Resistance (Mage) | 20 | 400 |
| Banish Evil (Priest) | 25 | 6,250 |
| Mana Storm (Mage) | 45 | 9,000 |
| Enchant Armour (Priest/Paladin) | 37 | 9,250 |
| Command (Necromancer) | 45 | 11,250 |
| Single Combat (Paladin) | 40 | 12,000 |
| Demon Bane (Priest) | 42 | 12,600 |

ADVICE: cast each newly learned spell once at the first safe moment (in town or a cleared corridor);
for utility spells with no valid target (Recharging, Identify Rune, Enchant) you need a valid target
item for the effect to complete — the experience is only given when the effect actually runs
(`effect_do()` returning false, e.g. on cancelling the item prompt, aborts before the reward).

---

## 4. Casting: requirements, energy, over-exertion

FACT:

- **Book**: the book must be in your pack or on the floor under you (`src/ui-spell.c`, `USE_INVEN | USE_FLOOR`).
- **Can cast** (`src/player-util.c:player_can_cast()`): not blind, not confused, and your own square
  must be "seen" (`no_light()`); an `UNLIGHT` character always sees its own square, so necromancers
  can cast in total darkness. There is **no spell that cures blindness or confusion** usable while
  blind or confused — carry potions (CLW cures blindness; CSW/CCW cure confusion; see
  [Chapter 7](07-consumables-and-devices.md)).
- **Shapechanged**: casting, studying, reading, quaffing, using devices, wielding, firing and
  throwing all prompt "Change back and ... (y/n) or (r)eturn to normal?". Answering `y` reverts
  (free) and continues the command (`player_get_resume_normal_shape()`).
- **Energy**: a cast costs `move_energy` (100); with `FASTCAST` (Mage *Mana Channel*) 75
  (`do_cmd_cast()`). A failed cast still costs the turn **and the mana**.
- **Not enough mana**: you are asked to confirm. Fail chance rises by 5% per missing SP (before the
  50% cap). Whether it succeeds or not, your SP become 0 and you are **paralysed for 1d(5×missing+1)
  turns — this bypasses Free Action** — and have a 50% chance of CON damage, permanent with 25%
  probability (`player_over_exert()`). Never do this with monsters near; it is only a deliberate
  "last gamble" when the alternative is certain death.
- **Blackguards** heal a little whenever they cast (`convert_mana_to_hp()`).
- A cast does **not** disturb or wake monsters by itself beyond normal noise; some spells explicitly
  wake (*Unleash Chaos* `WAKE`; *Command*), and damage wakes the target.

---

## 5. Spell failure

FACT (`src/player-spell.c:spell_chance()`), in order:

```
chance  = base_fail                                  (class.txt)
chance -= 3 * (L - spell_level)
chance -= adj_mag_stat[casting stat]
if spell_mana > current SP:  chance += 5 * (spell_mana - current SP)
minfail = adj_mag_fail[casting stat]
if class lacks ZERO_FAIL and minfail < 5:  minfail = 5
if UNLIGHT and own square is lit:  chance += 25
if afraid:  chance += 20
chance = clamp(chance, minfail, 50)                  <-- the 50% cap
if stun > 50 (heavy stun): chance += 25  else if stunned: chance += 15
if amnesia: chance = 50 + chance/2
chance = min(chance, 95)
```

Casting-stat tables (selected rows; `adj_mag_stat` reduces fail, `adj_mag_fail` is the floor):

| Stat | 8 | 10 | 12 | 14 | 16 | 17 | 18/00 | 18/20 | 18/50 | 18/70 | 18/80 | 18/90 | 18/100 | 18/120 | 18/150 | 18/170 | 18/200+ |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| fail reduction | −1 | 0 | 0 | 1 | 3 | 4 | 5 | 7 | 10 | 12 | 15 | 18 | 21 | 27 | 36 | 42 | 51 (57 at 18/220) |
| minimum fail % | 50 | 20 | 12 | 10 | 8 | 7 | 6 | 5 | 4 | 4 | 4 | 3 | 3 | 2 | 1 | 1 | 0 |

Key consequences:

- **The 50% cap is the most important number.** Any spell whose computed fail is above 50% shows and
  behaves as exactly 50% (unless stunned/amnesiac). A L35 Mage learning *Banishment* (base 95) is at
  50%, not 95%; *Holy Word* (95) is 50% for a fresh L39 Priest. High-base spells are usable
  emergency tools from the moment you learn them, at coin-flip odds. Don't rely on them when a
  failure kills you; do use them when a 50% chance is better than any alternative.
- Each character level above the spell level is −3%: a spell learned at its level with base 40 and
  stat 18/50 starts at 30% and reaches the floor (4%) about 9 levels later.
- Fear adds 20% *before* the cap. The anti-fear spells have low base fail so they still work while
  afraid (Heroism 30, Berserk Strength 20).
- Stun adds 15% (25% heavy) *after* the cap: a stunned caster may be at 65–75%. Cure stun first
  (Minor Healing, CLW/CSW potions) before relying on spells.
- Amnesia: `50 + chance/2` — at least 50%. Healing cures amnesia; nothing else castable does.
- **Armour weight does not change failure** in this version; it only reduces mana (section 6).
- **Gloves have no spellcasting penalty in this version.** There is no glove/encumbrance code in
  `src/` at all (the 3.x/4.1 rule that gloves without Free Action/DEX hurt arcane casters is gone).
  Wear the best gloves you find.
- Worked example: Mage, INT 18/50, L20, *Frost Bolt* (L13, base 40): 40 − 21 − 10 = 9% (floor 4).
  Same mage at L13: 40 − 0 − 10 = 30%. Same mage casting *Mana Bolt* (L25, base 60) at L25 with
  18/100: 60 − 0 − 21 = 39%.
- Necromancer, Nether Bolt at L10 with INT 18/20: 22 − 27 − 7 = −12 → floor 5% in the dark. On a lit
  square the +25 is added *before* the floor: −12 + 25 = 13%. For a spell nearer its level the
  penalty is the full +25 (e.g. Disenchant at L16, INT 18/50: 50 − 10 = 40% dark, 50% (capped) lit).
  Darkness matters enormously for necromancers.

Device skill is unrelated to spell failure (see [Chapter 7](07-consumables-and-devices.md) and
[Chapter 1](01-core-mechanics.md)).

---

## 6. Mana: maximum, encumbrance, regeneration

### 6.1 Maximum mana

FACT (`src/player-calcs.c:calc_mana()`):

```
levels = L - first_spell_level + 1
msp    = 1 + adj_mag_mana[stat] * levels / 100        (0 if levels <= 0)
msp   -= max(0, (armour_weight - class_weight_limit) / 10)    (weights in 1/10 lb)
```

Armour weight counts every equipped item except weapon, bow, rings, amulet and light. Recalculated
continuously, so a stat gain is retroactive to level 1 (as the official manual says).

`adj_mag_mana` (extra SP per level ×100):

| Stat | 8 | 10 | 12 | 14 | 16 | 17 | 18/00 | 18/20 | 18/50 | 18/70 | 18/80 | 18/90 | 18/100 | 18/120 | 18/150 | 18/170 | 18/180+ |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| SP/level ×100 | 50 | 70 | 90 | 110 | 130 | 140 | 150 | 170 | 200 | 250 | 300 | 350 | 400 | 500 | 650 | 750 | 800 |

Example max SP (first spell level 1):

| L | stat 16 | 18/50 | 18/100 | 18/150 | 18/200 |
|---|---|---|---|---|---|
| 10 | 14 | 21 | 41 | 66 | 81 |
| 20 | 27 | 41 | 81 | 131 | 161 |
| 30 | 40 | 61 | 121 | 196 | 241 |
| 40 | 53 | 81 | 161 | 261 | 321 |
| 50 | 66 | 101 | 201 | 326 | 401 |

Rogue (first level 5) and Ranger (3) use `L−4` and `L−2` levels. Mana grows much faster with the
stat above 18/70 than below it; for a pure caster every +10 in the casting stat past 18/70 is worth
more mana than several character levels.

**Encumbrance**: 1 SP lost per full pound over the limit (Mage/Necro 30 lb, Druid/Priest/Rogue
35 lb, Paladin/Ranger 40 lb, Blackguard 60 lb). Reference weights (`lib/gamedata/object.txt`): Robe 2,
Soft Leather 8, Hard Leather 12, Leather Scale 14, Metal Scale 25, Chain 22, Mithril Chain 15 lb; caps
2–6 lb; boots 1.5–6; gloves 0.5–4; shields: Wicker 3, Small Metal 6, Leather 9, Large Metal 12 lb;
dragon armour 10–30 lb. A Mage in Soft Leather + leather boots + leather cap + cloak + leather gloves
+ wicker shield carries ~17 lb — no penalty. ADVICE: a few SP lost for much better AC is often fine
at high mana; at low level (10–20 SP total) stay under the limit.

### 6.2 Regeneration

FACT (`src/player-util.c:player_regen_mana()`, called from `process_world()` every 10 game turns,
i.e. once per normal-speed player turn regardless of your speed):

```
percent = 197                     (PY_REGEN_NORMAL, units of 1/65536)
if REGEN flag:      percent *= 2
if resting (>=5 turns, or rest '&' / '*' / HP-or-SP modes immediately): percent *= 2
if IMPAIR_MANA:     percent /= 2
gain per tick = msp * percent / 65536  + 524/65536
```

- About **0.30% of max SP per tick** walking, 0.60% resting, 1.2% resting with *Regeneration*. From
  empty: ~330 ticks walking, ~170 resting, ~85 resting with Regen.
- Speed does not speed up regeneration: a hasted character gets fewer SP per action. Resting while
  hasted wastes haste duration.
- Mana also comes from: Potions of Restore Mana; Mage *Tap Magical Energy*; Necromancer *Tap Unlife*
  and *Power Sacrifice*; Blackguard combat ([Chapter 7](07-consumables-and-devices.md) for items).

### 6.3 Blackguard mana (`COMBAT_REGEN`)

FACT (`src/player-util.c`, `src/player-attack.c`, `src/player-spell.c`):

- Taking damage gives SP: losing X% of max HP gives X% of max(SP,10) (not from poison, starvation,
  "fatal wound").
- Each melee attack command gives 5% of max(SP,10).
- SP *decay* over time at half the normal regeneration rate; the decay is converted to HP at double
  the efficiency of casting. Regen/resting bonuses to the decay apply only when HP ≤ 50%.
- Every spell cast converts its cost to healing: spending X% of max SP restores X/2% of *missing* HP
  (capped at 25% of missing HP per cast).
- So a Blackguard starts fights with little mana and builds it by hitting and being hit. Plan
  spells as finishers and mid-fight buffs, not openers.

---

## 7. How spell effects work

### 7.1 Geometry and targeting

FACT (`src/effect-handler-attack.c`, `src/project.c`):

| Effect | Shape | Stops at first monster? | Hurts floor items? | Notes |
|---|---|---|---|---|
| `BOLT` | single target | yes (even with a target set: it hits whatever is first in the path) | no | |
| `BEAM` | line to range | no, hits all in line | no | |
| `BOLT_OR_BEAM` | bolt, beam with chance `beam% + other` | as above | no | Mage only; beam% = L |
| `LINE` | beam that also affects grids | no | no | Spear of Light lights; Stone to Mud digs |
| `BALL` | explodes at target | **targeted:** flies over monsters to the target grid; **direction:** stops at first monster | yes | damage at distance d from centre = `(dam+d)/(d+1)`: full, 1/2, 1/3... |
| `SPHERE` | ball centred on you | — | yes | does not hit you; source "diameter" sets fall-off (Volcanic Eruption full damage to radius 4) |
| `ARC` | cone | no | yes | full damage out to `4×60/degrees` grids (Acid Spray 60°: 3 grids; River of Lightning 20°: 11 grids), then falls off |
| `SHORT_BEAM` | short cone/beam of fixed length | no | yes | full damage over its whole length |
| `STRIKE` | ball at the target, no path ("from above") | — | yes | target must be in LOS and passable or it lands on **you** (centre on you, monsters around you hit) |
| `SWARM` | N balls at one target point | each stops at first monster | yes | Meteor Swarm |
| `PROJECT_LOS(_AWARE)` | every monster in line of sight | — | no | damage rolled **once** and applied to all |
| `TOUCH` | radius around you | — | yes | |
| `SPOT` | radius around you **including you** | — | yes | Zone of Unmagic hits the caster |

ADVICE: use bolts and beams near loot (they never destroy floor objects); use a target (not a
direction) for balls so they reach the group behind a blocker.

### 7.2 Dice, `$` expressions and `m_bonus`

FACT: `dice:` strings may contain `$X` placeholders resolved by `expr:X:BASE:ops` (bases:
`PLAYER_LEVEL`, `PLAYER_HP` = current HP, `MONSTER_PERCENT_HP_GONE`, ...; `src/effects.c`). `m$M` sets
the *m_bonus* field, which different effects use differently: for `HEAL_HP` it is the percentage of
*missing* HP healed (with the base+dice as minimum); for `TELEPORT` it makes the distance a
percentage of the distance to the level edge; for `SWARM` it is the number of projectiles. Spell
damage gets no device "boost" (`spell_cast()` passes boost 0).

### 7.3 Monster saving throws against status spells

FACT (`src/list-mon-timed.h`, `src/mon-timed.c`):

| Status | Saving throw? | Blocked by flag | Stacking | Max | Notes |
|---|---|---|---|---|---|
| Sleep | yes | `NO_SLEEP` | no | 10000 | |
| Fear | yes | `NO_FEAR` | adds | 10000 | |
| Confusion | **no** | `NO_CONF` | max | 50 | |
| Slow | **no** | `NO_SLOW` | adds | 50 | −2 speed per 10 turns of timer (max −10) |
| Hold | **no** | `NO_HOLD` | max | 50 | broken by any damage from you |
| Stun | **no** | `NO_STUN` | max | 50 | 1 in 10 turns lost, −25% to-hit/damage |
| Disenchant (spell disruption) | no | `IM_DISEN` | max | 50 | only monsters with non-innate spells |

Saving throw (sleep, fear): resist chance = `min(90, monster_level + max(0, 25 − timer/2))`;
uniques roll twice. Huge timers (Mass Sleep 10L+500) reduce this to just the monster's level.
Teleport-away (`AWAY_*`) and fear-from-howl have **no** resistance beyond the type check; teleport
works on uniques. Polymorph (Chaos) never affects uniques.

Consequence (ADVICE): *Slow Monster*, *Confuse Monster* and *Trance* are far more reliable than their
reputation suggests: only 7 monsters in the game have `NO_SLOW` (see the table in
[19.2](#192-monster-flag-prevalence-by-depth)), and there is no level-based save.

---

## 8. Books

FACT (`book:` and `book-properties:cost:commonness:min to max` in `lib/gamedata/class.txt`;
`src/init.c` gives dungeon books `IGNORE` for acid/elec/fire/cold and the `GOOD` kind flag; `src/store.c`:
`always:<book tval>` stocks every **town** book of that realm):

| Book | Realm | Type | Base cost | Commonness | Depth | Classes (spells) |
|---|---|---|---|---|---|---|
| [First Spells] | arcane | town | 25 | 40 | 1–100 | Mage (7), Rogue (6) |
| [Attacks and Knowledge] | arcane | town | 400 | 40 | 10–100 | Mage (6) |
| [Magical Defences] | arcane | town | 1600 | 20 | 30–100 | Mage (6) |
| [Arcane Control] | arcane | dungeon | 30000 | 15 | 40–100 | Mage (6), Rogue (4) |
| [Wizard's Tome of Power] | arcane | dungeon | 50000 | 10 | 60–100 | Mage (5) |
| [Novice's Handbook] | divine | town | 25 | 40 | 1–100 | Priest (6), Paladin (6) |
| [Cleansing Power] | divine | town | 100 | 40 | 5–100 | Priest (6) |
| [Healing and Sanctuary] | divine | dungeon | 10000 | 20 | 30–100 | Priest (6), Paladin (5) |
| [Battle Blessings] | divine | dungeon | 30000 | 15 | 40–100 | Priest (5), Paladin (5) |
| [Wrath of the Valar] | divine | dungeon | 50000 | 10 | 60–100 | Priest (5) |
| [Lesser Charms] | nature | town | 25 | 40 | 1–100 | Druid (6), Ranger (6) |
| [Gifts of Nature] | nature | town | 400 | 40 | 10–100 | Druid (6) |
| [Creature Dominion] | nature | dungeon | 10000 | 20 | 30–100 | Druid (5) |
| [Nature Craft] | nature | dungeon | 30000 | 15 | 40–100 | Druid (5), Ranger (5) |
| [Wild Forces] | nature | dungeon | 50000 | 10 | 60–100 | Druid (5) |
| [Into the Shadows] | shadow | town | 25 | 40 | 1–100 | Necromancer (5), Blackguard (6) |
| [Dark Rituals] | shadow | town | 400 | 40 | 10–100 | Necromancer (5) |
| [Fear and Torment] | shadow | town | 1600 | 20 | 30–100 | Necromancer (5), Blackguard (5) |
| [Deadly Powers] | shadow | dungeon | 30000 | 15 | 40–100 | Necromancer (6), Blackguard (4) |
| [Corruption of Spirit] | shadow | dungeon | 50000 | 10 | 60–100 | Necromancer (5) |

Where to get them (FACT, `lib/gamedata/store.txt`):

- The **Bookseller (store 4)** always stocks every town book of all four realms and buys all books.
  Prices shown in the store include the owner's markup on the base cost (see
  [Chapter 11](11-town-and-shopping.md)).
- The **Magic shop (6)** buys magic books (does not normally stock them). The **Black market** can
  sell anything, including dungeon books, at a large markup.
- **Dungeon books** are only found in the dungeon (or the black market, or a store you sold one to).
  They are `GOOD` objects, so they can come from "good" drops, *Acquirement* and vaults, and they
  cannot burn or dissolve. Town books **can** be destroyed by fire (and the like) in your pack —
  carry a spare of your key town book once you can afford it ([Chapter 5](05-elements-and-status.md)).
- Starting kit: every caster starts with its first town book (Mage/Rogue [First Spells], Druid/Ranger
  [Lesser Charms], Priest/Paladin [Novice's Handbook], Necromancer/Blackguard [Into the Shadows]).

Which book gives what (ADVICE summary; details in each class section):

| Class | Must-buy town books, in order | Dungeon books that change the game |
|---|---|---|
| Mage | First Spells (start) → Attacks and Knowledge (400: Recharging, Identify Rune, Frost Bolt) → Magical Defences (1600: **Teleport Self L7**, Teleport Other, Resistance) | Arcane Control (Mana Bolt, Teleport Level, Detection, Dimension Door), Wizard's Tome (Banishment, Mass Banishment, Mana Storm) |
| Druid | Lesser Charms (start) → Gifts of Nature (400: Stone to Mud, Sense Surroundings, Lightning Strike, Earth Rising) | Creature Dominion (Trance, Mass Sleep, shapes), Nature Craft (**Haste Self**, Herbal Curing), Wild Forces (Meteor Swarm, Rift, Ice Storm) |
| Priest | Novice's Handbook (start) → Cleansing Power (100: **Orb of Draining**, PfE, Remove Curse) | Healing and Sanctuary (**Portal**, **WoR**, **Healing**, Clairvoyance), Battle Blessings, Wrath of the Valar (Banish Evil, Holy Word) |
| Necromancer | Into the Shadows (start) → Dark Rituals (400: Crush, Shadow Shift, Disenchant) → Fear and Torment (1600: Vampire Strike, Dispel Life, Dark Spear) | Deadly Powers (Annihilate, Banish Spirits, Unleash Chaos, Storm of Darkness), Corruption of Spirit (Vampire Form, Curse) |
| Paladin | Novice's Handbook (start) | Healing and Sanctuary (PfE, WoR, Healing), Battle Blessings (Smite Evil, Single Combat) |
| Rogue | First Spells (start) | Arcane Control (**Teleport Self L17**, Teleport Other, Teleport Level) |
| Ranger | Lesser Charms (start) | Nature Craft (**Haste Self L25**, Decoy, Brand Ammunition) |
| Blackguard | Into the Shadows (start) → Fear and Torment (1600: Maim Foe, Howl, Taunting, Venom, Werewolf) | Deadly Powers (Bloodlust, Unholy Reprieve, Forceful Blow, Quake) |

Important implication: **no hybrid class and no Priest gets a teleport/escape spell from a town
book**. Priest *Portal* is in a dungeon book (DL30+), Rogue *Teleport Self* in another (DL40+).
Rogues, Priests, Paladins, Rangers, Druids and Blackguards rely on scrolls for escapes early.

---
## 9. Mage

INT, arcane, `ZERO_FAIL | BEAM | CHOOSE_SPELLS`, first spell L1, armour limit 30 lb, 30 spells in 5
books. The most complete toolbox: unresistable bolts, the best detection, teleport-other, and the
only identify/recharge/banishment caster. Weak HP. Official manual: [birth](https://angband.readthedocs.io/en/latest/birth.html#mage).

### 9.1 Mage spells, resolved

Beam chance for `BOLT_OR_BEAM` spells: L% (Magic Missile: L−10%).

**[First Spells]** (town, 25 gold, starting book)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Magic Missile | 1 | 1 | 22 | 4 | Bolt/beam of `MISSILE` (nothing resists), `((L−1)/5+3)d4`: 3d4 (7.5) L1–5, 4d4 L6–10, 6d4 (15) L16–20, 8d4 (20) L26–30, 10d4 (25) L36–40, 12d4 (30) L46–50. |
| Light Room | 1 | 2 | 26 | 4 | Lights the room you are in plus radius 2; 1 point of `LIGHT_WEAK` (hurts only `HURT_LIGHT` monsters) in radius 2. |
| Find Traps, Doors & Stairs | 1 | 1 | 20 | 2 | Detects traps, doors, stairs within ±22 rows, ±40 columns. |
| Phase Door | 2 | 2 | 22 | 4 | Teleport ~10 squares (randomised ±25%: 8–12). |
| Electric Arc | 2 | 2 | 34 | 4 | Short beam of `ELEC`, length L grids (max range 20), `((L−1)/5+3)d6` (same dice count as Magic Missile, d6): 3d6 at L2–5, 6d6 at L16–20. Hits every monster along it; destroys elec-vulnerable floor items. |
| Detect Monsters | 3 | 2 | 24 | 4 | Shows non-invisible monsters in ±22/±40 for one turn. |
| Fire Ball | 6 | 5 | 33 | 5 | Radius-2 `FIRE` ball, 2L at the centre (12 at L6, 20 at L10, 40 at L20, 100 at L50), half at radius 1, a third at radius 2. |

**[Attacks and Knowledge]** (town, 400 gold, DL10+)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Recharging | 5 | 5 | 28 | 6 | Recharge strength `L/8+6` (6 at L5, 8 at L20, 12 at L50; the Scroll of Recharging is 6). Backfire (destroys one item of the stack) with chance `1/(strength + (100−item level)/10 − 2×charges_per_item)`; on success adds `2 + 1d(strength/(10−ease)+1)` charges. |
| Identify Rune | 8 | 7 | 25 | 8 | Reveals one unknown rune on a chosen item. |
| Treasure Detection | 10 | 3 | 60 | 5 | Detects mineral veins with treasure; *senses* gold and objects (shows that something is there, not what) in ±22/±40. |
| Frost Bolt | 13 | 5 | 40 | 6 | Bolt/beam of `COLD`, `((L−5)/3+6)d8`: 8d8 (36) at L13, 11d8 (49.5) at L20, 14d8 (63) at L30, 17d8 (76.5) at L40, 21d8 (94.5) at L50. |
| Reveal Monsters | 15 | 6 | 40 | 8 | Shows all monsters, including invisible, in ±22/±40 for one turn. |
| Acid Spray | 20 | 5 | 30 | 12 | 60° cone of `ACID`, range 10, `(L/2)d8`: 10d8 (45) at L20, 15d8 (67.5) at L30, 25d8 (112.5) at L50; full damage out to 3 grids, then 4/(d+1). |

**[Magical Defences]** (town, 1600 gold, DL30+)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Disable Traps, Destroy Doors | 5 | 5 | 30 | 6 | Destroys doors and disables traps in radius 1 around you. |
| Teleport Self | 7 | 6 | 35 | 5 | Teleport a distance of `9L/5`% of the distance to the farthest level edge (±25%): 18% at L10, 36% at L20, 54% at L30, 90% at L50. The Scroll of Teleportation is 60% — **the spell only matches the scroll from L34.** |
| Teleport Other | 15 | 10 | 30 | 12 | Bolt that teleports the first monster hit 3L squares away. No saving throw; works on uniques (not in Single Combat arenas). |
| Resistance | 20 | 20 | 65 | 20 | Temporary resist acid, lightning, cold and fire for 20+1d20 turns (one shared roll). |
| Tap Magical Energy | 22 | 2 | 30 | 12 | Drains *all* charges of a wand/staff stack: gain `(5 + item level) × charges / 4` SP (needs at least 6 SP worth), and you are stunned 1–2 turns. |
| Mana Channel | 25 | 10 | 50 | 20 | `FASTCAST` for 5+1d5 turns: each cast costs 75 energy (33% more casts per game time). Cancelled by stun. |

**[Arcane Control]** (dungeon, DL40+)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Door Creation | 13 | 9 | 40 | 12 | Closed (unlocked) door on every empty floor grid adjacent to you. |
| Mana Bolt | 25 | 8 | 60 | 30 | Bolt/beam of `MISSILE` (unresistable), `(L−10)d8`: 15d8 (67.5) at L25, 20d8 (90) at L30, 30d8 (135) at L40, 40d8 (180) at L50. |
| Teleport Level | 28 | 17 | 65 | 20 | One level up or down at random (may do nothing with forced descent). |
| Detection | 30 | 10 | 70 | 30 | Traps, doors, stairs, treasure veins, senses gold and objects, all monsters including invisible, ±22/±40. |
| Dimension Door | 35 | 30 | 80 | 40 | Teleport to (near) a grid you target; a vault target is randomised by up to 10. |
| Thrust Away | 40 | 12 | 90 | 40 | Short beam of `FORCE`, length 1+L/10 (5 at L40), `Ld8` (180 at L40, 225 at L50); 1/3 chance to stun 5–14 turns; knocks monsters back `3 + damage/20` squares. Only force breathers resist. |

**[Wizard's Tome of Power]** (dungeon, DL60+)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Shock Wave | 20 | 5 | 40 | 16 | Radius-2 `SOUND` ball, 2L; 1/3 chance to stun each monster (unless `NO_STUN`). |
| Explosion | 30 | 10 | 50 | 20 | Radius-2 `SHARD` ball 2L, then radius-2 `FORCE` ball L/5 (knockback, stun chance). |
| Banishment | 35 | 45 | 95 | 25 | Removes every non-unique monster of a chosen symbol from the level; you take 1d4 per monster removed. |
| Mass Banishment | 40 | 75 | 90 | 100 | Removes every non-unique monster within 20 squares; 1d3 damage per monster removed. |
| Mana Storm | 45 | 16 | 85 | 200 | Radius-3 `MANA` ball (unresistable), 2L+300 (390 at L45, 400 at L50). |

### 9.2 Mage spell-by-spell advice

- **Magic Missile** — FACT: cheapest damage per SP in the game for single targets until about L20,
  and still ~25–30 damage per SP late; never resisted; beams at L−10%. ADVICE: learn first; the
  default attack against anything weak, anything with unknown resists, and to finish wounded
  monsters. Bolts stop at the first monster: line up.
- **Light Room** — ADVICE: light dark rooms before entering to see what is inside. It does not wake
  monsters by itself, but a lit room is lit for everyone: what you can see can also target you. The
  1-point damage is irrelevant.
- **Find Traps, Doors & Stairs** — ADVICE: cast on arrival on every level and whenever you leave the
  detected area. Essential for stair-scumming and diving ([Chapter 10](10-exploration.md)).
- **Phase Door** — ADVICE: the bread-and-butter escape and positioning tool (break line of sight,
  get out of melee to cast). Keep scrolls anyway: the spell can fail (22% base) and needs you not
  blind/confused.
- **Electric Arc** — ADVICE: for corridors full of weak monsters (e.g. a line of jackals) it beats
  Magic Missile per cast; per SP it's worse (2 SP). Destroys floor items (wands/rings) — don't use it
  over loot.
- **Detect Monsters** — ADVICE: cast before opening doors, on arrival, before resting. Invisible
  monsters need Reveal Monsters (L15).
- **Fire Ball** — ADVICE: worth it against groups of 3+ or `HURT_FIRE` monsters. Against a single
  monster at L6–20 Magic Missile does more per SP. Target the group (not a direction) so the ball
  passes over the front monster.
- **Recharging** — ADVICE: recharge only *empty* wands (charges count against you, `−2×charges`),
  low-level wands (Magic Missile L3, Stinking Cloud) are safe-ish; deep wands (Teleport Other, Drain
  Life) have a real backfire chance — never recharge your only one. A mage with Recharging and
  a Wand of Magic Missile/Stinking Cloud has a second "mana pool".
- **Identify Rune** — ADVICE: big quality-of-life spell; identify runes on wearables you are about to
  rely on (especially to learn curses before wielding, see [Chapter 6](06-equipment.md)).
- **Treasure Detection** — TRAP: 60% base fail and it only *senses* objects. Useful only for gold-diving;
  Detection (L30) supersedes it. Learn for the XP when study slots are plentiful.
- **Frost Bolt** — ADVICE: the main single-target spell from L13 to the mid-20s (~50 damage for 5 SP at
  L20). Swap to Magic Missile against `IM_COLD` monsters (cold immunity: 16% at DL11–20, 34% at
  DL31–40). Doubles against `HURT_COLD`.
- **Reveal Monsters** — ADVICE: from L15 this is your monster detection; 6 SP is cheap insurance
  against ghosts and invisible casters.
- **Acid Spray** — ADVICE: the best crowd spell a mage has until Mana Storm: a 60° cone with full
  damage to 3 grids, 45 damage at L20 for 5 SP. Use it on packs in rooms and against breeders. Acid
  immunity is rare at DL21–40 (5–15%). Destroys acid-vulnerable floor items.
- **Disable Traps, Destroy Doors** — ADVICE: situational; useful when a trap blocks a corridor, or to
  remove doors so you can't be ambushed from them. Mostly XP fodder.
- **Teleport Self** — FACT: short at low level (see table). ADVICE: learn it as soon as you can buy
  Magical Defences (1600 gold) — it's still a free escape that does not use inventory — but until
  ~L34 keep Scrolls of Teleportation as the "get away for real" option.
- **Teleport Other** — ADVICE: the mage's best defensive spell. Removes any single monster, including
  uniques, with no save. Bolt: it hits the *first* monster in the path — clear line needed. The main
  answer to dangerous melee uniques, invisible-but-detected ghosts, and breathers in corridors.
- **Resistance** — ADVICE: cast before fighting dragons, hounds or casters of the four base elements
  when you lack permanent resists; temporary resistance stacks with item resistance (double resist
  cuts damage to 1/9). 20 SP and 65% base fail make it a pre-buff, not an emergency spell.
- **Tap Magical Energy** — TRAP in most situations: it destroys all charges and stuns you (1–2
  turns). Use only on junk wands/staves when you need mana *now* in a safe place, e.g. tapping a
  Staff of Detect Evil (level 20ish, many charges) between fights.
- **Mana Channel** — ADVICE: before a long boss fight; 10 SP buys 5–10 turns of 33% faster casting.
  Stun cancels it: cure stun first.
- **Door Creation** — ADVICE: niche tactical spell: box yourself in to rest, or put a door between you
  and a non-door-opening breather pack. Many monsters open or bash doors.
- **Mana Bolt** — ADVICE: the unique-killer from L25: unresistable, 90 damage at L30 for 8 SP. Starts at
  60% base fail (capped to 50%) — it becomes reliable around L30–35 with a good INT.
- **Teleport Level** — ADVICE: the strongest escape that also leaves the level's threats behind.
  17 SP, 65% base: practise it until reliable before depending on it. Takes effect immediately (unlike
  Deep Descent / Word of Recall scrolls).
- **Detection** — ADVICE: one cast does everything; becomes the standard arrival spell once its fail
  drops (70% base).
- **Dimension Door** — ADVICE: precise escape/positioning: jump across chasms of monsters to the stairs,
  or behind a wall. 30 SP, 80% base.
- **Thrust Away** — ADVICE: a strong short-range spell late (180+ force damage, stun, knockback); its
  90% base fail keeps it at 50% for a while. Knockback also breaks melee contact. Short range (5–6).
- **Shock Wave / Explosion** — ADVICE: arrive only when the Wizard's Tome is found (DL60+). By then
  Acid Spray and Mana Bolt are better per SP for most targets; use Shock Wave against
  acid-immune groups (stun is a nice bonus), Explosion is rarely worth 10 SP.
- **Banishment** — ADVICE: removes a whole symbol (e.g. all `Z` hounds, all `d`/`D`, all `U` that are not
  unique) at the cost of 45 SP and 1d4 HP per removed monster. Superb against hound packs, nests and
  summoners' summons; check HP first: banishing a 40-monster nest costs up to 160 HP.
- **Mass Banishment** — ADVICE: the panic button: clears everything non-unique within 20 squares. 75 SP,
  1d3 HP per monster. Leaves uniques — great for isolating a unique from its escort.
- **Mana Storm** — ADVICE: endgame damage: 400 unresistable damage in radius 3 for 16 SP. The best
  spell for Morgoth/Sauron fights together with Mana Bolt.

### 9.3 Mage priorities

**Workhorse attacks by level**

| Level | Single target | Groups |
|---|---|---|
| 1–5 | Magic Missile | Electric Arc (lines) |
| 6–12 | Magic Missile | Fire Ball (3+ targets) |
| 13–19 | Frost Bolt (Magic Missile vs cold-immune) | Fire Ball |
| 20–24 | Frost Bolt | Acid Spray |
| 25–44 | Mana Bolt (tough/unique), Frost Bolt (routine), Magic Missile (trivial) | Acid Spray |
| 45–50 | Mana Bolt, Mana Storm | Mana Storm, Acid Spray |

**Utility**: Find Traps/Doors/Stairs (L1) → Detect Monsters (L3) → Identify Rune (L8) → Recharging (L5)
→ Reveal Monsters (L15) → Detection (L30). Light Room (L1) for lighting.

**Emergency**: Phase Door (L2), Teleport Self (L7; scroll better before L34), Teleport Other (L15),
Teleport Level (L28), Dimension Door (L35), Mass Banishment (L40), Banishment (L35).

**Traps / low value**: Treasure Detection, Tap Magical Energy (except as deliberate mana top-up),
Door Creation, Explosion; Thrust Away until its fail drops.

Note what the Mage does **not** have: no healing, no cure, no haste, no Word of Recall, no Deep
Descent, no remove curse. Buy potions/scrolls/rods for these ([Chapter 7](07-consumables-and-devices.md)).

---

## 10. Druid

WIS, nature, `ZERO_FAIL | CHOOSE_SPELLS | CHARM`, first spell L1, armour limit 35 lb, 27 spells.
A caster of balls, cones and line-of-sight storms with shapechanges and crowd control. Only two town
books — the middle game depends on finding dungeon books. Official manual:
[birth](https://angband.readthedocs.io/en/latest/birth.html#druid).

### 10.1 Druid spells, resolved

**[Lesser Charms]** (town, 25 gold, starting book)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Detect Life | 1 | 1 | 23 | 2 | Shows all *living* monsters (not undead/nonliving), even invisible ones, ±22/±40, one turn. |
| Fox Form | 1 | 2 | 20 | 10 | Shapechange to fox ([section 17](#17-shapechanges)): +1 blow, moves cost half energy, stealth +5, STR −3, to-hit/dam −3, AC +3, Feather Fall, Free Action. |
| Remove Hunger | 2 | 2 | 25 | 2 | Sets food to at least 50% ("Fed", not full). |
| Stinking Cloud | 3 | 2 | 27 | 4 | Radius-2 `POIS` ball, `L/2+10` (11 at L3, 15 at L10, 20 at L20, 35 at L50). |
| Confuse Monster | 5 | 3 | 30 | 5 | Bolt: confusion for `5+1d(L−5)` turns (5 at L5, 5–20 at L20); ×1.5 vs animals; only `NO_CONF` resists. |
| Slow Monster | 6 | 4 | 30 | 5 | Bolt: slow for `5+1d(L−5)` turns, stacking; −2 speed per 10 turns of timer; ×1.5 vs animals; only `NO_SLOW` (7 monsters) resists. |

**[Gifts of Nature]** (town, 400 gold, DL10+)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Cure Poison | 4 | 4 | 30 | 4 | Cures poison. |
| Resist Poison | 7 | 6 | 32 | 5 | Temporary resist poison 20+1d20 turns. |
| Turn Stone to Mud | 8 | 3 | 25 | 5 | Beam that dissolves the first wall section (not permanent walls); 20+1d30 to `HURT_ROCK` monsters in the beam. |
| Sense Surroundings | 9 | 4 | 35 | 6 | Maps the area ±22 rows × ±44 columns; from L30 ±44 × ±88. |
| Lightning Strike | 12 | 6 | 35 | 8 | `ELEC` strike `(L/4)d4` on the target grid, then a radius-3 `SOUND` blast `L+5` centred there (falls off with distance; stuns 1/3). Needs a visible passable target in LOS, else it centres on you. |
| Earth Rising | 14 | 5 | 40 | 12 | Short beam of `SHARD`, length `4+L/5`, `(L/3+2)d6` (6d6=21 at L14, 8d6=28 at L20, 12d6=42 at L30, 18d6=63 at L50). |

**[Creature Dominion]** (dungeon, DL30+)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Trance | 20 | 10 | 45 | 15 | Holds every adjacent monster for `3+1d(L/7)` turns (×1.5 animals). Only `NO_HOLD` resists; **any damage from you breaks hold**. |
| Mass Sleep | 25 | 15 | 50 | 15 | Sleep (power 10L+500) on every monster in LOS. Monsters without `NO_SLEEP` resist with probability = their level % (uniques roll twice). |
| Become Pukel-man | 30 | 20 | 75 | 50 | Shapechange: speed −5, +4 STR/CON, damage reduction 10, rPois, rShards, protection from stun, regen, hold life, +20 save, +20 AC; cures poison and stun on change. |
| Eagle's Flight | 35 | 20 | 80 | 80 | Shapechange: moves cost 1/4 energy, see invisible, protection from blind/conf/fear, Free Action, trap immunity. |
| Bear Form | 40 | 20 | 85 | 100 | Shapechange: +15 to-hit, +15 to-dam, +5 AC, +1 blow, STR+3 CON+2 INT−2, protection from fear, melee skill +10, stealth −3. |

**[Nature Craft]** (dungeon, DL40+)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Tremor | 20 | 20 | 60 | 16 | Earthquake radius 4 at a targeted grid (no effect in town; can hurt you if you are inside it). |
| Haste Self | 25 | 12 | 65 | 15 | +10 speed for `L+1d20` turns; recasting while hasted adds only 5 turns. |
| Revitalize | 35 | 70 | 90 | 90 | Restores all five stats and experience. |
| Rapid Regeneration | 37 | 20 | 90 | 100 | `HEAL` timed effect 5+1d3 turns: +30 HP every game tick, burns 8× food per tick, ends when hungry. |
| Herbal Curing | 40 | 20 | 90 | 100 | Cures cuts, poison, stun, Black Breath; feeds to 80%. |

**[Wild Forces]** (dungeon, DL60+)

| Spell | Lv | SP | Fail | XP | What it does (FACT) |
|---|---|---|---|---|---|
| Meteor Swarm | 30 | 14 | 85 | 20 | `L/20+2` meteors (3 below L40, 4 from L40), each a radius-1 `METEOR` ball (unresistable) of `L/2+30` (45 at L30, 50 at L40, 55 at L50), all aimed at the same point; each stops at the first monster in its path. Up to 135/200/220 total on one target. |
| Rift | 35 | 20 | 60 | 25 | Beam of `GRAVITY`, `40+Ld7` (180 at L35, 240 at L50); each hit monster may be teleported 10 squares (if 1d127 > its level). Only gravity breathers resist. |
| Ice Storm | 37 | 25 | 75 | 24 | `ICE` on every monster in LOS, `3d(3L)` rolled once (avg ~168 at L37, 226 at L50); cold immunity divides by 9, `HURT_COLD` doubles; 1/3 stun. |
| Volcanic Eruption | 40 | 30 | 75 | 50 | `FIRE` sphere radius 5 around you, `3L/2 + 1d(3L)` (avg 120 at L40, 150 at L50), full strength to radius 4; then an earthquake radius 5 centred on you. |
| River of Lightning | 43 | 35 | 75 | 100 | 20° cone of `PLASMA`, range 20, `(L+10)d8` (238 at L43, 270 at L50), full damage to 11 grids. Only `IM_PLASMA` (8 monsters) resists. |

### 10.2 Druid spell-by-spell advice

- **Detect Life** — ADVICE: cheap; misses undead and golems/elementals/vortices, which is exactly what
  you must not forget. Combine with Sense Surroundings for exploring.
- **Fox Form** — ADVICE: an L1 movement tool: two moves per turn means you outrun almost everything
  early. Use it to cross explored levels, flee, and scout. You cannot read or quaff while a fox (must
  revert, which is free, then act next turn) — so transform only when the danger is "out-runnable".
  The 10 XP first cast is a free level 2.
- **Remove Hunger** — ADVICE: never buy food again.
- **Stinking Cloud** — ADVICE: workhorse from L3 to the mid-20s; radius 2 means it is always an area
  spell. Poison immunity is common (29% at DL1–20, ~40% at DL21–40: jellies, molds, undead, golems,
  many demons/dragons) — check monster recall and switch to Lightning Strike / melee.
- **Confuse Monster / Slow Monster** — ADVICE: underrated because there is **no saving throw** (only
  the flag). Slow stacks: two casts of Slow at L20 average −4 to −6 speed. Excellent against single
  dangerous melee monsters and early uniques without `NO_CONF` (Bullroarer, Wormtongue, Grishnákh,
  Golfimbul, Shagrat, Gorbag, Bolg, Azog — but not Grip/Fang, Nár, Uglúk or Mîm, which have it). At
  depth, `NO_CONF` becomes near-universal (68% at DL31–40) but `NO_SLOW` stays near zero.
- **Cure Poison / Resist Poison** — ADVICE: Cure Poison saves CLW potions early; Resist Poison before
  fighting poison breathers and poison-biting swarms when you lack permanent rPois.
- **Turn Stone to Mud** — ADVICE: digging (shortcuts, escape tunnels, reaching vault items) and
  damaging `HURT_ROCK` monsters: clay/stone golems, umber hulks, xorns/xarens, earth spirits and
  elementals, stone trolls and **Bert, Bill and Tom** (DL30) — 20+1d30 per cast for 3 SP. Cheap, low fail.
- **Sense Surroundings** — ADVICE: core exploration spell (the druid's magic mapping). Doubles in area at
  L30.
- **Lightning Strike** — ADVICE: the mid-game single-target-plus-splash spell: ~38 at L20 for 6 SP. Two
  elements (elec + sound) means it is almost never fully resisted. Requires LOS to a passable target
  grid; if you aim at a direction or an unseen grid it explodes **on your position** (hitting
  monsters around you, not you — the projection lacks the player flag; unverified in play).
- **Earth Rising** — ADVICE: shard short beam, good in corridors (full damage over its length).
  Shards are resisted only by shard breathers. Comparable to Lightning Strike; cheaper (5 SP).
- **Trance** — ADVICE: an escape, not an attack: hold all adjacent monsters, then walk away or phase.
  Any damage you deal breaks it. Animals held longer.
- **Mass Sleep** — ADVICE: put a room to sleep and leave or pick targets. Most deep monsters have
  `NO_SLEEP` (67%+ beyond DL30), so it is mainly a DL20–40 tool.
- **Become Pukel-man** — ADVICE: a defensive melee form: DR 10, rShards/rPois, stun protection, regen,
  +20 AC — but **speed −5** is a large price. Useful for tanking a slow melee unique when you cannot
  escape. Cuts neither heal nor bleed while in this form (`ROCK` flag, see [17](#17-shapechanges)).
- **Eagle's Flight** — ADVICE: fastest overland travel in the game (4 moves per turn), plus
  blind/conf/fear protection and trap immunity. Use for crossing known levels to stairs and for
  escaping slow pursuers. No item use.
- **Bear Form** — ADVICE: turns a druid into a credible melee character (+15/+15, +1 blow). By L40,
  though, your spells usually out-damage your claws; use when out of mana or against magic-resistant
  monsters.
- **Tremor** — ADVICE: targeted earthquake: wall off a corridor, drop rubble between you and a pack, or
  destroy a summoner's surroundings. Can bury items. 60% base.
- **Haste Self** — ADVICE: the druid's most important utility spell (+10 speed, ~45 turns at L25). Cast
  before every serious fight once its fail is acceptable; until then carry Potions of Speed.
- **Revitalize** — ADVICE: full restore after nexus/drain damage, 70 SP; cast in safety.
- **Rapid Regeneration** — ADVICE: 150–240 HP over a few game ticks at the cost of food; out-of-combat
  healing. 90% base fail (capped 50% until high level).
- **Herbal Curing** — ADVICE: the druid's answer to cuts/poison/stun/Black Breath; 90% base so long
  practice needed. Not healing.
- **Meteor Swarm** — ADVICE: the druid's single-target nuke from DL60 books: 200 unresistable damage on
  one target for 14 SP at L40 (four meteors). Each meteor stops at the first monster in its path; with
  monsters in front, they hit the front monster.
- **Rift** — ADVICE: big gravity beam (200+), resisted only by gravity hounds/dragons; may blink targets
  away (useful defensively). Lower fail (60) than the other Wild Forces spells.
- **Ice Storm** — ADVICE: line-of-sight room clearer: one roll applied to every visible monster. Bad vs
  cold-immune crowds (53%+ of DL41+ monsters are `IM_COLD`). Great vs mixed animal/orc/troll packs.
- **Volcanic Eruption** — ADVICE: point-blank fire sphere + earthquake around yourself. The earthquake
  can wall you in and destroy floor items; fire immunity is common deep. Situational.
- **River of Lightning** — ADVICE: the druid's best damage spell: 238–270 plasma in a long narrow cone,
  practically unresisted. Use on lines of monsters and big single targets.

### 10.3 Druid priorities

**Workhorse attacks by level**

| Level | Single target | Groups |
|---|---|---|
| 1–2 | melee / thrown, Fox Form to escape | — |
| 3–11 | Stinking Cloud | Stinking Cloud |
| 12–29 | Lightning Strike, Earth Rising (corridors); Stinking Cloud vs non-poison-immune | Stinking Cloud, Earth Rising |
| 30–36 (with Wild Forces) | Meteor Swarm, Rift | Lightning Strike, Rift (lines) |
| 37–42 | Meteor Swarm, Rift | Ice Storm |
| 43–50 | River of Lightning, Meteor Swarm | River of Lightning, Ice Storm |

Without dungeon books the druid stays on Lightning Strike/Earth Rising/Stinking Cloud. **Find
Creature Dominion (DL30+) and Nature Craft (DL40+) early** — their arrival is the druid's real power
curve.

**Utility**: Detect Life (L1), Remove Hunger (L2), Sense Surroundings (L9), Stone to Mud (L8), Cure
Poison (L4), Resist Poison (L7), Haste Self (L25), Revitalize (L35).

**Emergency**: Fox Form (L1), Trance (L20), Mass Sleep (L25), Eagle's Flight (L35), Rift (L35, teleport
side effect), Herbal Curing (L40). The druid has **no teleport spell, no phase door, no healing spell
(except Rapid Regeneration)** — carry Phase Door, Teleportation, CLW/CSW/CCW.

**Traps / low value**: Volcanic Eruption (self-centred quake), Become Pukel-man (speed −5),
Rapid Regeneration in combat, Tremor when you are inside the radius.

---
