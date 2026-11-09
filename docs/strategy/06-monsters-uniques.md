# Chapter 6 — Monsters and Uniques

*Part of [The Angband Strategy Manual](README.md) for Angband 4.2.6. Every number
in this chapter is taken from `lib/gamedata/monster.txt`,
`lib/gamedata/monster_base.txt`, `lib/gamedata/monster_spell.txt`,
`lib/gamedata/summon.txt`, `lib/gamedata/projection.txt`,
`lib/gamedata/constants.txt`, `lib/gamedata/quest.txt` and the C sources under
`src/` (mainly `mon-attack.c`, `mon-move.c`, `mon-spell.c`, `mon-make.c`,
`mon-summon.c`, `mon-blows.c`, `project-player.c`). Where a claim depends on
code, the file and function are named so you can check it.*

This chapter is about **analysis and tactics**. For the raw stat block of every
monster see [Appendix A1 — Monster Compendium](A1-monster-compendium.md); for a
card per unique see [Appendix A2 — The Uniques](A2-uniques.md); for every monster
spell see [Appendix A3 — Monster Spells](A3-monster-spells.md). Elements and
their side effects are covered in depth in
[Chapter 4](04-resistances-afflictions.md); speed, to-hit and the monster AI in
general are in [Chapter 1](01-mechanics.md).

## Contents

1. [Reading a monster](#1-reading-a-monster)
   1. [Monster memory (recall)](#11-monster-memory-recall)
   2. [Speed and energy](#12-speed-and-energy)
   3. [Hit points, regeneration and variance](#13-hit-points-regeneration-and-variance)
   4. [How often monsters cast, and how often they fail](#14-how-often-monsters-cast-and-how-often-they-fail)
   5. [Flags that matter tactically](#15-flags-that-matter-tactically)
   6. [What monsters drop](#16-what-monsters-drop)
2. [Monster families by glyph](#2-monster-families-by-glyph)
3. [The killers list, by depth](#3-the-killers-list-by-depth)
4. [Monster spells](#4-monster-spells)
5. [Breeders](#5-breeders)
6. [The uniques](#6-the-uniques)
7. [General combat tactics](#7-general-combat-tactics)
8. [Quick reference](#8-quick-reference)

### Conventions

* **DL** = dungeon level (the monster's native `depth:`). Monsters can appear
  out of depth; uniques in particular are often met deeper than their DL.
* **Speed** is written as a modifier to normal: a monster with `speed:120` in
  `monster.txt` is **+10**. Normal speed is 110 in the data files.
* **1/N** is a spell or innate-attack frequency: `spell-freq:4` means the
  monster *tries* a spell on about 1 turn in 4 (see §1.4 for the details).
* **HP** for a unique is exact; for a normal monster it is the average (§1.3).
* Damage ranges are written *average / maximum*.
* Breath damage is computed from the monster's **current** HP (§4.3).

---

## 1. Reading a monster

### 1.1 Monster memory (recall)

Angband keeps a *monster memory* (lore) for every race, filled in as you
observe monsters: what you have seen them do, what you have seen them resist,
how many you have killed and how many of your previous characters they have
killed. You can read it:

* **Look** (`l`, or `x` in the roguelike keyset) at a visible monster and press
  `r` to recall it.
* **Identify symbol** (`/`) followed by a symbol, then answer *yes* to see the
  memory of every race with that glyph. `Ctrl-a`, `Ctrl-u` and `Ctrl-n` at the
  symbol prompt give *all*, *unique* and *non-unique* monsters
  (`docs/command.rst`).
* **Knowledge menu** (`~`), *Monster knowledge*, browsed by family.
* A **monster recall subwindow** and the **monster list** (visible / aware
  monsters) can be opened as terminal subwindows.

Things worth knowing about the recall display (`src/mon-lore.c`):

* **Spell colours are personalised.** Each spell is coloured according to
  *your current resistances* (`spell_color()` in `mon-lore.c`): the base colour
  means it will hurt you fully, the "resist" colour means you resist part of it,
  the "immune" colour means it cannot touch you. For saving-throw spells the
  game checks whether your saving throw is below 100 and whether you have the
  matching protection (free action, protection from confusion, etc.). Read the
  colours before a fight — a red `HOLD` means you lack Free Action.
* **Speed is shown as a multiplier.** `lore_multiplier_speed()` prints e.g.
  "moves at 2.0x normal speed, which is 1.0x your current speed". The second
  number is the one that matters: above 1.0x the monster gets more turns than
  you.
* **What you have not observed is not shown.** HP, AC, spell frequency, the
  number and kind of blows, resistances and drops are only filled in as you
  watch, kill or are hit. A blank recall is not a safe monster; the
  [appendices](A1-monster-compendium.md) of this manual list everything the
  data files contain.
* **Kills and deaths** are counted across characters (`lore->deaths`); a
  monster with "has slain N of your ancestors" deserves respect.

### 1.2 Speed and energy

Each game turn every creature gains energy from the table `extract_energy[]`
in `src/game-world.c`; a creature acts when it has accumulated
`move-energy` = 100 (`constants.txt: world:move-energy:100`). The table is
**not linear above +20**:

| Speed | Energy/turn | Relative to normal | Typical holders |
|---:|---:|---:|---|
| -10 | 5 | 0.5x | worm masses, creeping copper coins, colossus |
| +0 | 10 | 1.0x | most early monsters |
| +5 | 15 | 1.5x | mature dragons (115), olog, ogre mage |
| +10 | 20 | 2.0x | most mid-game monsters; Grip and Fang; ancient dragons, great wyrms |
| +20 | 30 | 3.0x | Sméagol, time hound, many late uniques, Sauron |
| +30 | 38 | 3.8x | Morgoth, Harowen, Pazuzu, Cantoras, Wolf-Sauron, death mold |
| +40 | 42 | 4.2x | (no monster) |

Consequences:

* **+10 is double speed.** A +10 monster against a +0 player gets two turns per
  player turn. This is why Grip and Fang (+10 at DL2) and Bullroarer (+10 at
  DL5) kill so many level-1 characters.
* **Diminishing returns above +20.** Going from +20 to +30 is only 3.0 → 3.8
  (+27%), not +50%. But it is exactly what you need against Morgoth (+30): at
  equal speed you trade one action for one action; at +20 you face Morgoth
  getting about 1.27 actions per yours, which means regular double moves.
* **Non-unique monsters vary slightly.** `mon-make.c` adds
  `rand_spread(0, turn_energy(speed)/10)` to each individual: ±1 speed at +0,
  ±2 at +10, ±3 at +20. Two "+10" hounds in the same pack may be at +8 and +12.
  Uniques never vary.
* **Slow (−10) on you halves your turns.** Player slowness is a flat −10 speed
  (`player-calcs.c`: `state->speed -= 10`). Against a +10 monster that is a 4:1
  action ratio. Free Action prevents monster `SLOW` spells (the `SLOW` timed
  effect has `fail:1:FREE_ACT` in `player_timed.txt`) but **not** inertia
  breath, which applies slowness directly.

### 1.3 Hit points, regeneration and variance

* **Uniques have exactly their listed HP** (`mon-make.c`: *Uniques get a fixed
  amount of HP*).
* **Ordinary monsters roll HP from a normal distribution** around the listed
  average with a standard deviation of about 1/8 of it (`mon_hp()`), clipped to
  ±4 SD. A pack of 200-HP monsters will contain some with ~150 and some with
  ~250.
* **Regeneration**: every 100 game turns (10 normal-speed player turns),
  each wounded monster regains `maxhp/100` HP, doubled for `REGENERATE`
  (`regen_monster()` in `mon-move.c`). For most monsters this is negligible in
  a fight but it matters in two cases: (a) you leave and come back — a monster
  you wounded heals 1% per 10 turns; (b) huge regenerators: Morgoth regains
  400 HP and Sauron 160 HP every 100 game turns. Long fights against them are
  races.
* **Breath scales with current HP** (§4.3), so wounding a dragon before it
  breathes reduces its breath.

### 1.4 How often monsters cast, and how often they fail

The `spell-freq:N` and `innate-freq:N` lines are converted to a percentage
`100/N` (`parse_monster_spell_freq()` in `mon-init.c`). Each turn a monster
first checks for a normal spell, then for an innate attack (breaths, arrows,
boulders, spits, shrieks — anything marked innate), in
`make_ranged_attack()` (`mon-attack.c`). The check in `monster_can_cast()`:

1. A monster with `MFLAG_NICE` (just created, not yet acted) cannot cast.
2. If you are **taunting** (`TMD_TAUNT`), the chance is halved.
3. If the monster is at its **preferred range** (`best_range`), the chance is
   **doubled**. Spellcasters and breathers prefer to keep their distance.
4. The target must be within `max-range` (20 grids) and in a projectable line.

After a spell is chosen, **non-innate spells can fail**. In 4.2.6 the code is:

```c
int power = MIN(mon->race->spell_power, 1);
failrate = 25 - (power + 3) / 4;   /* not for STUPID monsters */
if (afraid)          failrate += 20;
if (confused || disenchanted) failrate += 50;
```

Because of the `MIN(…, 1)` the base fail rate is effectively **24% for every
non-stupid caster regardless of level** (25% for a spell power 0 monster).
Innate attacks (breaths, arrows, boulders) never fail. Two practical
consequences: (a) confusing or frightening a caster is a real defence (+50% and
+20% failure respectively — `NO_CONF` and `NO_FEAR` monsters are immune);
(b) Morgoth's mana storms fail about a quarter of the time, but you cannot
count on it.

**Spell choice is filtered by intelligence** (`make_ranged_attack()`):

* Monsters that are not `STUPID` drop spells the AI considers useless against
  you (`remove_bad_spells()` → `unset_spells()` in `mon-spell.c`): elemental
  attacks you resist are dropped with chance `res_level × 25%` (50% for
  `SMART` monsters); timed-effect spells you are protected against are dropped
  with chance 2/3 (always for `SMART`). This only uses what the monster has
  *learned* about you, which needs the birth option **Monsters learn from their
  mistakes** (`birth_ai_learn`, on by default).
* Bolt spells are dropped if another monster is in the way.
* Summons are dropped if there is no empty grid within 2 squares of the caster
  in its line of sight (`summon_possible()`) — the basis of corridor tactics,
  §7.2.
* A `SMART` monster below 10% HP will, half the time, use only non-damaging
  spells (heal, blink, teleport, summon) — the "desperation" behaviour.

### 1.5 Flags that matter tactically

The full flag list is `src/list-mon-race-flags.h`. A monster inherits the
flags of its base (`monster_base.txt`) plus its own. The ones that should
change what you do:

#### Movement and terrain

| Flag | Meaning | What to do |
|---|---|---|
| `NEVER_MOVE` | Never moves (molds, mushrooms, eyes, quylthulgs, lurkers, mimics, jellies, Old Man Willow, the druj) | Never step next to an unknown one. Kill it from range or walk around it. It can still cast and gaze. |
| `NEVER_BLOW` | Has no melee (quylthulgs) | Safe to stand next to; the danger is summons. |
| `RAND_25`, `RAND_50` | 25% / 50% (cumulative 75%) chance of a random step | Erratic movers are hard to pin down; they also waste turns. Grip, Fang, Sméagol, vortices, insects. |
| `PASS_WALL` | Walks through rock (ghosts, some drakes, xorns, dreads, Quaker) | Corridors do not protect you; they come out of the wall next to you. Stand in a room or near permanent features. |
| `KILL_WALL` | Bores through rock, leaving rubble-free floor (umber hulk, abyss worm mass, hasty ent, black reaver, Makar, Maeglin, storm of Unmagic) | Same as above, but they make new corridors. |
| `SMASH_WALL` | Smashes walls (only Morgoth) | |
| `MOVE_BODY` / `KILL_BODY` | Pushes past / kills weaker monsters in the way | Escorts do not block them; a corridor full of orcs will not slow Azog. |
| `OPEN_DOOR`, `BASH_DOOR` | Can open / bash doors | Jamming a door only stops monsters that have neither. |
| `PASS_WEB`, `CLEAR_WEB` | Spiders move freely through webs; most others must clear them (costs a turn) | Webs (from `WEAVE`) block you, not spiders. |
| `TAKE_ITEM`, `KILL_ITEM` | Picks up / destroys floor items | Don't fight a gelatinous cube over your loot pile. |

#### Detection

| Flag | Meaning | What to do |
|---|---|---|
| `INVISIBLE` | Cannot be seen without See Invisible (62 races incl. all ghosts, quylthulgs, many uniques) | Carry See Invisible by DL20–30; `Detect Invisible` scrolls / `True Seeing` potions in emergencies; telepathy shows them unless `EMPTY_MIND`. |
| `COLD_BLOOD` | Invisible to infravision | Infravision is a minor sense; matters early. |
| `EMPTY_MIND` | **Never** shown by telepathy (jellies, molds, golems, elementals, skeletons, zombies, vortices, quylthulgs, lurkers, creeping coins, xorns) | ESP will not save you from a quylthulg or an invisible elemental. |
| `WEIRD_MIND` | Only one in ten individuals is visible to telepathy: `monster_is_esp_detectable()` shows only those whose index `% 10 == 5` | Ants, spiders, worm masses, insects, centipedes, beetles, some bats. |
| `UNAWARE`, `MIMIC_INV` | Starts disguised as an object or floor (mimics `!?=~`, creeping coins `$`, lurkers and trappers) | Treat unknown objects in odd places with suspicion; detect monsters before picking things up. |
| `ATTR_CLEAR`, `CHAR_CLEAR`, `ATTR_MULTI`, `ATTR_FLICKER` | Display only | A flickering/multicoloured glyph is usually something nasty (Morgoth, aether vortex, Saruman). |

#### Behaviour / AI

| Flag | Meaning |
|---|---|
| `SMART` | Uses everything it knows about your resistances; never wastes protected-against status spells; uses desperation spells at low HP; always wakes from noise in `cave-map.c`. |
| `STUPID` | Never filters spells; never fails spells; rarely woken by disturbance. Jellies, mushrooms. |
| `SPIRIT` | Detected by "detect spirits"-type effects and teleported by `AWAY_SPIRIT` projections (`project-mon.c`). |
| `POWERFUL` | Bigger balls (+1 radius) and breath that stays at full strength further out (breath "diameter" ×1.5); the side effects of its breaths use power ≥ 80 (e.g. nether breath that also drains mana and energy, §4.3). |
| `GROUP_AI` | Pack tactics: will not follow you into a corridor while you are above half HP; tries to surround you in rooms (see §7.6). Hounds `Z`, wolves, cave spiders, army ants, hellhounds, craban. |
| `FRIENDS`/`friends:` lines | Generated with companions (`friends:chance:number:race:role`). The *role* `servant`/`bodyguard` is used by group AI. |
| `MULTIPLY` | Breeds (§5). |
| `REGENERATE` | Double regeneration (trolls of note, vampires, Morgoth, Sauron). |
| `FORCE_SLEEP` | Always generated asleep. |
| `FORCE_DEPTH` | Never generated shallower than its native level (Sauron, Morgoth and Sauron's forms). |
| `UNIQUE` / `QUESTOR` | One of a kind / must be killed to win (§6.12). |

#### Race flags (for slays)

`ORC`, `TROLL`, `GIANT`, `DRAGON`, `DEMON`, `UNDEAD` are hit by ×3 slays (×5
for *kill dragon/demon/undead*), `EVIL` and `ANIMAL` by ×2 slays
(`slay.txt`). Note the less obvious memberships, which come from bases:

* All of `o` (orcs) are `ORC`; Ugluk's gang too. `O` (ogres) and `P` (giants)
  are both `GIANT`.
* `drolem` carries `DRAGON` — slay dragon works on it.
* `dracolich` is `ancient dragon` base (DRAGON) **and** `UNDEAD`.
* Ghosts `G`, liches `L`, mummies/zombies `z`, skeletons `s`, vampires `V`,
  wraiths `W` are `UNDEAD` and `EVIL`.
* `Z` hounds, `C` dogs, `f` cats, `a` ants, `b` bats, `B` birds, `c`
  centipedes, `K` beetles, `M` hydras, `R` reptiles, `r` rodents, `w` worms,
  `J` snakes, `Y` yeti are `ANIMAL`. Spiders `S` are **not** animal by base
  (only individual spiders like Shelob have it).
* `NONLIVING` (golems, elementals, vortices) cannot be "killed", only
  "destroyed"; it matters for some effects (drain life).

#### Vulnerabilities and immunities

| Flag | Effect (from `project-mon.c` / `obj-slays.c`) |
|---|---|
| `HURT_LIGHT` | Light beams/balls do ×2; the weak light of a *Wand of Light* or *Rod of Illumination* hurts **only** these. Orcs (most), trolls (stone/forest/cave), vampires, wraiths, worm masses, ghouls, many spiders. |
| `HURT_ROCK` | Stone to Mud (20+1d30) damages them. Golems of clay/stone, stone trolls, xorns, earth spirits/elementals, Quaker, Bert/Bill/Tom. |
| `HURT_FIRE`, `HURT_COLD` | Fire/cold spells ×2; fire/cold brands get their *extra* damage doubled (`get_monster_brand_multiplier()`). Hydras (cold), nagas (cold), snakes and reptiles (cold), trees/molds/mushrooms/zombies/vampires (fire). |
| `IM_ACID/ELEC/FIRE/COLD/POIS` | Brands of that element do nothing extra; spells of that element do 1/9 damage. |
| `IM_NETHER`, `IM_WATER`, `IM_PLASMA`, `IM_NEXUS`, `IM_DISEN` | Resist that projection. Nether is special (`project_monster_handler_NETHER()`): **undead take no nether damage at all**, `IM_NETHER` monsters take ×3/(d6+6), and other `EVIL` monsters take half. |
| A monster that **breathes** an element resists it (e.g. light breathers take `×2/(d6+6)` from light). | |
| `NO_FEAR`, `NO_CONF`, `NO_SLEEP`, `NO_STUN`, `NO_HOLD`, `NO_SLOW` | Immune to that status. All uniques of note have `NO_CONF`/`NO_SLEEP`; most bases of undead have `NO_STUN`. |

### 1.6 What monsters drop

From `mon_create_drop_count()` and `monster_death()` in `mon-make.c`:

| Flag | Objects |
|---|---|
| `DROP_20` / `DROP_40` / `DROP_60` | one object with 20/40/60% chance |
| `DROP_1` | exactly 1 |
| `DROP_2` | 1–3 |
| `DROP_3` | 2–4 |
| `DROP_4` | 2–6 |
| `ONLY_GOLD` / `ONLY_ITEM` | no objects / no gold; otherwise 50/50 per drop |
| `DROP_GOOD` | every object is at least "good" |
| `DROP_GREAT` | every object is "great" (ego or artifact level rolls) |
| `drop:` / `drop-base:` lines | specific items with a percentage (only 2 non-uniques in 4.2.6 use `drop:`, both Phase Door; Farmer Maggot drops mushrooms via `drop-base:`) |

Uniques get a bonus: the object level is
`max((monlevel + depth)/2, monlevel)` where `monlevel = min(level+15, 2×level)`
for uniques, plus an extra "good" roll (`extra_roll`). Killing a unique **in or
above** its native depth still rolls from its own (boosted) level, which is why
early uniques are worth hunting. **Morgoth** additionally drops every artifact
whose kind has `QUEST_ART` — *Grond* and the *Massive Iron Crown of Morgoth* (`'Grond'` and `of Morgoth`
in `artifact.txt`, section "Quest artifacts").

---

## 2. Monster families by glyph

Every monster has a **base** in `monster_base.txt`, which supplies its glyph,
pain messages and shared flags. Knowing the base tells you most of what a
newly met monster can do. Below, each family lists its base flags, its
members' depth range and the representative or dangerous members with
**DL / speed / HP / key attacks** from `monster.txt`. Counts exclude uniques
(which are in §6) and shape-only forms (`rarity:0`).

### Summary table

| Glyph | Base | # | DL range | Base flags worth knowing | Main threat | Answer |
|:-:|---|--:|---|---|---|---|
| `a` | ant | 9 | 1–35 | ANIMAL, WEIRD_MIND | late ants in groups with elemental bites/spit | rFire/rElec by DL30; corridor |
| `A` | ainu | 9 | 34–71 | IM_COLD/FIRE/POIS, NO_CONF/SLEEP/HOLD, SMART | heavy melee, blind/scare, summons of other Maiar | resist blind/fear, speed |
| `b` | bat | 9 | 1–32 | ANIMAL | erratic, fast; disenchanter bat; doombat fire | patience, rFire |
| `B` | bird | 6 | 2–48 | ANIMAL | crebain shriek; winged horror drains | ranged |
| `c` | centipede | 8 | 1–25 | ANIMAL, WEIRD_MIND | carrion crawler paralysis | **Free Action by DL20** |
| `C` | canine | 10 | 0–63 | ANIMAL | packs (GROUP_AI): wolves, wargs, hellhounds | corridor, area damage |
| `d` | dragon | 29 | 9–43 | DRAGON, EVIL, FORCE_SLEEP | breaths (HP/3 base elements) | base resists; kill asleep |
| `D` | ancient dragon | 24 | 50–82 | DRAGON, EVIL, SMART, POWERFUL, DROP_4, NO_CONF/SLEEP/HOLD | huge breaths, summons at the top end | resists, HP, speed |
| `e` | eye | 10 | 1–67 | — (eyes vary) | paralysis gaze, mana drain, forget | FA; kill at range |
| `E` | elemental | 16 | 12–65 | EMPTY_MIND, IM_POIS, NONLIVING | invisible air things, PASS_WALL earth things | SI; Stone to Mud |
| `f` | feline | 6 | 0–26 | ANIMAL | fast melee; displacer beast invisible | — |
| `F` | dragon fly | 4 | 5–6 | ANIMAL, WEIRD_MIND, RAND_50 | weak breaths | — |
| `g` | golem | 13 | 14–75 | EMPTY_MIND, IM_ELEC, NONLIVING | slow but huge HP; drolem breath; silent watcher | avoid unless worth it |
| `G` | ghost | 14 | 3–62 | UNDEAD, EVIL, **INVISIBLE, PASS_WALL**, COLD_BLOOD, IM_COLD/POIS | invisible wall-walkers draining stats/XP/mana | **See Invisible**, Hold Life |
| `h` | humanoid | 11 | 0–65 | OPEN/BASH_DOOR | dwarf casters (blind/conf), mind flayer | pBlind, pConf, saving throw |
| `H` | hybrid | 9 | 2–68 | — | gorgimaera paralysis, jabberwock chaos | FA, rChaos late |
| `i` | icky thing | 7 | 1–14 | RAND_50 | blue icky thing breeds | trivial |
| `I` | insect swarm | 7 | 3–26 | ANIMAL, WEIRD_MIND, **MULTIPLY**, RAND_50/25 | breeders | kill early or leave |
| `j` | jelly | 14 | 2–23 | EMPTY_MIND, STUPID | passive touch effects, drain mana | don't touch; ranged |
| `J` | snake | 9 | 1–78 | ANIMAL, HURT_COLD | slow armoured; nightcrawler late | — |
| `k` | kobold | 5 | 1–5 | EVIL, IM_POIS | archers & shamans in groups | — |
| `K` | killer beetle | 8 | 13–45 | ANIMAL, WEIRD_MIND | heavy bites | — |
| `l` | tree | 4 | 11–52 | REGENERATE, HURT_FIRE, IM_WATER | huge HP, TELE_TO | fire |
| `L` | lich | 5 | 34–74 | UNDEAD, EVIL, SMART, IM_COLD/POIS | brain smash, drain mana, TELE_TO, summons | high save, FA, pBlind |
| `m` | mold | 9 | 1–47 | NEVER_MOVE, HURT_FIRE, IM_POIS | touch effects; death mold | don't touch |
| `M` | hydra | 8 | 17–57 | ANIMAL, HURT_COLD, ONLY_GOLD | many bites, fire/poison breath, scare | rFire, rPois, cold attacks |
| `n` | naga | 6 | 3–48 | EVIL, HURT_COLD, FEMALE | slow tanks; gorgon | — |
| `o` | orc | 8 | 6–18 | ORC, EVIL | big groups with archers | slay orc, corridors |
| `O` | ogre | 6 | 13–32 | GIANT, EVIL | groups, shamans/mages | — |
| `p` | person | 39 | 2–57 | — | casters and archers of every kind | pBlind, pConf, FA |
| `P` | giant | 9 | 25–66 | GIANT, EVIL | boulders, elemental fists | slay giant |
| `q` | quadruped | 6 | 9–39 | — | night mare XP drain | — |
| `Q` | quylthulg | 9 | 20–76 | **INVISIBLE, EMPTY_MIND, NEVER_MOVE, NEVER_BLOW** | summons/teleports every turn | find & kill fast, or leave |
| `r` | rodent | 5 | 1–15 | ANIMAL | breeders | — |
| `R` | reptile | 9 | 1–45 | ANIMAL, HURT_COLD | **basilisk paralysis** | FA |
| `s` | skeleton | 8 | 5–74 | UNDEAD, EVIL, EMPTY_MIND, IM_COLD/POIS | druj (never-moving casters) | SI irrelevant; ranged |
| `S` | spider | 14 | 2–63 | PASS_WEB, BASH_DOOR | swarms, poison, webs, Gorgoroth breath | rPois, light |
| `t` | townsfolk | 11 | 0 | — | thieves | — |
| `T` | troll | 12 | 17–42 | TROLL, EVIL | groups, regeneration (some) | slay troll, light |
| `u` | minor demon | 8 | 8–40 | DEMON, EVIL, IM_FIRE | imps/quasits (invisible, TELE_LEVEL) | SI, rNexus-ish |
| `U` | major demon | 14 | 40–85 | DEMON, EVIL, NO_CONF/SLEEP/HOLD, ONLY_ITEM | fire, summons, Balrogs | rFire/imm, speed |
| `v` | vortex | 12 | 21–95 | EMPTY_MIND, POWERFUL, RAND_50, NONLIVING | engulf + breath of their element | the matching resist |
| `V` | vampire | 4 | 27–59 | UNDEAD, EVIL, REGENERATE, HURT_LIGHT/FIRE | XP drain, mind blast, hold | Hold Life, FA |
| `w` | worm | 11 | 1–40 | ANIMAL | breeders | — |
| `W` | wraith | 10 | 24–73 | UNDEAD, EVIL, HURT_LIGHT, IM_COLD/POIS | XP drain, hold, nether bolts | Hold Life, FA, rNether |
| `x` | lurker | 2 | 14–36 | INVISIBLE, EMPTY_MIND, UNAWARE, NEVER_MOVE | looks like floor; trapper paralyses | FA |
| `X` | xorn | 3 | 16–40 | HURT_ROCK, EMPTY_MIND | wall walkers | Stone to Mud |
| `y` | yeek | 3 | 2–12 | ANIMAL, IM_ACID | nothing | — |
| `Y` | yeti | 2 | 9–20 | ANIMAL, IM_COLD | — | — |
| `Z` | zephyr hound | 21 | 15–74 | ANIMAL, GROUP_AI | packs of breathers | the right resist; corridor |
| `z` | zombie/mummy | 9 | 7–38 | UNDEAD, EVIL, IM_COLD/POIS, HURT_FIRE | ghoul/ghast paralysis | FA |
| `$` | creeping coins | 5 | 4–27 | MIMIC_INV, UNAWARE, ONLY_GOLD, IM_ELEC/POIS | poison touches | don't pick up blindly |
| `!?=~` | mimic | 4 | 18–35 | NEVER_MOVE, UNAWARE, EMPTY_MIND | spells when "picked up" | detect monsters |
| `,` | mushroom | 9 | 1–32 | EMPTY_MIND, STUPID, HURT_FIRE | shriekers, memory moss | kill from range |

The sections below expand on each family, ordered roughly by how often they
end characters.

### `Z` — Zephyr hounds

Base: `ANIMAL | GROUP_AI`. Every hound race has `friends:100:2d7:Same`, so a
pack is the leader plus 2–14 more. Packs arrive **awake and together**, they
surround you in rooms and lurk outside corridors while you are healthy
(`get_move()` in `mon-move.c`), and almost every hound breathes (`innate-freq`
1/5 or 1/10). Because breath is `HP/6` (high elements) or `HP/3` (base
elements and gravity/time), individual breaths are small — the danger is
**fifteen of them in one turn** plus the side effects.

| Hound | DL | Spd | HP | Breath (max per hound) | Side effect that kills |
|---|--:|--:|--:|---|---|
| light / dark hound | 15 | +0 | 21 | light/dark 3 | blindness (pBlind/rLight/rDark) |
| clear hound | 15 | +0 | 21 | none; **invisible** | — |
| fire / cold / energy hound | 18 | +0 | 35 | 11 | inventory burn/shatter |
| earth / air / water hound | 20 | +0 | 68 | shards 11 / poison 22 / acid 22 | cuts / poison |
| vibration hound | 27 | +0 | 138 | sound 23 | **stun** (no pStun/rSound) |
| nexus hound | 27 | +0 | 138 | nexus 23 | **teleport / teleport-to / teleport level / stat scramble** |
| gravity hound | 35 | +0 | 193 | gravity 64 | **blink + slow (−10) + stun**, unresistable |
| inertia hound | 35 | +0 | 193 | inertia 32 | **slow (−10)**, unresistable, even with FA |
| impact hound | 35 | +0 | 193 | force 32 | stun + knock-back |
| multi-hued hound | 43 | +0 | 220 | acid/cold/elec/fire/poison 73 | burns several inventory types |
| nether hound | 51 | +10 | 330 | nether 55 | XP drain (rNether / Hold Life) |
| time hound | 51 | **+20** | 330 | time 110 | **stat drain / XP drain**, unresistable |
| plasma hound | 51 | +10 | 330 | plasma 55 | stun (pStun) |
| chaos hound | 64 | +10 | 930 | chaos 155 | confusion, hallucination, XP drain (rChaos) |
| hound of Tindalos | 69 | +20 | 480 | nether 80, disen 80, time 150 | **invisible + PASS_WALL** |
| aether hound | 74 | +20 | 1230 | 18 elements, 150–410 each | everything |

(Breath = current HP / divisor, capped; e.g. a gravity hound has 193 HP and
gravity's divisor is 3, so 64. Values from `projection.txt`.)

Tactics:

* **Fight in a corridor, not a room.** In a corridor only one or two can reach
  you and only those with line of sight breathe. `GROUP_AI` hounds will often
  refuse to enter while you are above 50% HP and wait just out of sight —
  step back so the corridor bends, and they will come one at a time when you
  look weak, or throw/zap down the corridor.
* **Gravity and inertia packs are the classic mid-game killers** (DL35+):
  gravity blinks you out of your corridor into the room, slows you and stuns
  you; there is no resistance to gravity or inertia. Have Free Action *and*
  protection from stun *and* a speed margin, or leave the level.
* **Time hounds** (+20) at DL51 drain stats and experience with no resistance.
  Unless you can kill them very fast, leave.
* **Nexus hounds**: rNexus (or a very high saving throw against the
  stat-scramble part) before DL27–30. Note teleport level from nexus can drop
  you a level deeper.
* **Area attacks** (balls, *Dispel*, Banishment of `Z`) are efficient: hound
  HP is low.

### `d` and `D` — Dragons

Base `dragon` (`d`): `DRAGON | EVIL | SPIRIT | FORCE_SLEEP`. Base
`ancient dragon` (`D`): adds `POWERFUL | SMART | DROP_4 | MOVE_BODY |
NO_CONF | NO_SLEEP | NO_HOLD`. All start **asleep**: the single most useful
fact about dragons. A sleeping dragon cannot breathe; a damaged one breathes
less (breath = current HP/3 for acid/elec/fire/cold, /3 capped at 800 for
poison, /6 capped at 500–600 for the others).

| Stage | Examples (DL, speed, HP) | Max breath at full HP |
|---|---|---|
| Baby (`d`, DL9–11) | baby blue/white/green/black/red/gold D9 +0 88; baby multi-hued D11 114 | 29; MH 38 |
| Young (`d`, DL29–32) | young blue/white/green D29 237; black/gold/red D31 264; young MH D32 281 | 79–93 |
| Drakes (`d`, DL33) | shadow drake (invisible, PASS_WALL, nether), chaos, law, balance, ethereal (invisible, PASS_WALL), crystal drake 396 | 44–88 |
| Mature (`d`, DL39–43) | mature white/blue/green D39 +5 700; red/gold/black D41 740; mature MH D43 +5 1000 | 233–246; MH 333 each |
| Ancient (`D`, DL50–55) | ancient blue/white/green D50 +10 1500; black/red/gold D51 1560; ancient MH D53 2400; death drake D50 1950 (invisible, PASS_WALL, nether); dracolich D55 3080; dracolisk D55 3080 | 500–520; AMHD 800; dracolich nether 513 + cold 1026; dracolisk fire 1026 + nexus 400 |
| Great wyrms (`D`, DL70–82) | great storm/ice/swamp D70 4200; great hell/bile/thunder D73 4500; chaos/law D75 3960; annihilation, sky dragon D80 5000; balance, many colours D82 5200 | 1400–1600 fire/cold/acid/elec; 800 poison; 500–600 high elements |

Tactics:

* **Base resistances are mandatory before DL30–40**: without rFire, a mature
  red dragon's 246-point breath is lethal to a 200-HP character; with it, it
  is 82. By DL50, an AMHD breathes 800 of five different elements — you need
  all five base resists and rPois (poison is not halved by armour and kills
  more characters than any other base element).
* **Double resistance**: a resist from equipment plus the matching temporary
  resist (potion, spell) reduces base elements to 1/9 (`adjust_dam()` in
  `project-player.c` applies the 1/3 factor once per resist level). Against
  great wyrms (1500–1600) double resistance turns a death into 177.
* **Approach sleeping dragons with stealth** and open with your biggest
  attack. Slay/kill dragon weapons (×3/×5) are excellent.
* **Death drakes and ethereal drakes/dragons are invisible and walk through
  walls**; dracoliches are undead (slay undead, nether immune).
* Great wyrms of chaos, law, balance, annihilation, many colours and sky
  dragons can **summon dragons and ancient dragons** (`S_DRAGON`,
  `S_HI_DRAGON`).
* Dragon pits (`pit.txt`: acid/electric/fire/ice/poison/multi-hued/gold
  dragons, rarity 6, average level 85) are great XP and loot if you have the matching resist —
  and death if you don't.

### `U` and `u` — Demons

`major demon` (`U`): `DEMON | EVIL | SPIRIT | ONLY_ITEM | NO_CONF | NO_SLEEP |
NO_HOLD | FORCE_SLEEP`. `minor demon` (`u`): `DEMON | EVIL | SPIRIT | IM_FIRE`.

Minor demons that matter:

* **imp** (DL17, +0, 27 HP) and **quasit** (DL16): `INVISIBLE`, `SMART`, cast
  `BLIND CONF SCARE TELE_LEVEL TELE_TO TPORT` at 1/10. Imps have poison hits
  and are immune to fire. They are a nuisance rather than a killer, but their
  `TELE_LEVEL` and `TELE_TO` can drop you somewhere bad.
* **homunculus** (DL15): paralysing hit — Free Action.
* **death quasit** (DL40, **+20**, 387 HP): invisible, `PASS_WALL`, summons
  demons, `WOUND` 1/10. Dangerous because you cannot see it and it is fast.

Major demons:

| Demon | DL | Spd | HP | Notes |
|---|--:|--:|--:|---|
| vrock | 40 | +0 | 352 | crushes 8d12 ×2; friends |
| nalfeshnee | 45 | +0 | 792 | POWERFUL fire breath 264, summons demons |
| lesser Balrog | 49 | +10 | 1760 | fire breath 586, fire hits 4d12 ×2, drain charges, summons demons |
| hezrou | 53 | −10 | 1850 | acid/fire breath & bolts, summons demons, friends |
| barbazu | 55 | +10 | 700 | CON drain, poison sting, summons |
| bile demon | 61 | +10 | 2464 | acid breath 821, poison breath 800 |
| osyluth | 65 | **+20** | 2288 | **invisible**, stat drains, ball spells, summons |
| gelugon | 69 | +20 | 3080 | cold breath 1026, shards 500, paralysing hit, `HOLD`/`SLOW`, S_HI_DEMON |
| horned reaper | 72 | +20 | 3344 | 11d11 ×4 melee, HASTE, S_HI_DEMON |
| pit fiend | 77 | +20 | 3520 | fire 1173, poison 800, chaos 586; S_HI_DEMON and S_HI_DRAGON |
| fury | 80 | +20 | 4000 | PASS_WALL, brain smash, TELE_SELF_TO |
| greater Balrog | 85 | +20 | 4500 | fire breath 1500, plasma 150, fire balls, whip, S_HI_DEMON |

Fire immunity is the single best defence against the `U` family (most of the
damage is fire); after that, confusion/blindness protection and speed.
Demons are `EVIL` and `DEMON` — *slay evil*, *slay demon*, *kill demon* and
the priestly/paladin anti-evil spells all work, and Protection from Evil
repels their melee (if your level is high enough relative to theirs).

### `G` — Ghosts

Base: `UNDEAD | EVIL | INVISIBLE | COLD_BLOOD | SPIRIT | PASS_WALL | IM_COLD |
IM_POIS | NO_CONF | NO_SLEEP | NO_STUN | NO_HOLD`. Every ghost is invisible
and walks through walls, so the counter is always the same: **See Invisible
(or telepathy — ghosts are not EMPTY_MIND)**, then slay undead or light/holy
attacks.

| Ghost | DL | Spd | HP | What it does |
|---|--:|--:|--:|---|
| poltergeist | 3 | **+20** | 6 | terrify touch, blink; harmless but disconcerting |
| green glutton ghost | 5 | +20 | 8 | eats your food |
| lost soul | 7 | +0 | 9 | WIS drain touch, drain mana, teleport |
| moaning spirit | 12 | +10 | 23 | DEX drain, SCARE, teleports away |
| banshee | 24 | +10 | 27 | XP drain touch, drain mana |
| ghost | 31 | +10 | 59 | XP/INT/WIS drain, blind, **hold** |
| shade, spectre | 34, 37 | +10 | 147, 167 | XP drain, blind, hold, WIS/INT drain |
| spirit troll | 40 | +0 | 880 | big invisible regenerating melee troll-ghost |
| phantom | 40 | +10 | 260 | XP drain 80 touches, FORGET |
| **dread** | 44 | +10 | 263 | friends 2d5; 6d6 ×2 hits + STR drain; BLIND CONF HOLD DRAIN_MANA, nether bolt ~111 |
| shadow | 50 | +10 | 175 | XP drain, SLOW, TELE_TO, nether bolt |
| dreadmaster | 54 | +10 | 1056 | adds WOUND, TELE_LEVEL, S_UNDEAD |
| **dreadlord** | 62 | +10 | 2640 | friends; 1/4: BRAIN_SMASH, nether ball 248+10d10, HOLD, TELE_TO, S_UNDEAD |

Dreads are the iconic "you never saw it coming" killer of the DL40s: a pack
of up to 11 invisible, wall-walking, paralysing nether casters. With Free
Action, See Invisible and rNether they are manageable XP; without any one of
them, leave.

### `W` — Wraiths and wights; `V` — Vampires; `L` — Liches; `z`, `s`

All undead: slay undead, light (most `W`, all `V` are `HURT_LIGHT`), and
**Hold Life** against their experience drains.

* **Wights** (`W`, DL24–37): forest/grave/barrow/emperor wight. XP-draining
  touches, `SCARE`, `HOLD` from barrow wights on. Emperor wight D37 has
  EXP_80 touches and nether bolts.
* **Wraiths** (`W`, DL38–73): white D38 +10 450, grey D45 650, black D49 840,
  **nether wraith** D54 (invisible, PASS_WALL, mind blast, nether bolt and
  nether *lance* `BE_NETH`), nightwing D61 (brain smash, nether ball, mana
  bolt, S_UNDEAD), nightwalker D73 **+20** 4 disenchant hits up to 10d10.
* **Vampires** (`V`, DL27–59): vampire D27, master vampire D36, vampire lord
  D46 +10 1400 (brain smash, drain mana), elder vampire D59 (invisible,
  PASS_WALL). All `REGENERATE`, `HURT_LIGHT`, `HURT_FIRE`; bites drain XP;
  `HOLD` and `MIND_BLAST` mean you need Free Action and a good saving throw.
* **Liches** (`L`, DL34–74): lich D34 (TELE_AWAY, TELE_TO, brain smash, hold,
  drain mana, slow), master lich D41 +10 1584, demilich D54 2816, archlich
  D64 3520 (PASS_WALL, nether ball, S_HI_UNDEAD), black reaver D74 (KILL_WALL,
  mana ball). Liches touch to drain charges and DEX. They are `SMART`: they
  will learn your resistances and use the spells you can't stop. Fight them
  with full status protection or not at all.
* **Zombies and mummies** (`z`, DL7–38): slow, stupid, `EMPTY_MIND`. The
  dangerous ones are **ghouls** (DL26, friends 4d4, paralysing claws + HOLD
  spell) and **ghasts** (DL30, +10, paralysis + CON drain) — Free Action
  is non-negotiable from DL25. Mummified chieftain D38 is a full caster
  (forget, mind blast, S_UNDEAD).
* **Skeletons** (`s`): harmless until the **druj** — hand druj D57, eye druj
  D66, skull druj D74 — `NEVER_MOVE`, +20 speed, cast **every turn**
  (`spell-freq:1`): BRAIN_SMASH, nether bolts/balls, mana bolts,
  TELE_AWAY, FORGET, S_UNDEAD. Stay out of their line of sight.

### `e` — Floating eyes and beholders

Eyes `NEVER_MOVE` except spectator (D28), gauth (D36) and the beholders.

* **floating eye** (DL1): gaze paralyses. It never moves and only gazes at an
  adjacent player — never melee it without Free Action; shoot it or ignore
  it.
* **disenchanter eye** (DL5, −10): gaze disenchants; drain mana.
* **evil eye** (DL18): XP-draining gaze, `HOLD`, `TELE_TO` (it pulls you next
  to itself to gaze at you).
* **spectator** (DL28): paralysis + confusion gazes, FORGET, HOLD, SLOW.
* **beholder** (DL40, +10, 1400 HP, spells 1/2): FORGET, MIND_BLAST,
  DRAIN_MANA, SLOW, CONF, BLIND, four elemental bolts; INT-draining gaze.
* **undead beholder** (DL45, 2376 HP, 1/2): BRAIN_SMASH, mana and nether
  bolts, cold/fire balls, S_UNDEAD.
* **beholder hive-mother** (DL67, 3080 HP, 1/2): paralysis gaze, BRAIN_SMASH,
  darkness/acid/cold/fire balls, S_KIN (more beholders).

### `p` and `h` — People and humanoids (the casters)

The `p` family is huge (39 races) and mostly spellcasters and archers. Note
that **4.2 has no dark elves**: the old dark elven priests and mages were
replaced by **dwarves** in `h` (blacklock mage, ironfist priest, eastern
dwarf, dark dwarven lord, stiffbeard sorcerer).

Early casters that kill low-level characters:

* **apprentice** (DL2), **acolyte** (DL2), **cutpurse** (DL2): weak but come
  in mixed groups (`friends` 20–80% of other persons).
* **blacklock mage** (`h`, DL10, +10, 39 HP, 1/5): BLIND, CONF, DARKNESS,
  magic missile, **stinking cloud** (BA_POIS). Fast and in groups of casters.
* **ironfist priest** (`h`, DL12, +10, SMART, 1/5): BLIND, CONF, WOUND, HEAL,
  DARKNESS.
* **illusionist** (`p`, DL13, +10, SMART, 1/3): BLIND, CONF, HOLD, SLOW,
  HASTE, TPORT.
* **druid** (DL13, 1/3): HOLD, SLOW, CONF, stinking cloud, fire bolt,
  S_ANIMAL, SHAPECHANGE.
* **dark dwarven lord** (`h`, DL20, +10): HASTE, BLIND, CONF, fire/frost bolt.

Mid-game threats:

* **mind flayer** (`h`, DL28, 1/8): **BRAIN_SMASH** (12d15 plus slow,
  confusion, paralysis and blindness on a failed save), MIND_BLAST (8d8 +
  confusion), FORGET, HOLD, SCARE, INT/WIS-draining gazes. A good saving
  throw and Free Action/pConf are the defence.
* **mage** (DL28), **sorcerer** (DL40, +20, 1/2): TELE_AWAY, TELE_TO, traps,
  balls, summon dragons/undead/monsters. Sorcerers are among the most
  dangerous non-uniques of the DL40s.
* **necromancer** (DL25), **dúnadan of Angmar** (DL36), **lord of Carn Dûm**
  (DL46): nether bolts, HOLD, S_UNDEAD.
* **demonologist** (DL36, 1/2): HOLD, fire bolt, **S_DEMON** every other turn.
* **master thief** (DL34, +20): steals gold and items, then blinks away.
* **mystic / master mystic / grand master mystic** (DL33/50/57, invisible,
  +10/+20/+20): paralysing kicks (`HIT:PARALYZE:20d1`), summon animals,
  spiders and hounds.
* **southron archer** (DL39, +10, innate 1/2) and **ranger chieftain** (DL41,
  invisible, innate 1/2): heavy arrows from range.

### `o`, `O`, `T`, `P` — Orcs, ogres, trolls, giants

The "humanoid hordes": individually weak, dangerous in numbers. All are
slayable (`ORC`, `GIANT`, `TROLL`). Most orcs and many trolls are
`HURT_LIGHT`.

* **Orcs** (DL6–18): snaga, cave orc, orc tracker, orc shaman, orc archer
  (innate 1/3 arrows), half-orc, uruk (DL16, arrows and whip), orc captain.
  They come in groups of 2d6–2d9 plus escorts. Fight them in a corridor; an
  orc archer group in a room will shred a weak character.
* **Ogres** (DL13–32): ogre, black ogre, cave ogre (groups), ogre shaman
  (traps, fire bolt, HOLD, S_MONSTER), ogre mage (DL32, cold ball, HOLD,
  S_MONSTER), ogre chieftain.
* **Trolls** (DL17–42): forest troll (groups 4d4), stone troll (HURT_ROCK and
  HURT_LIGHT), cave troll, troll priest (blink, darkness, WOUND), hill troll, **olog** (DL36,
  +5, SMART, groups — not light-sensitive), mountain troll, **etten** (DL38,
  1320 HP), troll chieftain (DL40, +10). Trolls hit hard and in numbers;
  slay troll and a corridor turn them into the best XP of the DL30s.
* **Giants** (`P`, DL25–66): hill/frost/fire/stone/cloud giant (boulders,
  elemental fists, come with hounds of their element), cyclops D45 (boulders
  ~9d12 range), storm giant D49 (storms, lightning balls, TELE_TO), **lesser
  titan** D56 and **greater titan** D66 (confusing 9d9/12d12 hits ×4,
  TELE_TO, HEAL, S_MONSTERS). Confusion protection before meeting titans.

### `S` — Spiders, scorpions, ticks

Base: `BASH_DOOR | PASS_WEB`; most members `WEIRD_MIND` (only 1 in 10 shows
on ESP). Spiders can `WEAVE` webs that block you but not them.

* **cave spider** (DL2, **+10**, 7 HP, `GROUP_AI`, friends 2d8): a swarm of
  up to 17 double-speed biters is a top-five killer of level-1 characters.
  Fight them in a corridor or leave the level immediately.
* **giant white tick** (DL10, −10, poison 2d6), **giant tarantula** (DL15,
  +10, triple poison), **Mirkwood spider** (DL20, +10, groups, webs), **phase
  spider** (DL23, +10, groups, BLINK/TELE_TO/TELE_SELF_TO).
* **abyss spider** (DL38): HURT_LIGHT, dark and poison breath, webs, CONF,
  SLOW, BLIND.
* **ancient spider** (DL48, 1050 HP, SMART): WOUND, DRAIN_MANA, HOLD, dark
  breath, S_KIN and S_SPIDER.
* **spider of Gorgoroth** (DL63, +10, 4000 HP, friends): paralysing bite,
  STR sting, dark breath 500 and **poison breath 800**, S_KIN.

### `c`, `R`, `x` — the paralysers

Paralysis is the effect that most reliably turns a fight into a death,
because a paralysed character takes free hits until it ends; in 4.2 the
`PARALYZED` effect is `NONSTACKING` so it cannot be extended while active,
but a fresh paralysis can be applied the moment it ends. **Free Action**
(`FREE_ACT`) blocks it entirely (`fail:1:FREE_ACT` in `player_timed.txt`), as
does a successful saving throw against melee paralysis and the `HOLD` spell.

| Monster | DL | Spd | HP | Paralysis source |
|---|--:|--:|--:|---|
| floating eye | 1 | +0 | 11 | gaze (never moves) |
| homunculus | 15 | +0 | 36 | hit 1d2 |
| Old Man Willow (unique) | 19 | +0 | 1000 | 2 touches 1d14, `HOLD` 1/10, `TELE_TO` |
| carrion crawler | 25 | +0 | 130 | 2 stings 2d6; friends 2d4 |
| ghoul | 26 | +0 | 83 | 2 claws 1d4, `HOLD`; friends 4d4 |
| gorgimaera | 27 | +0 | 263 | gaze 2d6, fire breath |
| spectator | 28 | +0 | 171 | gaze 1d4, `HOLD` |
| basilisk | 28 | **+10** | 310 | gaze, poison breath 103 |
| ghast | 30 | +10 | 165 | 2 claws 2d4, `HOLD`; friends |
| trapper | 36 | +10 | 528 | 2 hits 15d1; invisible, looks like floor |
| killer iridescent beetle | 37 | +0 | 330 | gaze |
| greater basilisk | 45 | +10 | 1010 | 2 gazes 3d12, dark/nexus/poison breath |
| master mystic | 50 | +20 | 968 | kicks 20d1/15d1 |
| Shelob / Ungoliant / Tevildo / Wolf-Sauron | 55–99 | | | bites/claws |

Get Free Action before DL20 and never take it off.

### `j`, `m`, `,`, `i`, `$`, mimics — the stationary and the disguised

* **Jellies** (`j`, `EMPTY_MIND | STUPID`, most `NEVER_MOVE`): only hurt you
  if you touch them, but some *drain mana* or *eat light* at range (silver
  jelly D3 `DRAIN_MANA` 1/15). **ochre jelly** (D13, +10) and **gelatinous
  cube** (D16, 316 HP, immune to acid/cold/elec/fire/poison, three acid
  touches 1d10, `TAKE_ITEM`, `DROP_4`) *move*. The cube is a tank that eats
  floor items; kill it with a physical weapon or avoid. **black ooze** (D23,
  −20) breeds.
* **Molds** (`m`, `NEVER_MOVE`, `HURT_FIRE`, `IM_POIS`, `EMPTY_MIND`): touch
  effects only. **disenchanter mold** (D10) disenchants. **death mold**
  (D47, **+30**, 1050 HP, four disenchant/EXP_80 spores up to 7d7): never
  melee it; never stand next to it.
* **Mushrooms** (`,`): **shrieker** (D2) and **shambling mound** (D18) `SHRIEK`,
  which wakes the level *and hastes monsters in line of sight*
  (`SHRIEK` = `WAKE` + `PROJECT_LOS:MON_SPEED` in `monster_spell.txt`). Kill
  shriekers on sight. **magic mushroom patch** (D15, +30): confusion and
  hallucination spores, blinks around. **memory moss** (D32): FORGET 1/6.
* **Creeping coins** (`$`, `MIMIC_INV`, `UNAWARE`): look like gold; poison
  touches; drop gold (`ONLY_GOLD`). Creeping adamantite coins (D27, +10, 260
  HP) can hurt.
* **Mimics** (`!` potion D18, `?` scroll D21, `=` ring D29, `~` chest D35):
  `NEVER_MOVE`, disguised as an object until you approach; they cast bolts,
  BLIND/CONF/SCARE and **S_MONSTER**. A suspicious lone object in a corridor
  is worth a *Detect Monsters*.
* **Lurkers** (`x`): **lurker** (D14) and **trapper** (D36) look like floor,
  are invisible and never move; the trapper's paralysing hits make Free
  Action mandatory.

### `Q` — Quylthulgs

Base: `EMPTY_MIND | INVISIBLE | NEVER_BLOW | NEVER_MOVE | NO_FEAR | NO_CONF |
NO_SLEEP | NO_HOLD`. Pulsing flesh mounds that do nothing but cast — and
cast often.

| Quylthulg | DL | Spd | HP | Freq | Spells |
|---|--:|--:|--:|--|---|
| quylthulg | 20 | +0 | 27 | 1/4 | BLINK S_MONSTER |
| nexus quylthulg | 32 | +0 | 65 | **1/1** | BLINK **TELE_AWAY** |
| rotting / demonic / draconic quylthulg | 45 | +10 | 420 | 1/2 | BLINK TPORT S_UNDEAD / S_DEMON / S_DRAGON |
| greater demonic / draconic / rotting quylthulg | 71 | +10 | 1320 | 1/2 | BLINK TELE_TO S_HI_DEMON / S_HI_DRAGON / S_HI_UNDEAD |
| master quylthulg | 76 | +10 | 2640 | **1/1** | BLINK TELE_TO S_ANIMAL S_HI_DEMON S_HI_DRAGON S_HI_UNDEAD S_MONSTERS |

They are invisible **and** mindless, so neither ESP nor (often) your eyes find
them. The only signs are "something mumbles" and monsters appearing from
nowhere. Answers: See Invisible plus *Detect Monsters* (they are detected by
normal monster detection), then kill them with one big attack, or leave. Never
let a greater quylthulg get `TELE_TO` on you in the middle of its summons.

### `v` — Vortices

Base: `EMPTY_MIND | POWERFUL | RAND_50 | BASH_DOOR | NO_FEAR | NO_CONF |
NO_SLEEP | NO_HOLD | IM_POIS | NONLIVING`. They engulf for their element and
breathe it (innate 1/4). Their HP is low (45–420) so their breaths are weak,
except for the side effects:

* fire/cold/water (acid) vortex D21, energy vortex D29 — burn inventory.
* nexus vortex D37 — teleportation effects; **plasma vortex** D37 — stun;
  **shardstorm** D37 — cuts; **time vortex** D38 +20 — time breath 58 with
  stat/XP drain; **shimmering vortex** D38 **+30** — blinding light breath and
  `SHRIEK`; chaos vortex D53.
* **aether vortex** (D60, +20, 420 HP) breathes all 17 high and low elements.
* **storm of Unmagic** (D95, +30, 600 HP, KILL_WALL): chaos, disenchantment
  and time breath; engulfs to drain charges, disenchant, `EXP_80` and
  **LOSE_ALL** (all stats).

Kill vortices at range; they wander randomly (`RAND_50`) so they often waste
turns.

### `M` — Hydras

Base: `ANIMAL | ONLY_GOLD | BASH_DOOR | FORCE_SLEEP | HURT_COLD`. Each head is
a bite; heads add elements (poison, fire, acid, blind, confusion, XP drain)
and breaths from 6 heads on. All cast `SCARE`. 2-headed D17 → 9-headed D57
(+10, 1200 HP, POWERFUL, REGENERATE, fire/poison/acid/**nether** breath,
S_HYDRA, S_KIN). Cold attacks do double damage; protection from fear lets you
keep meleeing; good gold drops.

### `E` — Elementals and spirits

Base: `EMPTY_MIND | IM_POIS | NO_FEAR | NO_CONF | NO_SLEEP | NONLIVING`.

* **air spirit** D12 +20 invisible — erratic, harmless-ish, annoying.
* **earth spirit** D17 and **earth elemental** D34 — `PASS_WALL`,
  `HURT_ROCK` (Stone to Mud).
* **invisible stalker** (D35, **+20**, invisible, RAND_50): annoying more than
  deadly.
* **will o' the wisp** (D38, +20, invisible, PASS_WALL, SMART, spells 1/2:
  WOUND, CONF, BLINK, TPORT, hallucination hits).
* **magma** and **ice** elementals (D37, POWERFUL balls), **ooze elemental**
  (D36, acid balls).

### `C`, `f`, `q`, `Y`, `H`, `b`, `B`, `K`, `a`, `r`, `w`, `I`, `F`, `J`, `n`, `k`, `y`, `t`, `l`, `X`

The remaining families are mostly fodder at their native depth, with a few
standouts:

* **Canines** (`C`): **wild dog** (D1, friends 2d7), **wolf** (D10,
  +10, friends 2d10, GROUP_AI) and **warg** (D14) packs kill many early
  characters in open rooms; **wolf chieftain** (D26, SMART, darkness, wail);
  **hellhound** (D42, +10, 352 HP, packs, fire breath 117 each, IM_FIRE);
  **werewolf of Sauron** (D63, SMART, nether/dark breath 366, S_HOUND).
* **Felines** (`f`): fast clawing; **displacer beast** (D26) is invisible.
* **Quadrupeds** (`q`): **night mare** (D39, +10, XP-drain bite, confusing
  hit); mûmak (D35) packs.
* **Hybrids** (`H`): chimaera (fire breath), **gorgimaera** (paralysis
  gaze), manticore (D30, innate arrows 1/5), minotaur (D40, +20, 550 HP),
  **jabberwock** (D68, +20, 2816 HP, chaos breath 469, four 10d10 attacks).
* **Bats** (`b`): erratic and fast; **disenchanter bat** (D26, +20);
  **doombat** (D32, +20, three 5d4 fire bites); bat of Gorgoroth (dark/poison
  breath).
* **Birds** (`B`): crebain (D16, GROUP_AI, `SHRIEK`); **winged horror** (D48,
  +10, XP-drain bites, dark/nether/poison breath; accompanies the Witch-king
  50% of the time).
* **Beetles** (`K`) and **ants** (`a`): heavy melee; giant army ants (D30,
  +10, GROUP_AI, KILL_BODY) and giant fire ants (D35) come in 4d4 groups.
* **Rodents** (`r`), **worms** (`w`), **insects** (`I`): breeders (§5).
  Purple worm (D29, acid/poison) does not breed.
* **Snakes** (`J`): slow and armoured; king cobra/black mamba spit
  blindness/poison; **serpent of the brownlands** (D32, HOLD, BLIND, CONF);
  **nightcrawler** (D69, 2440 HP, nether breath 406, brain smash); **serpent
  of chaos** (D78, +20, chaos breath).
* **Nagas** (`n`): slow tanks; **spirit naga** (D28, invisible, mind blast,
  heal); **gorgon** (D48, +10, XP gaze, paralysis gaze, HOLD, S_HYDRA).
* **Kobolds** (`k`): kobold archer (D4, arrows 1/2) and kobold shaman (D3,
  magic missile) are the first ranged threats a new character meets.
* **Townsfolk** (`t`): steal gold/items; harmless otherwise.
* **Trees** (`l`): **old forest tree** (D11, 800 HP, never moves, CONF);
  **huorn** (D31) and **black-hearted huorn** (D42) TELE_TO you into their
  4-crush embrace; **hasty ent** (D52, KILL_WALL). All `HURT_FIRE`.
* **Xorns** (`X`): umber hulk (D16, KILL_WALL, confusing gaze), xorn (D36)
  and xaren (D40) PASS_WALL; all `HURT_ROCK`.

### `A` — Ainur (Maiar)

Base: `POWERFUL | SPIRIT | SMART | ONLY_ITEM | IM_COLD | IM_FIRE | IM_POIS |
NO_CONF | NO_SLEEP | NO_HOLD | FORCE_SLEEP`. Not evil (holy attacks and slay
evil do not help).

| Maia | DL | Spd | HP | Notes |
|---|--:|--:|--:|---|
| Maia | 34 | +0 | 528 | BLIND CONF HASTE HEAL SCARE |
| Maia of Nienna / of Mandos | 43 | +10 | 1250 | invisible; HOLD (Mandos), mana bolt (Nienna) |
| Maia of Oromë | 52 | +10 | 2000 | arrows, S_HOUND |
| Maia of Yavanna / of Aulë | 60 | +10 | 3500 | S_AINU / boulders, PASS_WALL, SHATTER hit 10d10 |
| Maia of Ulmo | 65 | +10 | 4700 | paralysis and acid engulf, water balls/bolts, S_AINU |
| Maia of Manwë / of Varda | 71 | **+20** | 5000 | storms, lightning, S_AINU; paralysis touch (Varda) |

All shapechange. They hit hard and their status attacks make pBlind,
pFear and Free Action essential.

---

## 3. The killers list, by depth

These are the non-unique monsters (and a few uniques, marked ★, that are
covered in full in §6) that kill characters who are *at a normal depth for
their level*. Each entry says why it kills and what to do. All stats from
`monster.txt`; "breath" is at full HP.

### DL1–10: the new character

| Monster | DL | Spd | HP | Why it kills | Counter |
|---|--:|--:|--:|---|---|
| ★ Grip / Fang, Farmer Maggot's Dog | 2 | +10 | 25 | double speed at DL2, erratic; you can't outrun them | fight in a corridor, wear armour; they only bite 1d4 |
| cave spider swarm | 2 | +10 | 7 ×(3–17) | `friends:100:2d8`, GROUP_AI, double speed | corridor at once; or stairs/Phase Door and leave the level |
| floating eye | 1 | +0 | 11 | paralysing gaze if you melee it | don't melee without Free Action; throw/shoot |
| kobold archer, kobold shaman | 4, 3 | +0 | 24, 11 | arrows 1/2, magic missile — first ranged damage | close the distance or break line of sight |
| wild dog / jackal-like packs | 1 | +0 | 3 | numbers in open rooms | corridor |
| ★ Bullroarer the Hobbit | 5 | +10 | 60 | double-speed melee 1d8×2 at DL5 | be clvl 10+, or Phase/Flee |
| shrieker mushroom | 2 | +0 | 1 | `SHRIEK` wakes the level and hastes what can see you | kill it first |
| ★ Wormtongue | 8 | +0 | 250 | frost bolt (max 50), stinking cloud, SLOW, traps, heals | ranged attacks, rPois helps |
| ★ Grishnákh, Lagduf, Golfimbul + orc escorts | 8–12 | +0 | 190–240 | a unique plus 10–20 orcs | corridor, flasks of oil, slay orc |
| blacklock mage | 10 | +10 | 39 | fast, BLIND/CONF, stinking cloud, in caster groups | CLW potions (cure blindness), kill first |
| wolf pack | 10 | +10 | 21 ×(3–21) | `friends:100:2d10`, double speed, GROUP_AI | corridor |
| radiation eye / disenchanter eye | 3 / 5 | | | stat drain / disenchant gaze | avoid melee |

The level-1 to level-10 character's universal escape kit is **Flasks of oil**
(a 1d4 throwing weapon that counts as lit, `THROWING | EXPLODE` in
`object.txt`), **Phase Door**, **Cure Light Wounds** (cures blindness),
and the habit of taking the stairs when a group appears in the open.

### DL10–20: the first "real" threats

| Monster | DL | Spd | HP | Why it kills | Counter |
|---|--:|--:|--:|---|---|
| ★ Orfax / Boldor (yeeks) | 10 / 13 | +10 | 120 / 180 | TELE_TO, CONF, SLOW, BLIND, summons | trivial melee — kill fast, they heal |
| ironfist priest, illusionist, druid | 12–13 | +0/+10 | 39–78 | BLIND, CONF, HOLD, SLOW, stinking cloud, S_ANIMAL | pConf/pBlind are not yet available — CLW/CSW potions; kill casters first |
| warg packs, orc archers | 13–14 | +10/+0 | 36/66 | numbers; arrows 1/3 | corridor |
| moaning spirit | 12 | +10 | 23 | invisible, PASS_WALL, DEX drain, SCARE | See Invisible is still rare: fight with your back to a wall, or leave |
| homunculus | 15 | +0 | 36 | paralysing hit | Free Action, or ranged |
| giant tarantula | 15 | +10 | 80 | 3 poison bites | CLW/CSW, rPois later |
| light/dark hounds; clear hounds | 15 | +0 | 21 | packs; blindness; invisible packs | corridor; CLW |
| fire/cold/energy hounds | 18 | +0 | 35 | packs burning your potions/scrolls | resists |
| gelatinous cube | 16 | +0 | 316 | big HP, immune to elements, three acid touches, eats floor items | don't fight it for loot; leave it |
| umber hulk | 16 | +0 | 110 | digs to you, confusing gaze | pConf; kill fast |
| ★ Ulfast, Nár, Shagrat, Gorbag, Bolg | 16–20 | | 340–500 | heavy melee (+ Nár's WOUND/MIND_BLAST/BLIND/CONF) | see §6 |
| imp, quasit | 16–17 | +0 | 27 | invisible, TELE_LEVEL/TELE_TO, BLIND/CONF | annoying; See Invisible |
| ★ Old Man Willow | 19 | +0 | 1000 | `TELE_TO` + two paralysing touches | **Free Action** or do not go near |
| hummerhorns | 18 | +10 | 3 | breeding confusers | kill the first one or leave |

### DL20–30: the Free Action wall

From DL20 the game stops forgiving missing protections. The biggest single
cause of death in this band is **paralysis**.

| Monster | DL | Spd | HP | Why it kills | Counter |
|---|--:|--:|--:|---|---|
| carrion crawler | 25 | +0 | 130 | two paralysing stings, friends 2d4 | **Free Action** |
| ghoul / ghast | 26 / 30 | +0 / +10 | 83 / 165 | paralysing claws + HOLD spell; groups 4d4 | **Free Action**, slay undead |
| basilisk | 28 | +10 | 310 | paralysis gaze, 3×2d12 bites, poison breath 103 | **Free Action**, rPois |
| mind flayer | 28 | +0 | 132 | BRAIN_SMASH (12d15 + slow/conf/paralysis/blind), MIND_BLAST, FORGET, HOLD | high saving throw, FA, pConf, pBlind |
| vampire | 27 | +0 | 163 | XP drain, MIND_BLAST, HOLD, TELE_TO | Hold Life, FA |
| young dragons | 29–32 | +0 | 237–281 | 79–93 breath of base elements | base resists |
| vibration / nexus hounds | 27 | +0 | 138 | stun / teleport + stat scramble | pStun or rSound; rNexus |
| phase spiders | 23 | +10 | 27 | groups that blink and teleport-to | area attacks |
| ★ the Mîm family, Sangahyando & Angamaitë, Azog, Uglúk | 22–27 | | 720–1100 | see §6 | |
| black ooze | 23 | −20 | 27 | breeds, drains mana | kill early or leave |
| giant fire tick, killer fire beetle | 26–27 | +0 | 72–99 | fire spit — burns scrolls/books | rFire |
| disenchanter bat | 26 | +20 | 27 | disenchants your gear | kill fast |
| abyss worm mass | 26 | −10 | 35 | invisible, wall-digging, XP-draining breeders | leave the level |

**Checklist before DL30**: Free Action, See Invisible (or ESP), rFire,
rCold, rElec, rAcid (at least most), some speed (+3 to +10), 200+ HP, and a
reliable escape (Teleportation scroll/staff).

### DL30–40: hounds, trolls and invisible things

| Monster | DL | Spd | HP | Why it kills | Counter |
|---|--:|--:|--:|---|---|
| **gravity hounds** | 35 | +0 | 193 | pack; blink + slow + stun every breath; no resist | pStun + FA + speed, corridor, or leave |
| **inertia hounds** | 35 | +0 | 193 | pack; −10 speed each breath; no resist | speed margin; corridor; leave |
| impact hounds | 35 | +0 | 193 | stun + knock-back | pStun |
| time vortex | 38 | +20 | 176 | time breath 58: stat or XP drain | kill at range, or avoid |
| shimmering vortex | 38 | +30 | 176 | blinding light breath + SHRIEK | pBlind or rLight |
| nexus quylthulg | 32 | +0 | 65 | TELE_AWAY every turn while you see it | find with See Inv + detection, kill in one blow |
| ghost, shade, spectre | 31–37 | +10 | 59–167 | invisible PASS_WALL XP/stat drainers, HOLD | SI, FA, Hold Life |
| sorcerer, mage, demonologist, dúnadan of Angmar | 28–40 | +0…+20 | 68–457 | TELE_TO / TELE_AWAY, summons, balls | kill first; anti-summon corridor |
| ogre mage, ogre shaman | 27–32 | | 163–220 | HOLD, traps, S_MONSTER | FA |
| trapper | 36 | +10 | 528 | invisible "floor" with paralysing hits | FA |
| troll packs (olog, hill/cave trolls, troll priests) | 33–40 | +0/+5 | 156–440 | numbers, heavy melee | slay troll, corridor, light |
| invisible stalker | 35 | +20 | 124 | fast, invisible | SI |
| chest/ring mimics | 29–35 | | 180–308 | bolts and S_MONSTER from an "item" | detect |
| mature dragons | 39–43 | +5 | 700–1000 | 233–333 breath | resists + 400 HP |
| will o' the wisp | 38 | +20 | 176 | invisible, PASS_WALL, WOUND/CONF 1/2 | SI, pConf |

### DL40–55: the second wall (high resists)

| Monster | DL | Spd | HP | Why it kills | Counter |
|---|--:|--:|--:|---|---|
| **dreads** | 44 | +10 | 263 ×(3–11) | invisible PASS_WALL packs; HOLD, BLIND, CONF, nether bolts ~111, STR drain | SI, FA, pBlind, pConf, rNether |
| beholder / undead beholder | 40 / 45 | +10 | 1400 / 2376 | spells 1/2: FORGET, MIND_BLAST, BRAIN_SMASH, DRAIN_MANA, bolts | saving throw, FA, pConf, pBlind |
| **greater basilisk** | 45 | +10 | 1010 | two paralysis gazes, poison 336 / nexus 168 / dark 168 breath | FA, rPois, rNexus |
| hellhound packs | 42 | +10 | 352 ×(3–11) | fire breath 117 each | rFire (double resist); corridor |
| master lich, demilich | 41 / 54 | +10 | 1584 / 2816 | BRAIN_SMASH, HOLD, TELE_TO, DRAIN_MANA, S_UNDEAD | saving throw, FA, kill quickly |
| lesser Balrog | 49 | +10 | 1760 | fire breath 586, fire melee | rFire+temp |
| ancient dragons, **AMHD** | 50–53 | +10 | 1500–2400 | 500–800 breaths | all base resists + rPois, 500+ HP |
| **death drake** | 50 | +10 | 1950 | invisible, PASS_WALL, nether breath 325 | SI, rNether |
| nether / time / plasma hounds | 51 | +10/+20/+10 | 330 | XP drain / **stat drain** / stun | rNether / leave / pStun |
| **Drolem** | 54 | +10 | 2200 | **poison breath 733**, BLIND, CONF, SLOW; armour 195 | rPois (+temp rPois), it is a DRAGON |
| dreadmaster | 54 | +10 | 1056 | TELE_LEVEL, S_UNDEAD, nether bolts | SI, rNether |
| nether wraith | 54 | +10 | 1000 | invisible, PASS_WALL, nether bolt + lance | SI, rNether |
| **dracolich** | 55 | +10 | 3080 | nether 513 + cold 1026 breath, XP bite | rNether, rCold+temp |
| dracolisk | 55 | +10 | 3080 | fire 1026 + nexus 400, paralysis gaze, HOLD | rFire+temp, rNexus, FA |
| death mold | 47 | +30 | 1050 | never moves, but four disenchant/XP spores | never step next to it |

### DL55–75: the deep

| Monster | DL | Spd | HP | Why it kills | Counter |
|---|--:|--:|--:|---|---|
| dreadlord | 62 | +10 | 2640 | 1/4: BRAIN_SMASH, nether ball 248+10d10, HOLD, TELE_TO, S_UNDEAD | saving throw, rNether |
| hand / eye / skull druj | 57/66/74 | +20 | 528–1540 | cast **every turn**: brain smash, nether/mana bolts, TELE_AWAY, FORGET | never stay in their LOS |
| spider of Gorgoroth | 63 | +10 | 4000 | poison breath 800, dark 500, paralysis | rPois, FA, rDark |
| chaos hounds | 64 | +10 | 930 | chaos 155 each in packs | rChaos |
| osyluth | 65 | +20 | 2288 | invisible, stat drains, balls, S_DEMON | SI |
| gelugon | 69 | +20 | 3080 | cold 1026, shards 500, paralysis, S_HI_DEMON | rCold+temp, rShards, FA |
| hound of Tindalos | 69 | +20 | 480 | invisible PASS_WALL packs of nether/disen/time breathers | SI, leave |
| jabberwock | 68 | +20 | 2816 | chaos 469, 10d10 ×4 | rChaos |
| great wyrms | 70–82 | +10 | 3960–5200 | 1400–1600 base breaths; summons | double resists, 700+ HP |
| greater titan | 66 | +10 | 3344 | 4 × 12d12 confusing hits, S_MONSTERS | pConf, ranged |
| nightwalker | 73 | +20 | 1400 | 4 disenchanting hits up to 10d10 | rDisen, ranged |
| aether hounds / aether vortex | 74 / 60 | +20 | 1230 / 420 | every element | full resists; leave |

### DL75–100: the end game

| Monster | DL | Spd | HP | Why it kills | Counter |
|---|--:|--:|--:|---|---|
| pit fiend | 77 | +20 | 3520 | fire 1173, poison 800, chaos 586; S_HI_DEMON/S_HI_DRAGON | full resists |
| fury | 80 | +20 | 4000 | PASS_WALL, BRAIN_SMASH, TELE_SELF_TO | saving throw |
| great wyrm of annihilation / sky dragon / balance / many colours | 80–82 | +10 | 5000–5200 | 500–1600 breaths of several elements + summons | everything |
| greater Balrog | 85 | +20 | 4500 | fire 1500, plasma 150, fire balls, S_HI_DEMON | rFire+temp, pStun |
| storm of Unmagic | 95 | +30 | 600 | chaos/disen/time breath; engulf drains everything | kill instantly or avoid |
| ★ the Nazgûl, Ancalagon, Carcharoth, Gothmog, Sauron, Morgoth | 77–100 | | | §6 | |

---

## 4. Monster spells

All 4.2.6 monster spells are defined in `monster_spell.txt` (formulas) and
`list-mon-spells.h` (types). `SPELL_POWER` defaults to the monster's level
(`r->spell_power = r->level` in `mon-init.c`) unless a `spell-power:` line
overrides it (Sauron 130, Cantoras 110, Wolf-Sauron 105, Serpent-Sauron 108,
Vampire-Sauron 130, Nan 85). Many spells have stronger descriptions above
`power-cutoff` values (e.g. "nether storms" above power 80).

### 4.1 The saving throw

Only these spells allow a saving throw (`message-save` lines):
**`BLIND`, `CONF`, `SCARE`, `SLOW`, `HOLD`, `FORGET`, `MIND_BLAST`,
`BRAIN_SMASH`, `WOUND`, `TELE_LEVEL`**. In `do_mon_spell()` (`mon-spell.c`)
the roll is `randint0(100) < skills[SKILL_SAVE]`; on a success the *whole*
spell (including the damage of WOUND/MIND_BLAST/BRAIN_SMASH) is avoided. So a
saving throw of 95% means 1 in 20 of these hit you. Separately, each status
effect is blocked by its protection even if the save fails: `BLIND` by
pBlind, `CONF` by pConf, `SCARE` by pFear, `SLOW` and `HOLD` by Free Action
(`player_timed.txt`), and `TELE_LEVEL` by **rNexus**. `FORGET` (amnesia — which stops you reading scrolls, including Teleportation
and Word of Recall) has no protection other than the save.

The saving throw also applies to *melee* status blows that go through
`melee_effect_timed()` with `save=true` (paralysis among them) and to the
stat-swap and teleport-level parts of nexus.

**No saving throw** applies to: all bolts, balls and breaths; `DRAIN_MANA`;
`TELE_TO`, `TELE_SELF_TO`, `TELE_AWAY`; `DARKNESS`; `TRAPS`; summons; `HEAL`,
`HASTE`, `BLINK`, `TPORT`.

### 4.2 Spell catalogue, by role

#### Damage: bolts, balls, beams (formulas, `P` = spell power)

| Spell | Element | Damage | At P=50 | At P=100 |
|---|---|---|---|---|
| `MISSILE` | missile | P/3 + 2d6 | 23 / 28 | 40 / 45 |
| `BO_ACID` | acid | P/3 + 7d8 | 47 / 72 | 64 / 89 |
| `BO_ELEC` | elec | P/3 + 4d8 | 34 / 48 | 51 / 65 |
| `BO_FIRE` | fire | P/3 + 9d8 | 56 / 88 | 73 / 105 |
| `BO_COLD` | cold | P/3 + 6d8 | 43 / 64 | 60 / 81 |
| `BO_POIS` | poison | P/3 + 9d8 | 56 / 88 | 73 / 105 |
| `BO_NETH` | nether | 3P/2 + 30 + 5d5 | 120 / 130 | 195 / 205 |
| `BO_WATE` | water | P + 10d10 | 105 / 150 | 155 / 200 |
| `BO_MANA` | mana | 50 + 1d(5P/2) | 113 / 175 | 176 / 300 |
| `BO_PLAS` | plasma | P + 10 + 8d7 | 92 / 116 | 142 / 166 |
| `BO_ICE` | ice | P + 6d6 | 71 / 86 | 121 / 136 |
| `BE_ELEC` | elec (short beam) | 2P + 30 + 5d5 | 145 / 155 | 245 / 255 |
| `BE_NETH` | nether (short beam) | 2P + 30 + 5d5 | 145 / 155 | 245 / 255 |
| `BA_ACID` | acid r2 | 15 + 1d(3P) | 90 / 165 | 165 / 315 |
| `BA_ELEC` | elec r2 | 8 + 1d(3P/2) | 46 / 83 | 84 / 158 |
| `BA_FIRE` | fire r2 | 10 + 1d(7P/2) | 98 / 185 | 186 / 360 |
| `BA_COLD` | cold r2 | 10 + 1d(3P/2) | 48 / 85 | 86 / 160 |
| `BA_POIS` | poison r2 | (P/2+3)d4 | 70 / 112 | 133 / 212 |
| `BA_SHAR` | shards r2 | 10 + 1d(3P/2) | 48 / 85 | 86 / 160 |
| `BA_WATE` | water r2 | 50 + 1d(5P/2) | 113 / 175 | 176 / 300 |
| `BA_LIGHT` | light r2 | 10 + 1d(3P/2) | 48 / 85 | 86 / 160 |
| `BA_DARK` | dark r2 | 4P + 10d10 | 255 / 300 | 455 / 500 |
| `BA_NETH` | nether r2 | 4P + 10d10 | 255 / 300 | 455 / 500 |
| `BA_MANA` | mana r2 | 5P + 10d10 | 305 / 350 | 555 / 600 |
| `BA_HOLY` | holy orb r2 | 10 + 1d(3P/2) | 48 / 85 | 86 / 160 |
| `STORM` | water+elec+ice r3 | 30+(P/3)d5, 20+(P/3)d5, 20+(P/3)d5 | three balls | three balls |
| `ARROW` / `BOLT` / `SHOT` | arrow | (P/8+1)d6 / d7 / d5 (hit chance vs AC) | 7d6 | 13d6 |
| `BOULDER` | arrow | (P/7+1)d12 | 8d12 | 15d12 |

The three to fear are **`BA_NETH`, `BA_DARK` and `BA_MANA`**: their base is
4–5× spell power, so a level-80+ caster's ball does 400–600 before
resistance. Nether and dark are halved-ish by resistance (6/(8+1d4), i.e.
between 1/2 and 2/3 of the damage); **mana cannot be resisted at all** (no
player element resistance exists for it). Ice uses your cold resistance
(`adjust_dam()` maps ICE to COLD) and also cuts and stuns.

#### Status spells

| Spell | Effect | Protection | Save? |
|---|---|---|---|
| `BLIND` | blind 11+1d4 | pBlind | yes |
| `CONF` | confused 3+1d4 | pConf | yes |
| `SCARE` | afraid 3+1d4 (no melee) | pFear | yes |
| `SLOW` | slow (−10) 3+1d4 | Free Action | yes |
| `HOLD` | paralysed 3+1d4 | Free Action | yes |
| `FORGET` | amnesia for 3 turns: **you cannot read scrolls**, spell failure becomes 50% + half the normal chance, device skill drops (`player-util.c`, `player-spell.c`, `player-calcs.c`) | — | yes |
| `MIND_BLAST` | 8d8 + confusion 3+1d4 | pConf for the conf part | yes |
| `BRAIN_SMASH` | 12d15 + slow, confusion, paralysis 3+1d4 each, blind 7+1d8 | FA, pConf, pBlind for the parts | yes |
| `WOUND` | (2P/3)d5 + cut (P/5−10)d10 — at P=100 that is 66d5 (avg 198, max 330) | — | yes |
| `DRAIN_MANA` | drains 1d(P/2+1) SP, heals the monster | — | no |
| `DARKNESS` | unlights the room + weak dark ball | rDark | no |
| `TRAPS` | creates traps around you | — | no |
| `SHRIEK` | wakes the level, hastes monsters in LOS | — | no |
| `WEAVE` | spins webs around | — | no |

`WOUND` deserves a second look: at high spell power it is the strongest
single "spell" many monsters have (Sauron's is 86d5, avg 258 / max 430), but
it is stopped completely by a successful save. This is why a high saving
throw matters so much in the late game.

#### Movement spells

| Spell | Effect |
|---|---|
| `BLINK` | monster teleports 10 |
| `TPORT` | monster teleports far (2×max sight + 5) — the "teleport-self" escape uniques use when hurt |
| `TELE_TO` | **teleports you next to the monster** |
| `TELE_SELF_TO` | monster teleports next to you |
| `TELE_AWAY` | teleports you away 100 |
| `TELE_LEVEL` | teleports you up or down a level (save; rNexus blocks it) |
| `HASTE` | monster +10 speed for 50 turns (`MON_TIMED_INC:FAST`) |
| `HEAL` / `HEAL_KIN` | heals 6×P HP to itself / to kin |
| `SHAPECHANGE` | becomes one of its `shape:` forms for 5+1d10 turns (Beorn, Sauron, Maiar, druids) |

`TELE_TO` is the dangerous one: it overrides your positioning (pulls you out of
your corridor into the room full of its friends, or next to Old Man Willow).
`TELE_AWAY` is usually a nuisance but can drop you into an unexplored area
mid-fight. On a quest level (DL99, DL100) `TELE_LEVEL` can only send you *up*
(`effect_handler_TELEPORT_LEVEL`).

#### Summons

| Spell | Summons (`summon.txt`) | Attempts | Includes uniques? |
|---|---|--:|---|
| `S_MONSTER` | any monster | 1 | no |
| `S_MONSTERS` | any monsters | 8 | no |
| `S_KIN` | same base as the caster | 8 | no |
| `S_ANIMAL` | `ANIMAL` | 12 | no |
| `S_SPIDER` | base spider | 12 | no |
| `S_HOUND` | bases zephyr hound and canine | 12 | no |
| `S_HYDRA` | base hydra | 6 | no |
| `S_AINU` | base ainu | 1 | no |
| `S_DEMON` / `S_UNDEAD` / `S_DRAGON` | flag DEMON / UNDEAD / DRAGON | 1 | no |
| `S_HI_DEMON` | base major demon | 8 | **yes** |
| `S_HI_UNDEAD` | bases vampire, wraith, lich | 8 | **yes** |
| `S_HI_DRAGON` | base ancient dragon | 8 | **yes** |
| `S_WRAITH` | unique wraiths (the Nazgûl); falls back to HI_UNDEAD | 8 | **yes** |
| `S_UNIQUE` | any unique; falls back to HI_UNDEAD | 8 | **yes** |

How many arrive (`effect_handler_SUMMON()` in `effect-handler-general.c`):
the summoner keeps calling `summon_specific()` until **the sum of the squares
of the summoned monsters' levels reaches `dungeon depth × summoner level`**,
or it runs out of attempts (the number in the table). Each summoned monster is
drawn at level `(depth + summoner level)/2 + 5` (`summon_specific()` in
`mon-summon.c`) and placed up to 4 grids from the summoner. Practical
consequences:

* One `S_HI_DEMON` from Gothmog (level 95) at DL95 has a "budget" of
  95×95 = 9025, i.e. one greater Balrog (85² = 7225) plus something, or a
  handful of lesser demons.
* `S_KIN` and `S_MONSTERS` produce many weak monsters, `S_HI_*` a few
  strong ones.
* **Summoned monsters arrive awake and join the summoner's group**, and they
  do not get a delay (`delay=false` for monster summons).
* **Summons need an empty floor grid within 2 squares of the summoner in its
  line of sight** (`summon_possible()` in `mon-attack.c`), and none can be
  placed on a **glyph of warding**. Smart monsters will not even try in a
  corridor where they are boxed in. This is the basis of the anti-summon
  corridor (§7.2).

### 4.3 Breaths

A breath's damage is `current HP / divisor`, capped (`breath_dam()` in
`mon-spell.c`, values in `projection.txt`):

| Element | Divisor | Cap | Player resist effect | Unresisted side effect (`project-player.c`) |
|---|--:|--:|---|---|
| acid, lightning, fire, cold | 3 | 1600 | ×1/3 per resist level (temp + perm = ×1/9); immune = 0 | inventory damage; acid halved by armour that takes damage |
| poison | 3 | 800 | ×1/3, ×1/9 | poisoning |
| light, dark | 6 | 500 | ×6/(8+1d4) | blindness (and amnesia from strong dark) |
| sound | 6 | 500 | ×6/(8+1d4) | stun 5+1d(dam/3) up to 35 unless pStun; confusion if dam > 300 |
| shards | 6 | 500 | ×6/(8+1d4) | cuts |
| nexus | 6 | 400 | ×6/(8+1d4) | stat scramble (save), teleport-to 1/3, teleport level 1/4 (save), else teleport 200 |
| nether | 6 | 600 | ×6/(8+1d4) | XP drain unless rNether or Hold Life; at power ≥ 80 also mana loss and **energy loss** |
| chaos | 6 | 600 | ×6/(8+1d4) | hallucination, confusion for 10–29 turns, XP drain (Hold Life stops the drain) |
| disenchantment | 6 | 500 | ×6/(8+1d4) | disenchants equipment |
| gravity | 3 | 200 | none | blink (if `randint1(127) > clvl`), slow, stun |
| inertia | 6 | 200 | none | slow 4+1d4 turns |
| force | 6 | 200 | none | stun 1d20, knock-back |
| time | 3 | 150 | none | XP drain (1/2) or drain 2 stats (2/5) or all stats (1/10) |
| plasma | 6 | 150 | none | stun 5+1d(3dam/4) unless pStun |
| water (balls) | — | — | none | confusion 5+1d5, stun 1d40 (pConf / pStun protect) |
| mana | 3 | 1600 | **none** | — |

Remember: (1) breath is based on the monster's **current** HP — a dragon at
half health breathes for half; (2) `POWERFUL` breathers keep full strength
further from the source; (3) breaths, like balls, are not stopped by other
monsters in the way, so standing behind the breather's friends does not help.

---

## 5. Breeders

`MULTIPLY` monsters (23 races in 4.2.6, all non-unique):

| Breeder | DL | Spd | HP | Bite | Notes |
|---|--:|--:|--:|---|---|
| giant white mouse | 1 | +0 | 2 | 1d2 | the classic level-filler |
| white / green / yellow / clear / blue / red worm mass | 1–5 | −10 | 10–23 | poison / acid / DEX drain / poison / cold / fire | slow; `HURT_LIGHT` |
| giant white louse | 3 | +10 | 1 | 1d1 | insect, erratic |
| giant white rat, silver mouse | 4 | +0 | 2–3 | poison / eats light | |
| giant black louse, giant grey rat | 9 | +10 / +0 | 2–4 | | |
| clear mushroom patch | 10 | +10 | 1 | 1d1 | **invisible**, never moves |
| giant fruit fly | 10 | +10 | 3 | 1d2 | |
| nether worm mass | 12 | −10 | 23 | XP drain | |
| giant flea | 14 | +10 | 3 | 1d2 | |
| blue icky thing | 14 | −10 | 35 | poison, BLIND/CONF/SCARE 1/8 | |
| hummerhorn | 18 | +10 | 3 | **confusion** 2d2 | the most dangerous early breeder |
| neekerbreeker | 22 | +10 | 5 | poison, **SHRIEK** | wakes and hastes the level |
| black ooze | 23 | −20 | 27 | acid, DRAIN_MANA | `KILL_BODY` |
| abyss worm mass | 26 | −10 | 35 | XP drain | **invisible, KILL_WALL** |
| giant firefly | 26 | +10 | 5 | **blinding** bite | |
| disenchanter worm mass | 40 | −10 | 45 | disenchant | |

How breeding works (`monster_turn_multiply()` in `mon-move.c`):

* A breeder tries to multiply at the start of each of its turns, **before**
  moving or attacking — and only on turns when it is *active*, i.e. it can
  see, hear or smell you, or is hurt (`monster_check_active()`). A colony far
  from you does not grow.
* It counts the monsters in the 3×3 block around it (including itself), `k`.
  If `k ≥ 4` it does not breed; otherwise it succeeds with chance
  1/(k × `mult-rate`), with `mon-play:mult-rate:8` in `constants.txt` — so a
  lone breeder succeeds 1 time in 8, one with a neighbour 1 in 16, and so on.
  The child appears in an adjacent empty grid, awake.
* There is a hard cap: **no breeding once the level holds 100 breeders**
  (`mon-gen:repro-max:100`; `cave->num_repro` counts the MULTIPLY monsters
  currently on the level: incremented when one is placed, decremented when one
  is removed, in `mon-make.c`).
* Breeding costs the breeder its turn, so a crowded breeder that keeps
  trying does not attack.

Strategy:

* **Kill the first one immediately.** One giant louse is nothing; fifty fill
  the corridors in a few hundred game turns and cost you food, light and time.
* **If it has already spread, leave the level.** You gain very little XP from
  breeders (their `experience` values are tiny) and the confusion
  (hummerhorns), blindness (fireflies), XP drain (nether/abyss worms) and
  disenchantment (disenchanter worms) make farming them a losing game. There
  is no reward for clearing them.
* **Use the cap to your advantage only if you must stay** (e.g. a vault on the
  level): once 100 exist, no more appear; area attacks (balls, *Dispel*,
  Banishment of the symbol, a corridor and an earthquake/destruction) clean up.
* **Worm masses are slow** (−10) and `HURT_LIGHT`: a light beam down a
  corridor kills many at once.
* Breeders are a common trap for **players**, not because they kill directly
  but because a breeder-filled level is noisy, wakes everything and wastes your
  supplies.

---

## 6. The uniques

There are **101** entries with the `UNIQUE` flag in `monster.txt`: 96
fightable uniques, Father Christmas (`SEASONAL` — only generated 24–26
December, per `get_mon_num()` in `mon-make.c`) and four *shape-only* forms
(`rarity:0`): Beorn's bear form and Sauron's wolf, serpent and vampire forms.

### 6.1 How uniques differ from ordinary monsters

* **One of each, ever.** A unique is generated at most once at a time
  (`cur_num < max_num`) and never again after you kill it. If you leave a level
  with a unique on it, it can be generated again elsewhere — it is not lost,
  only "unseen".
* **Fixed HP, fixed speed.** No ±speed variance and no HP roll (§1.2, §1.3).
* **Status effects almost never stick.** A monster resists a timed effect
  (stun, confusion, fear, slow, hold, sleep) with chance
  `min(90, level + max(0, 25 − duration/2))`, and **uniques roll this twice**
  (`saving_throw()` in `mon-timed.c`). A level-40 unique resists a 10-turn
  confusion about 64% per roll, ~87% overall; anything level 65+ resists ~99%.
  On top of that most uniques of note have `NO_CONF`, `NO_SLEEP` and often
  `NO_HOLD`. Plan to kill uniques with damage, not with tricks.
* **Better drops.** See §1.6: object level is boosted to
  `min(level+15, 2×level)` and an extra "good" roll applies. `DROP_GREAT` on
  Sméagol, Wormtongue, Mîm, Ar-Pharazôn, Saruman, Glaurung, Feagwath,
  Maeglin, Cantoras, Ancalagon, the Witch-king, Vecna, Gothmog, Sauron and
  Morgoth.
* **They come with escorts.** `friends:` lines with role `servant` or
  `bodyguard` generate their gang with them (100% for most); some uniques
  bring *other uniques* (Golfimbul brings Bullroarer and Grishnákh; Azog brings
  Bolg; Uglúk brings Lugdush; Mîm brings Ibun and Khîm; Ancalagon brings the
  five other great dragons; Sauron brings Khamûl and the Witch-king; Gothmog
  brings the Balrog of Moria and Lungorthin; Meássë brings Makar).
* **Banishment and Mass Banishment skip uniques** (`effect_handler_BANISH`,
  `effect_handler_MASS_BANISH`). **Teleport Other works** on them, and
  **\*Destruction\* deletes** any monster in its radius, uniques and quest
  monsters included — they are removed, not killed (no drop, no XP) and can be
  generated again later.
* **Summoned uniques** (`S_UNIQUE`, `S_WRAITH`, and the `HI_*` summons that
  allow uniques) can bring a unique far out of its depth: a Nazgûl summoned at
  DL60 is a Nazgûl.

### 6.2 How to read the band tables

Each band below starts with a table giving **every** unique in it:

* **Melee** = average (maximum) damage of one full round of blows before
  armour, then the special blow effects. A +10 monster gets two such rounds
  per normal-speed turn of yours.
* **Spells** = `[1/N]` normal-spell frequency, `[inn 1/N]` innate frequency,
  then the spell list (see §4 and [Appendix A3](A3-monster-spells.md)).
* **Defences** = element immunities, then movement/vision quirks.
* **Escort** = races in its `friends:` lines.

Then the notable uniques get a paragraph of tactics.

