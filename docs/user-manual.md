# Angband — The Complete User Manual

*Covers the C version of Angband 4.2.6 (development line) as found on the
`c-borg-reference` branch of this repository, including the built-in automatic
player ("the Borg").*

Everything in this manual was written against the source tree and game data
files in this repository (`src/`, `lib/gamedata/`, `docs/`). The numeric tables
(races, classes, spells, stores, objects, traps, status effects…) were extracted
directly from `lib/gamedata/*.txt`, so they match the game exactly as built from
this branch. If you edit those data files, the in-game values change and these
tables will no longer be accurate.

---

## Table of contents

1. [What is Angband?](#1-what-is-angband)
2. [Installing, starting and saving](#2-installing-starting-and-saving)
3. [A five-minute first game](#3-a-five-minute-first-game)
4. [Creating a character](#4-creating-a-character)
5. [Races](#5-races)
6. [Classes](#6-classes)
7. [Stats, skills and derived values](#7-stats-skills-and-derived-values)
8. [The main screen](#8-the-main-screen)
9. [Giving commands](#9-giving-commands)
10. [Command reference](#10-command-reference)
11. [Targeting, looking and pathfinding](#11-targeting-looking-and-pathfinding)
12. [The town and its stores](#12-the-town-and-its-stores)
13. [The dungeon](#13-the-dungeon)
14. [Objects and equipment](#14-objects-and-equipment)
15. [Identification, runes and curses](#15-identification-runes-and-curses)
16. [Inscriptions and ignoring](#16-inscriptions-and-ignoring)
17. [Combat](#17-combat)
18. [Elements, resistances and status effects](#18-elements-resistances-and-status-effects)
19. [Magic and spellcasting](#19-magic-and-spellcasting)
20. [Complete spell lists by class](#20-complete-spell-lists-by-class)
21. [Shapechanges](#21-shapechanges)
22. [Monsters](#22-monsters)
23. [Options](#23-options)
24. [Customising the game](#24-customising-the-game)
25. [Front ends (Windows, X11, SDL, SDL2, macOS, curses)](#25-front-ends)
26. [Death, retirement, winning and the high-score list](#26-death-retirement-winning-and-the-high-score-list)
27. [Strategy guide](#27-strategy-guide)
28. [The Borg — Angband's automatic player](#28-the-borg--angbands-automatic-player)
29. [Debug (wizard) mode](#29-debug-wizard-mode)
30. [Frequently asked questions](#30-frequently-asked-questions)
- [Appendix A — Experience table](#appendix-a--experience-table)
- [Appendix B — Object kinds](#appendix-b--object-kinds)
- [Appendix C — Ego item names](#appendix-c--ego-item-names)
- [Appendix D — Object properties (runes)](#appendix-d--object-properties-runes)
- [Appendix E — Player abilities](#appendix-e--player-abilities)
- [Appendix F — Terrain](#appendix-f--terrain)
- [Appendix G — Game constants](#appendix-g--game-constants)
- [Appendix H — Files and directories](#appendix-h--files-and-directories)

---

## 1. What is Angband?

Angband is a single-player "roguelike" dungeon adventure. You create a
character, choosing a race and a class, and play that character over days,
weeks or months. The world is drawn with text characters (or optional
graphical tiles): walls, floors, items and monsters are all symbols on a grid,
and your character is the `@`.

You begin in a small **town** where you can buy food, light, armour, weapons,
potions, scrolls, magic devices and spell books. Below the town lie the **Pits
of Angband**: one hundred (and more) levels of randomly generated dungeon, each
50 feet deeper than the last. Levels are generated fresh every time you enter
them, so the dungeon is effectively infinite. You go down, kill monsters, gain
experience and find treasure, and return to town from time to time to restock.

**Your goal** is to descend to level 99 (4950 ft), kill **Sauron, the
Sorcerer**, take the stairs that then appear down to level 100 (5000 ft), and
kill **Morgoth, Lord of Darkness**. Do that and you have won. Everything else
is optional.

Angband has **permadeath**: when your character dies, it is gone. There is no
"reload". The game saves automatically on every new level and when you quit.

Angband is old (descended from *Moria*, 1983, via *Angband* 1990) and very
deep. Do not expect to understand everything at once. The in-game help (`?`)
and the forum at <https://angband.live/forums/> are the main places to get
help.

### About this version

This branch is the C code of Angband **4.2.6** plus later development work
(the README says "Angband 4.2.6"; `changes.txt` holds the full changelog). Notable
features of this line, all described in this manual:

- nine classes (Warrior, Mage, Druid, Priest, Necromancer, Paladin, Rogue,
  Ranger, Blackguard) in four magic realms;
- the **rune-based identification** system ("identify by use");
- **autoexplore commands** (an option): `p` walks to the nearest unexplored
  place, and `<` / `>` walk to the nearest known staircase;
- **pathfinding** by mouse click or from the look/target interface, which opens
  doors and clears rubble along the way;
- device activation that retries automatically on failure, like digging;
- the **Borg**, an automatic player, compiled in by default (`Ctrl-Z`).

---

## 2. Installing, starting and saving

### Getting the game

Official builds for Windows, macOS and Linux are at
<https://angband.github.io/angband/>. To build this repository yourself, see
`docs/hacking/compiling.rst` (Make/autotools, CMake, MSYS2, Visual Studio,
macOS Cocoa, Nintendo DS/3DS ports). The usual Linux build is:

```bash
./autogen.sh && ./configure --with-no-install && make
```

and then run `./angband` from the top of the source tree. CMake works too:

```bash
mkdir build && cd build && cmake .. && make
```

The Borg is built in when `ALLOW_BORG` is defined, which is the default on
most platforms (see [section 28](#28-the-borg--angbands-automatic-player)).

### Command-line options (Unix-like systems)

`angband [options] [-- subopts]`

| Option | Meaning |
|---|---|
| `-c` | Choose a savefile from a menu (overrides `-n`). |
| `-n` | Start a new character. **Overwrites the default savefile** unless used with `-u`. |
| `-l` | List all savefiles you can play. |
| `-w` | Resurrect a dead character (the savefile is marked as cheating). |
| `-g` | Request graphics mode. |
| `-u<who>` | Use the savefile for `<who>`. |
| `-d<dir>=<path>` | Override a game directory (`scores`, `gamedata`, `screens`, `help`, `pref`, `fonts`, `tiles`, `sounds`, `icons`, `user`, `save`, `panic`, `archive`). May be repeated. |
| `-s<mod>` | Use a particular sound module. |
| `-m<sys>` | Use front-end module `<sys>`: `x11`, `sdl`, `sdl2`, `gcu` (curses, text terminal), plus the special modules `test`, `stats`, `spoil` (and `ibm` on DOS builds). Which ones exist depends on how the game was compiled. |
| `-- <subopts>` | Options passed to the front end, e.g. `./angband -mx11 -- -n4` opens four X11 windows; `./angband -mgcu -- -h` lists curses options. |

On Windows and macOS you normally start the game by double-clicking it and
use the *File* menu to start a new character or open a savefile.

### Saving and quitting

- `Ctrl-S` saves without quitting. `Ctrl-X` saves and quits.
- The game also autosaves whenever you arrive on a new level.
- `Q` *retires* (kills off) your character — it cannot be undone.
- After death your savefile keeps only your options and birth choices; loading
  it starts a new character with the same settings ("quick restart").

### Where files are kept

| Platform | User directory (pref files, `lore.txt`, `borg.txt`, character dumps) |
|---|---|
| Linux / Unix | `~/.angband/Angband/` |
| macOS | `~/Documents/Angband/` |
| Windows | `lib/user/` inside the game folder |

Savefiles live under the user directory's `save` subdirectory (or
`lib/user/save`), the high-score file under `scores`, crash saves under
`panic`, and Borg logs/death saves under `archive`. See
[Appendix H](#appendix-h--files-and-directories).

---

## 3. A five-minute first game

This walkthrough assumes the default options (original keyset, no graphics)
and a numeric keypad. Whenever you see `-more-`, read the message and press
space. Escape cancels almost anything.

1. **Create a character.** On the birth screen press `a` three times: Human,
   Warrior, point-based stats. Look over the character sheet, then press
   Enter until you are placed in the town.
2. **Look around.** The `@` is you; `/` then `@` confirms it. Numbers `1`–`8`
   on the buildings are store entrances. `.` is floor, `:` is rubble (one kind
   can be walked through). Letters are monsters — most town folk are harmless,
   but give mercenaries and veterans a wide berth.
3. **`l`** (look) cycles through interesting things; `q` or Escape exits.
4. **`i`** shows your pack (food, a potion, torches, a scroll of Word of
   Recall…); **`e`** shows what you wear (soft armour, a dagger, a torch).
5. **Walk** with the number keys (`1`–`9`, `5` = stay). To enter a shop, walk
   onto its number. Inside, `p` buys, `d`/`s` sells or gives, `l` examines,
   Escape leaves.
6. **Go down.** Walk onto the `>` in town and press `>`.
7. **Fight** by walking into a monster. Use `R` then `&` to rest until healed.
8. **Come back** by reading Word of Recall (`r`), or walking up `<` stairs from
   level 1. In town, reading Recall takes you back to your maximum depth.

Then read [section 27](#27-strategy-guide) for real advice.

---

## 4. Creating a character

At the birth screen you choose, in order:

1. **Race** — see [section 5](#5-races).
2. **Class** — see [section 6](#6-classes).
3. **Stat allocation method** — *point-based* (recommended) or the *standard
   roller*.
4. **Name** (and optionally edit the background history).

You can also change the **birth options** here (`=` from the birth menu). They
can only be changed at character creation (or quick restart) and are listed in
[section 23](#23-options). As you move the cursor over races and classes their
stat bonuses and experience penalty are displayed.

### Attributes every character has

- **Race and class** — fixed for life.
- **Height, weight, age and background history** — random, flavour only. The
  history can be edited at birth.
- **Five stats** — Strength (STR), Intelligence (INT), Wisdom (WIS), Dexterity
  (DEX), Constitution (CON). Each ranges from 3 to 18, then into the
  "percentile" range 18/01 … 18/100 (the intrinsic maximum). Race, class and
  equipment bonuses can push the displayed value up to **18/220**. Potions found
  in the dungeon raise the base value permanently (to 18/100); some monster
  attacks and traps drain stats temporarily (restored on gaining a level or by
  rare potions); a *sustain* prevents draining.
- **Skills** — disarming (physical and magical), magic devices, saving throw,
  stealth, searching (perception), melee fighting, shooting, throwing and
  digging. They depend on race, class, level, stats and equipment.
- **Racial intrinsics** — infravision, resistances, sustains, special lores.
- **Experience points (XP) and level** — you start at level 1 with 0 XP. You
  gain XP by killing monsters, learning and first-casting spells, disarming
  traps and picking locks, and using/learning new items. The XP needed per level
  is multiplied by the race's experience factor (see the race table and
  [Appendix A](#appendix-a--experience-table)). Maximum level is 50.
- **Gold** — the base starting purse is 600 gold. With the *start kit* birth
  option (default on) you get some equipment and less gold; with point-based
  stats, unspent points become extra gold.
- **Armour Class (AC)** — shown as `[base,+bonus]` on armour.
- **Hit points (HP)** — see below.
- **Spell points (SP, mana)** — for spellcasters.
- **Speed** — normal is +0; see [section 17](#17-combat).

### Hit points

Your **hit die** is race hit die + class hit-die bonus. A Dwarf (11) Warrior
(+9) rolls 1d20 per level; a Hobbit (7) Mage (+0) rolls 1d7. You get the
maximum at level 1; later rolls are constrained so that the total at level 50
does not stray far from the average. Constitution adds a flat bonus per level,
recalculated back to level 1 whenever CON changes.

### Spell points

Maximum mana depends on class, level and the class's spell stat (INT for Mage,
Rogue, Necromancer and Blackguard; WIS for Priest, Paladin, Druid and Ranger).
It is recalculated back to level 1 when the stat changes. Warriors have none.
Mana regenerates over time and by resting; some items and spells restore it.

### Stat allocation

**Point-based (recommended).** You have a fixed pool of points to "buy" stat
increases; small increases are cheap, large ones expensive. The game pre-fills
a recommended allocation for your race/class. Any points you do not spend
become starting gold.

**Standard roller.** Rolls 3d6 for each stat. Press `r` to re-roll, or accept.

### Names and dynasties

The name is used for the savefile on some systems and on the high-score list.
If it ends in a Roman numeral ("Fred I", "Pimplesnarg XVI") the numeral is
incremented automatically each time a character dies or wins, so you can play a
dynasty.

## 5. Races

There are eleven races. The race fixes your base hit die, experience factor,
infravision, stat and skill adjustments, and any intrinsic powers.

**Human** — the baseline: no adjustments, no intrinsics, no infravision, and the
fastest levelling (100% XP factor).

**Half-Elf** — a little smarter and quicker than a human but less wise and
strong; slightly better at magic, disarming, saves, stealth, archery and
perception, slightly worse in melee. Dexterity is sustained. Weak infravision.

**Elf** — better magicians and archers than humans, worse in melee. Good at
perception, disarming and stealth. Resist light, sustained dexterity, fair
infravision.

**Hobbit** — superb at shooting, throwing, saves, disarming, perception and
stealth; weak in melee. Hold life (resists experience drain). Recognise
mushrooms on sight. Excellent burglars (rogues).

**Gnome** — excellent mages with very good saves; good at perception,
disarming and stealth; weak in melee. Free action (immune to paralysis).
Recognise wands, staffs and rods on sight, and are the best device users.

**Dwarf** — strong and tough, slow-witted and unsubtle. Good priests and
warriors (and paladins). Cannot be blinded; sense nearby buried ore (gold) in
walls; excellent diggers (+40 digging). Miserable stealth.

**Half-Orc** — good warriors and decent priests, poor at magic and at stealth,
perception and disarming. Resist darkness.

**Half-Troll** — enormously strong with the most hit points of any race; very
stupid and clumsy. Sustained strength and **regeneration** (heal fast, but eat
more). Great warriors, poor at almost everything else.

**Dúnadan** — hardy Men of the West, better than humans at everything,
especially constitution, which is sustained. No infravision.

**High-Elf** — the finest race in raw ability, with excellent device skill and
saves. See invisible and resist light. Their XP factor (145%) is the highest.

**Kobold** — small dog-headed humanoids: resist poison, good dexterity and
constitution, good stealth and disarming, but weak and not bright.

### Race tables

*Stats, hit die, experience factor and infravision:*

| Race | STR | INT | WIS | DEX | CON | Hit die | XP % | Infravision |
|---|---|---|---|---|---|---|---|---|
| Human | 0 | 0 | 0 | 0 | 0 | 10 | 100% | none |
| Half-Elf | 0 | +1 | -1 | +1 | -1 | 10 | 120% | 20 ft |
| Elf | -1 | +2 | -1 | +1 | -1 | 9 | 120% | 30 ft |
| Hobbit | -2 | +2 | +1 | +3 | +2 | 7 | 120% | 40 ft |
| Gnome | -1 | +2 | 0 | +2 | +1 | 8 | 120% | 40 ft |
| Dwarf | +2 | -3 | +2 | -2 | +2 | 11 | 120% | 50 ft |
| Half-Orc | +2 | -1 | 0 | 0 | +1 | 10 | 120% | 30 ft |
| Half-Troll | +4 | -4 | -2 | -4 | +3 | 12 | 120% | 30 ft |
| Dunadan | +1 | +2 | +2 | +2 | +3 | 10 | 120% | none |
| High-Elf | +1 | +3 | -1 | +3 | +1 | 10 | 145% | 40 ft |
| Kobold | -1 | -1 | 0 | +2 | +2 | 8 | 120% | 50 ft |

| Race | Disarm (phys) | Disarm (magic) | Device | Save | Stealth | Search | Melee | Shoot | Throw | Dig |
|---|---|---|---|---|---|---|---|---|---|---|
| Human | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| Half-Elf | +2 | +2 | +3 | +3 | +1 | +3 | -1 | +5 | +5 | 0 |
| Elf | +5 | +5 | +6 | +6 | +2 | +6 | -5 | +15 | +15 | 0 |
| Hobbit | +15 | +15 | +18 | +18 | +4 | +6 | -10 | +20 | +20 | 0 |
| Gnome | +10 | +10 | +22 | +12 | +3 | +4 | -8 | +12 | +12 | 0 |
| Dwarf | +2 | +2 | +9 | +9 | -1 | +2 | +15 | 0 | 0 | +40 |
| Half-Orc | -3 | -3 | -3 | -3 | -1 | -3 | +12 | -5 | -5 | 0 |
| Half-Troll | -5 | -5 | -8 | -8 | -2 | -9 | +20 | -10 | -10 | 0 |
| Dunadan | +4 | +4 | +5 | +5 | +1 | +3 | +15 | +10 | +10 | 0 |
| High-Elf | +4 | +4 | +20 | +20 | +2 | +10 | +10 | +25 | +25 | 0 |
| Kobold | +10 | +10 | +5 | 0 | +3 | +10 | -5 | +10 | +10 | 0 |

| Race | Age (base+d) | Height in. (base±) | Weight lb (base±) | Intrinsic abilities |
|---|---|---|---|---|
| Human | 14+1d6 | 69±10 | 165±35 | — |
| Half-Elf | 24+1d16 | 71±8 | 115±25 | sustain dexterity |
| Elf | 75+1d75 | 73±7 | 90±10 | sustain dexterity, resist light |
| Hobbit | 21+1d12 | 34±4 | 55±5 | hold life, Mushroom Lore (You easily recognize mushrooms) |
| Gnome | 50+1d40 | 40±5 | 80±5 | free action, Device Lore (You easily recognize magic devices) |
| Dwarf | 35+1d15 | 47±4 | 135±20 | protection from blindness, Miner (You can sense ore in the walls) |
| Half-Orc | 11+1d4 | 64±2 | 135±15 | resist dark |
| Half-Troll | 20+1d10 | 90±16 | 240±60 | sustain strength, regeneration |
| Dunadan | 50+1d20 | 80±6 | 190±20 | sustain constitution |
| High-Elf | 100+1d30 | 77±6 | 190±20 | see invisible, resist light |
| Kobold | 15+1d10 | 37±4 | 65±5 | resist pois |

Infravision lets you see warm-blooded monsters in the dark up to the listed
distance (10 ft per grid). It never improves with level, only with equipment.
Cold-blooded monsters are not seen by infravision. The **Dig** column is the
racial digging bonus.

---

## 6. Classes

The class is the character's profession. It adds stat adjustments and a
hit-die bonus, sets your skills and how they grow, how many melee blows you
can reach, and which (if any) magic you can learn.

**Warrior** — pure fighter, the simplest class to play. Prime stats STR, CON,
DEX. Gets more blows than anyone (up to 6), excellent at fighting, shooting and
throwing, poor at other skills, bad stealth. No spells. Becomes immune to fear
at level 30 and can bash with a shield.

**Mage** — lives by wits and magic (arcane realm, INT). Very weak in melee
(max 4 blows) and frail, but the best user of magic devices, with excellent
saves. Chooses which spells to learn, can reach 0% failure, and bolt spells
often become beams.

**Druid** — at one with nature (nature realm, WIS). Controls the environment
and creatures and can take animal forms; moderate skills; fights better when
transformed. Good saves. Druids are "extra persuasive" to monsters (Charms),
choose their own spells, can reach 0% failure, and prefer shaping events over
devices and missiles; at high level they gain potent nature magic.

**Priest** — holy devotion (divine realm, WIS). Prayers are granted by the
deity (you do not choose which prayer you learn next). **Blessed Fighter**:
improved melee with blessed or hafted weapons. Great saves, poor stealth, can
reach 0% failure.

**Necromancer** — master of spirits and shadow (shadow realm, INT). Has
**Unlight**: gains stealth in, can see in, and resists the dark, so normally
plays *without* a light source (and gets no benefit from magical lights). Is
**Evil**: resists nether but is hurt by holy attacks. Spells often harm or
endanger the caster, but become awesome at high level. Chooses spells and can
reach 0% failure.

**Paladin** — warrior/priest (divine realm, WIS). Second only to the warrior
in melee, poor with missiles, gains prayers slowly and cannot learn the most
powerful, and like priests does not choose prayers. Blessed Fighter and Shield
Bash. Poor stealth, perception and devices; decent saves.

**Rogue** — cunning thief (arcane realm, INT). Master of traps and locks,
best perception, high stealth, can **steal** from monsters (`s`). A few
spells (chosen by the player), always with some chance of failure.

**Ranger** — at home in nature (nature realm, WIS). Good fighter and the
**best archer**: shooting speed improves every 3 levels (Extra Shots). Good stealth, perception,
saves and devices.

**Blackguard** — unprincipled brawler (shadow realm, INT). Prefers heavy
weapons (minimum effective weapon weight 10 lb) and shields (Shield Bash);
learns a few destructive spells. **Combat Regeneration**: spell points are
gained when damaged by an enemy or when attacking in melee, and fade over time
instead of regenerating; spending or losing SP heals you, more so the more
hurt you are. Blackguards also have impaired hit-point recovery and suffer
*bloodlust* (see the status-effect table). Scorns other skills.

### Class tables

*Stat adjustments, hit-die bonus, blows and spellcasting basics:*

| Class | STR | INT | WIS | DEX | CON | Hit die bonus | Max blows | Min. weapon weight (lb) | Blows STR multiplier | Spell stat | First spell level | Armour weight limit for casting (lb) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Warrior | +3 | -2 | -2 | +2 | +2 | 9 | 6 | 3 | 5 | — | — | — |
| Mage | -3 | +3 | 0 | 0 | -2 | 0 | 4 | 4 | 2 | INT | 1 | 30 |
| Druid | -2 | 0 | +3 | -2 | 0 | 2 | 4 | 3.5 | 3 | WIS | 1 | 35 |
| Priest | -1 | -3 | +3 | -1 | +1 | 2 | 4 | 3.5 | 3 | WIS | 1 | 35 |
| Necromancer | -3 | +3 | 0 | 0 | -2 | 2 | 4 | 3.5 | 3 | INT | 1 | 30 |
| Paladin | +1 | -3 | +1 | -1 | +2 | 6 | 5 | 3 | 5 | WIS | 1 | 40 |
| Rogue | 0 | +1 | -3 | +3 | -1 | 4 | 5 | 2 | 4 | INT | 5 | 35 |
| Ranger | 0 | 0 | +2 | +1 | -1 | 5 | 5 | 3.5 | 4 | WIS | 3 | 40 |
| Blackguard | +2 | 0 | -3 | 0 | +2 | 8 | 5 | 10 | 5 | INT | 1 | 60 |

Skill values are *base (bonus per 10 character levels)*.

| Class | Disarm (phys) | Disarm (magic) | Device | Save | Stealth | Search | Melee | Shoot | Throw | Dig |
|---|---|---|---|---|---|---|---|---|---|---|
| Warrior | 25 (+15) | 20 (+10) | 18 (+7) | 18 (+10) | 0 (0) | 10 (+12) | 70 (+45) | 55 (+45) | 55 (+45) | 0 (0) |
| Mage | 30 (+10) | 35 (+12) | 36 (+13) | 30 (+9) | 2 (0) | 10 (+12) | 35 (+15) | 20 (+15) | 20 (+15) | 0 (0) |
| Druid | 30 (+10) | 30 (+10) | 24 (+10) | 30 (+10) | 3 (0) | 12 (+12) | 45 (+20) | 40 (+30) | 40 (+30) | 0 (0) |
| Priest | 25 (+12) | 25 (+12) | 30 (+10) | 32 (+12) | 2 (0) | 10 (+14) | 45 (+20) | 35 (+20) | 35 (+20) | 0 (0) |
| Necromancer | 30 (+10) | 35 (+12) | 36 (+13) | 30 (+9) | 2 (0) | 10 (+12) | 35 (+25) | 20 (+15) | 20 (+15) | 0 (0) |
| Paladin | 20 (+15) | 20 (+10) | 24 (+10) | 25 (+11) | 0 (0) | 10 (+12) | 65 (+40) | 50 (+30) | 50 (+30) | 0 (0) |
| Rogue | 45 (+20) | 45 (+20) | 32 (+10) | 28 (+10) | 3 (+1) | 20 (+16) | 35 (+45) | 66 (+30) | 72 (+45) | 0 (0) |
| Ranger | 40 (+15) | 30 (+10) | 28 (+10) | 32 (+10) | 3 (0) | 15 (+15) | 60 (+40) | 72 (+45) | 66 (+30) | 0 (0) |
| Blackguard | 20 (+15) | 20 (+10) | 24 (+10) | 18 (+10) | -1 (-1) | 8 (+10) | 65 (+40) | 35 (+15) | 40 (+30) | 0 (0) |

Notes on the columns:

- **Max blows** — the most melee blows per round the class can reach.
- **Min. weapon weight** — weapons lighter than this are treated as this heavy
  when computing blows.
- **Blows STR multiplier** — how strongly strength contributes to blows; higher
  means more blows from the same strength.
- **Spell stat** — the stat governing mana, number of spells and failure rates.
- **First spell level** — the character level at which the first spell can be
  learned.
- **Armour weight limit** — wearing more total armour weight than this
  reduces your mana.
- Skills: *base (bonus per 10 levels)*. To estimate a skill: race value + class
  base + (level ÷ 10) × class bonus, then stat and equipment modifiers.

Per-class abilities, starting kits and full spell lists are in
[section 20](#20-complete-spell-lists-by-class).

---

## 7. Stats, skills and derived values

### The five stats

**Strength** — melee to-hit and damage, carrying capacity (weight before you
are slowed), extra blows with heavier weapons, digging. The most important stat
early on for fighters.

**Intelligence** — spell stat for arcane and shadow casters (Mage, Rogue,
Necromancer, Blackguard): mana, number of spells learnable, failure rates. Also
helps with magic devices, picking locks and disarming magical traps.

**Wisdom** — spell stat for divine and nature casters (Priest, Paladin, Druid,
Ranger), and improves everyone's **saving throw**.

**Dexterity** — to-hit, extra blows with light weapons, AC, picking locks and
disarming physical traps, protection from thieves (high enough DEX makes thefts
always fail) and, for rogues, stealing.

**Constitution** — hit points per level and HP regeneration. Least important at
the start; the most important stat at the end.

The character sheet (`C`) shows each stat as *Self* (base), *RB* (race bonus),
*CB* (class bonus), *EB* (equipment bonus) and *Best* (the current value).
A drained stat is shown in lower-case/yellow with its undrained value.

### The skills

- **Infravision** — distance at which warm monsters are seen without light.
- **Fighting (melee)** — chance to hit in melee; rises with level, helped by
  STR and DEX. Inspecting a weapon (`I`) shows blows and damage per round.
- **Shooting** — chance to hit with launchers (does not affect range or damage).
- **Throwing** — chance to hit with thrown objects. Objects designed for
  throwing (daggers, spears, throwing axes…) are much better thrown.
- **Saving throw** — chance to resist monster spells such as blindness,
  confusion, paralysis, fear, cause-wounds. Can reach 100%.
- **Stealth** — how much noise you make; stealthy characters get first strike
  and can sneak past monsters.
- **Disarming** (physical / magical) — removing traps and picking locks. DEX
  helps physical, INT magical. Success gives a little XP.
- **Magic devices** — using wands, staffs, rods and activations; INT helps.
- **Searching / perception** — chance to notice traps etc.
- **Digging** — tunnelling speed (STR, weapon weight, digging tools).

On the character sheet skills are described by words (Very Bad … Superb,
Heroic, Legendary…), saving throw and disarm as percentages.

### Encumbrance

Your carrying limit depends on strength. Carrying more slows you down
(shown as negative speed); there is no hard upper limit. Being *Full* after
over-eating also slows you.

### Speed and energy

Every game turn each creature gains energy according to its speed; acting
costs energy (a normal move or attack costs 100). +10 speed is double normal
speed, −10 half speed, +20 about triple; beyond about +30 returns diminish.
See [section 17](#17-combat).

## 8. The main screen

The main window has four regions:

- **Message line** (top row) — the latest message. `-more-` means more
  messages are waiting; press a key (or turn on `auto_more`). `Ctrl-O` repeats
  the last message, `Ctrl-P` shows the message history.
- **Sidebar** (left by default) — race, title, class, level, experience
  (`Exp`, or `EXP` in a warning colour when drained), gold, the "equippy"
  characters (one symbol per worn item), the five stats, AC, HP, SP, the health
  bar of the tracked monster, speed, and depth (`Lev NN` and `NNNN ft`). The
  sidebar can be moved to the top of the screen or hidden with the options menu
  (`=`, `o` "Set sidebar mode": Left / Top / None).
- **Map** — the dungeon view. With `center_player` off the map scrolls by
  panels; `Ctrl-L` (`@` roguelike) re-centres it; `L` (`W`) lets you scroll
  around.
- **Status line** (bottom row) — temporary information, including:
  - `Rest ###` / `Rep. ###` — resting or repeating a command;
  - `Recall` / `Descent` — a pending Word of Recall or Deep Descent;
  - level feeling `LF:N-M` (danger–treasure; `?` until you have explored
    enough);
  - `Light N` — your current light radius; `Moves +N` — extra moves;
  - the remembered terrain/trap under you (e.g. lava, passable rubble, web);
  - `DTrap` — green when inside a trap-detected area, yellow on its edge;
  - `Study (N)` — you can learn N new spells;
  - every timed effect, using the labels in
    [section 18](#18-elements-resistances-and-status-effects) (e.g. `Haste`,
    `Blind`, `Poisoned`, `Hungry`, `RFire`…);
  - `Unignoring` — ignoring is temporarily switched off.

With the `hp_changes_color` option (on by default) your `@` changes colour as
your HP drops. HP and SP turn red below the `hitpoint_warn` percentage.

**Sub-windows** (on front ends that support them) can show inventory,
equipment, character sheet, map, messages, overhead map, monster recall,
object recall, monster list, item list and status — see
[section 24](#24-customising-the-game).

---

## 9. Giving commands

Every command is an *underlying command* plus optional arguments (count,
direction, item, spell…). Keys are mapped to commands by one of two
**keysets**:

- the **original keyset** (default): number keys/arrow keys move, letters are
  commands;
- the **roguelike keyset** (option `rogue_like_commands`): `hjklyubn` move,
  Shift+direction runs, Ctrl+direction alters (tunnels/opens/attacks), and
  several commands move to other keys.

### Directions

```
Original keyset            Roguelike keyset
    7  8  9                    y  k  u
    4  5  6                    h  .  l
    1  2  3                    b  j  n
```

At a "Direction?" prompt you can also press `t` or `5` to use the current
target, `'` to target the nearest monster, or `*` to choose a target.

### The Enter menu

Pressing **Enter** opens a menu of every command, grouped by type. It is the
easiest way to find a command you have forgotten. (To disable it, map Enter to
a single space in the keymap editor; see the FAQ.)

### Selecting items

When a command asks for an item, the list of valid items is shown:

- a lower-case letter selects that item immediately; an **upper-case** letter
  asks for confirmation first;
- `/` switches between inventory and equipment, `|` selects the quiver, `-`
  the floor; the right/left arrow keys rotate through equipment, inventory,
  quiver and floor;
- a digit `#` selects the first item inscribed `@#` or `@x#` (where `x` is the
  command key) — see [section 16](#16-inscriptions-and-ignoring);
- space or `*` lists choices; Escape cancels.

Items inscribed `!x` or `!*` ask for confirmation before use.

### Repeat counts

Type `0`, a number, then the command (in the original keyset, press space after
the count before a movement key). Counted commands stop when the count runs
out, on success, if you press a key, or if you are disturbed. With no count,
**open, tunnel, disarm, alter, close, aim a wand, use a staff, zap a rod and
activate** default to repeating up to 99 times (as does "use an item" with a
device or equipped item); everything else runs once. Failed device activations
retry automatically until they work or you are disturbed.

`n` (`Ctrl-V` roguelike) repeats the previous command.

### Keymaps and the bypass key

`\` before a key bypasses all keymaps (`\` `.` `6` always means "run east").
`^` followed by a key enters a control-key (`^` `p` = `Ctrl-P`), useful when
the OS intercepts control keys. Escape cancels prompts; `y`/`n` answer yes/no
prompts.

### Being disturbed

Running, resting, repeated commands and path-following stop when you are
**disturbed**. You are disturbed when, among other things:

- the command achieves its goal (wall dug, door opened, rest condition met);
- you press a key or mouse button (while resting this is checked every 128
  turns);
- a monster hits you, casts at you, or uses a ranged attack; a monster smashes
  a door;
- a visible monster in view moves, appears or disappears (if `disturb_near` is
  on — it is by default);
- you lose HP, make a melee attack, or are affected by a projection;
- a timed effect changes in a way that matters (e.g. going up a grade of
  cuts/stun/hunger, or an effect starting) — changes you caused yourself with a
  known spell or item do not disturb;
- you bump into impassable terrain;
- running or following a path reaches a visible object, monster, known trap,
  or the edge of a trap-detected area, or an obstacle the path cannot handle;
- you trigger a trap, find a secret door or a chest trap, faint from hunger,
  your pack overflows, a curse triggers, Recall/Deep Descent activates, your
  light runs low or out, an item inscribed `!!` (or any, with
  `notify_recharge`) recharges, autopickup scans a non-gold pile, you enter a
  store, or you arrive on a new level.

### Special keys on Unix

`Ctrl-C` interrupts the game; repeated interrupts warn you and the fifth quits
without saving (or kills the character on SETGID installs). `Ctrl-Z` suspends
the curses (`gcu`) version (`fg` resumes). Because the Borg command is also
`Ctrl-Z`, type `^` then `z` to reach it when your terminal intercepts
`Ctrl-Z`. `Ctrl-\`, `Ctrl-D` and `Ctrl-S` are not intercepted.

---

## 10. Command reference

### Key summary

| Command | Original | Roguelike |
|---|---|---|
| Aim a wand | `a` | `z` |
| Activate an object | `A` | `A` |
| Browse a book | `b` | `P` |
| Close a door | `c` | `c` |
| Character sheet | `C` | `C` |
| Drop an item | `d` | `d` |
| Disarm a trap/chest, or lock a door | `D` | `D` |
| Equipment list | `e` | `e` |
| Eat some food | `E` | `E` |
| Fire a missile | `f` | `t` |
| Fuel your light | `F` | `F` |
| Get (pick up) objects | `g` | `g` |
| Gain (study) a spell | `G` | `G` |
| Fire default ammo at nearest target | `h` | `Tab` |
| Inventory list | `i` | `i` |
| Inspect an item | `I` | `I` |
| Ignore an item | `k` | `Ctrl-D` |
| Toggle ignoring | `K` | `O` |
| Look around | `l` | `x` |
| Locate player on map / scroll map | `L` | `W` |
| Cast a spell | `m` | `m` |
| Full dungeon map | `M` | `M` |
| Repeat previous command | `n` | `Ctrl-V` |
| Open a door or chest | `o` | `o` |
| Explore: walk to nearest unknown grid (only with `autoexplore_commands`) | `p` | `p` |
| Quaff a potion | `q` | `q` |
| Retire character and quit | `Q` | `Q` |
| Read a scroll | `r` | `r` |
| Rest | `R` | `R` |
| Steal from a monster (Rogues) | `s` | `s` |
| View abilities | `S` | `S` |
| Take off equipment | `t` | `T` |
| Tunnel / dig | `T` | `Ctrl-T` |
| Use a staff | `u` | `Z` |
| Use any item | `U` | `X` |
| Throw an item | `v` | `v` |
| Version info | `V` | `V` |
| Wear / wield | `w` | `w` |
| Walk into a trap (jump) | `W` | `-` |
| Zap a rod | `z` | `a` |
| Walk (with pickup) | `;` | `;` |
| Run | `.` or Shift+dir | `,` or Shift+dir |
| Stay still (with pickup) | `,` or `5` | `.` or `5` |
| Go up staircase | `<` | `<` |
| Go down staircase | `>` | `>` |
| Alter grid (attack/tunnel/open/disarm as appropriate) | `+` or Ctrl+dir | `+` or Ctrl+dir |
| Target monster or location | `*` | `*` |
| Target closest monster | `'` | `'` |
| Inscribe an object | `{` | `{` |
| Uninscribe an object | `}` | `}` |
| Visible monster list | `[` | `[` |
| Visible item list | `]` | `]` |
| Quiver list | `\|` | `\|` |
| Identify a map symbol | `/` | `/` |
| Check knowledge | `~` | `~` |
| Take notes | `:` | `:` |
| Enter a user pref command | `"` | `"` |
| Options | `=` | `=` |
| Help | `?` | `?` |
| Save screen dump | `)` | `)` |
| Center map | `Ctrl-L` | `@` |
| Toggle inven/equip sub-window | `Ctrl-E` | `Ctrl-E` |
| Repeat level feeling | `Ctrl-F` | `Ctrl-F` |
| Do autopickup | `Ctrl-G` | `Ctrl-G` |
| Show previous message | `Ctrl-O` | `Ctrl-O` |
| Show message history | `Ctrl-P` | `Ctrl-P` |
| Redraw the screen | `Ctrl-R` | `Ctrl-R` |
| Save (don't quit) | `Ctrl-S` | `Ctrl-S` |
| Save and quit | `Ctrl-X` | `Ctrl-X` |
| Debug commands | `Ctrl-A` | `Ctrl-A` |
| Toggle wizard (debug) mode | `Ctrl-W` | `Ctrl-W` |
| Borg commands (if compiled in) | `Ctrl-Z` | `Ctrl-Z` |
| Bypass keymap | `\` | `\` |

Roguelike movement: `h` W, `j` S, `k` N, `l` E, `y` NW, `u` NE, `b` SW,
`n` SE; capitals run; Ctrl+letter alters in that direction.

### Inventory commands

**Inventory (`i`)** — your pack. You can carry **23** different stacks (a
stack holds up to 40 identical items). The space left before you are
over-weight, or how far over you are, is shown.

**Equipment (`e`)** — the 12 body slots: weapon, bow (launcher), two rings,
amulet, light, body armour, cloak, shield, helm, gloves, boots. You only gain
an item's powers by wearing it.

**Quiver (`|`)** — up to 10 slots. Ammunition goes here automatically. Every
40 missiles use up one pack slot. Throwing weapons inscribed for throwing
(e.g. `@v0`) also go in the quiver; every 8 of them (40 ÷ 5) use up a pack
slot.

**Drop (`d`)** — drops an item (optionally a quantity). If your grid cannot
hold objects (doors, stairs, traps) it lands nearby. Takes energy.

**Ignore (`k` / `Ctrl-D`)** — hides an item, and optionally all like it. See
[section 16](#16-inscriptions-and-ignoring). **`K` / `O`** toggles whether
ignored items are shown.

**Wear/wield (`w`)** — swaps the item into its slot; the old item goes to the
pack (or the floor if the pack is full). Takes energy.

**Take off (`t` / `T`)** — returns an item to the pack (or the floor if the
pack is full). Ordinary cursed items *can* be taken off in this version —
curses are separate runes with their own effects (section 15); only a very few
artifacts carry a *sticky* property that prevents removal. Also removes
ammunition from the quiver. Takes energy.

### Movement commands

**Move** — number keys/arrows (or `hjklyubn`). Moving into a monster attacks
it; into a closed door opens it; into a known trap tries to disarm it (unless
you are immune to traps). Ctrl+direction *alters* without moving.

**Walk into a trap (`W` / `-`)** — step onto a trap deliberately without
disarming.

**Run (`.`/`,` or Shift+direction)** — follows corridors until something
interesting happens or a choice of paths appears.

**Go up `<` / down `>`** — use a staircase you are standing on. Every level
except the town has at least one up staircase; every level has at least one
down staircase except quest levels before the quest monster is dead. Once you
leave a level you will never see it again (unless *persistent levels* is on),
and anything left there is lost. With **`autoexplore_commands`** on, pressing
`<` or `>` when *not* on the right kind of staircase (and not confused, with no
monster in view) walks you to the nearest known staircase of that kind.

**Explore (`p`)** — only with `autoexplore_commands` on, not confused and no
monster in view: walks to the nearest known passable grid next to unexplored
territory; if none, to the nearest grid next to a closed door or impassable
rubble that borders the unknown.

### Resting

**Stay still (`,` / `.` / `5`)** — pass one turn (and pick up, if you would).

**Get (`g`)** — pick up items; gold takes no time, each object 1/10 turn (max
one turn).

**Rest (`R`)** — then enter a number of turns, `*` (rest as needed: until HP
and SP are full) or `&` (rest until fully healed of everything or disturbed).
Better than standing still.

### Alter commands

**Tunnel (`T` / `Ctrl-T`)** — dig in a direction. Rock types, softest to
hardest: rubble (diggable by hand), magma, quartz, granite; permanent rock
cannot be dug. Veins with treasure look different. Your best digger in the pack
is used automatically. Repeats up to 99 times.

**Open (`o`)** — opens doors and chests, picking locks automatically (may take
several attempts). Opening a trapped chest springs the trap.

**Close (`c`)** — shut a door. Many monsters cannot open doors, and monsters
cannot see you through a closed door. Broken doors cannot be closed.

**Disarm (`D`)** — disarm a known trap (on the floor or on a chest) or **lock
a closed door**. Failure may set the trap off.

**Alter (`+`)** — performs the "obvious" action on an adjacent grid: attack,
tunnel, open, disarm.

**Steal (`s`)** — Rogues only. Works best when you are stealthy, faster than
the target and the target is asleep. Failure wakes it; a bad blunder makes it
shout for help.

### Spell commands

**Browse (`b` / `P`)** — list the spells in a book: level, mana, failure
chance, and whether known.

**Study (`G`)** — learn a new spell when `Study` shows on the status line.
Priests and Paladins get a random prayer from the book; others choose.

**Cast (`m`)** — needs the spell learned, the book in your pack, and you must
not be blind, confused or amnesiac; you need light unless you are a
Necromancer. Casting with insufficient mana is possible after confirmation but
risks paralysis/fainting and CON damage, with greatly increased failure.

### Object commands

**Eat (`E`)**, **Quaff (`q`)**, **Read (`r`)** (needs light and sight, not
confused/amnesiac; most scrolls that prompt can be cancelled without using
them up), **Fuel (`F`)** (refill a lantern from a flask of oil; torches cannot be
refuelled — carry spares; two flasks fill a lantern), **Inscribe (`{`)**, **Uninscribe (`}`)**.

The **hunger meter** on the status line shows `Fed` with a percentage, and
progresses Hungry → Weak → Faint → Starving. Fainting causes paralysis;
starving damages you. Over-eating makes you **Full**, which slows you.

### Magic device commands

**Aim a wand (`a` / `z`)** — needs a direction; bolts hit the first thing,
beams everything in line.

**Use a staff (`u` / `Z`)** — usually area effects.

**Zap a rod (`z` / `a`)** — rods never run out but need time to recharge; some
need a direction (always ask if the rod is unknown).

**Activate (`A`)** — use the special power of a worn item.

**Use any item (`U` / `X`)** — a generic "use" for any usable item.

All devices have a failure chance based on your device skill vs. the device's
level; failure retries automatically.

### Missile and throwing commands

**Fire (`f` / `t`)** — shoot ammunition from the quiver or pack with your
equipped launcher. Ammunition may break.

**Fire at nearest (`h` / `Tab`)** — fire the first suitable quiver ammunition
at the nearest visible, in-range target.

**Throw (`v`)** — throw any item. Range depends on strength and weight; most
items break or do little damage, but throwing weapons and **flasks of oil**
(fire damage) are effective.

### Looking commands

**Look (`l` / `x`)**, **Target (`*`)**, **Target closest (`'`)** — see
[section 11](#11-targeting-looking-and-pathfinding).

**Full map (`M`)** — the whole level, scaled down. **Locate (`L` / `W`)** —
scroll the map by sections.

**Inspect (`I`)** — full description of an item: known runes, damage per blow
and blows per round for weapons, damage per shot for ammunition, and how much
remains unknown.

**Monster list (`[`)** and **item list (`]`)** — everything you can see or
detect, with directions/distances, and whether monsters are asleep.

### Message commands

**Level feeling (`Ctrl-F`)**, **previous message (`Ctrl-O`)**, **message
history (`Ctrl-P`)**.

**Notes (`:`)** — write a note into the message log and character history.
Notes beginning `/say ` become `-- Name says: "…"`; `/me ` becomes
`-- Name …`; anything else becomes `-- Note: …`.

### Game status commands

**Character sheet (`C`)** — skills, current and potential stats, and more.
From it you can change your name, save a character dump to a file (includes
history, inventory, equipment and home), and switch to a second page showing
how each equipment slot and your innate abilities affect each resistance,
ability and modifier. In that view: `.` nothing/no effect, `?` unknown, `+`
known positive, `-` known negative, `!` positive timed effect, `=` negative
timed effect, `*` immunity, `~` both a positive and a negative. Label colours:
slate = rune unknown, white = no net effect, light blue = net positive, red =
net negative, green = elemental immunity.

**View abilities (`S`)** — your class/race abilities.

**Check knowledge (`~`)** — menus for:

- **objects** — every object kind you know; change ignore setting, display
  symbol/colour, or auto-inscription; view a summary for known flavours;
- **runes** — known object properties and their auto-inscriptions;
- **artifacts** — artifacts you have found/lost;
- **ego items** — ego types you have seen;
- **monsters** — monsters ever encountered (shared across characters), and
  whether uniques are still alive;
- **features** and **traps** — change their appearance by light level;
- **shapechange effects** — details of each shape;
- **stores and home** — contents as last seen;
- **hall of fame** — the score list;
- **character history** — notable events in this character's life;
- **equippable comparison** — a table of every wearable item you have access
  to (`e` equipped, `p` pack, `f` floor, `h` home, `s` store) with one column per
  property; `v` switches to a view of each item's effect on your key
  statistics, `c` cycles which locations are included, `?` shows further
  commands (filtering, comparing two items).

### Saving and exiting

**Save (`Ctrl-S`)**, **Save and quit (`Ctrl-X`)**, **Retire (`Q`)** — retiring
asks twice; afterwards the savefile can only start a new character (optionally
with the same birth options).

### Preferences and help

**Options (`=`)** — see [section 23](#23-options). **User pref command
(`"`)** — type one pref-file line directly. **Help (`?`)**. **Identify symbol
(`/`)** — what a map character means; for monster symbols you can then recall
all known monsters with that symbol (`k` sorts by kills, `p` by depth). At the
symbol prompt, `Ctrl-A` lists all monsters, `Ctrl-U` all uniques, `Ctrl-N` all
non-uniques. **Version (`V`)**.

### Extra commands

**`Ctrl-E`** toggles the inventory/equipment sub-window, **`Ctrl-R`** redraws,
**`)`** dumps the screen to an HTML file (plain HTML or forum-embedded HTML).

---

## 11. Targeting, looking and pathfinding

### Targeting prompts

When a command needs a target and you have one, it is used automatically if
`use_old_target` is on. Otherwise you are prompted: Escape (or the second mouse
button) cancels, `5`/`t` uses the current target, `'` targets the nearest
monster, a direction key fires in that direction, `*` or a first-button click
on the map opens the targeting interface.

Opening the targeting interface **clears** the current target — so `*` then
Escape is the way to forget a target.

### Interested vs. free targeting

The interface keeps a list of *interesting* locations. From `*` or a command
prompt it contains only monsters that a bolt/missile could reach; from look
(`l`/`x`) it also contains you, objects, traps and interesting terrain (stairs,
doors, rubble, shops). *Interested* mode jumps between those; *free* mode moves
the cursor anywhere.

| Key | Effect |
|---|---|
| direction | interested: jump to an interesting location roughly that way; free: move 1 grid (10 with a running key) |
| space, `+` / `-` | next / previous interesting location |
| `o` | switch to free mode, cursor stays |
| `p` | switch to free mode, cursor on the player |
| `m` | back to interested mode at the nearest interesting location |
| `<` / `>` | move cursor to the nearest down / up staircase (by travel time) |
| `x` | move cursor to the nearest grid bordering unexplored territory |
| `r` or click on cursor | toggle brief/detailed description |
| Enter | cycle through the things in the grid (monster, trap, object, terrain) |
| `t`, `5`, `0`, `.` | select target (monster in interested mode; any grid in free mode) |
| `g` | walk there by pathfinding (not from a command's target prompt) |
| `k` (`Ctrl-D`) | ignore settings for the object at the cursor (from look only) |
| `q`, Escape | leave without targeting |

Mouse: first button moves the cursor (clicking at the map edge scrolls);
second button exits (in free mode from a targeting prompt, clicking on the
cursor targets it); Ctrl+second button targets the clicked grid/monster;
Alt (Option on macOS)+second button pathfinds there.

When targeting, the projection path is drawn: **dark grey** unknown terrain
(and beyond), **red** a known monster that would stop a bolt, **yellow** a
known object, **blue** terrain that blocks, **white** clear. Targeting a grid
rather than a monster is useful to bend a bolt around an obstacle or to centre
a ball spell on a group or just around a corner.

Targeting a *location* is slightly dangerous: it persists even after you move
far away.

### Pathfinding

Pathfinding is used by `g` in the look/target interface, a first-button
click on the map (with `mouse_movement`), the second-button context menu, and
the autoexplore `<`, `>` and `p` commands. The path minimises the number of
turns, accounting for the time to open locked doors (assumed to be the hardest
locks) and dig through impassable rubble; unknown grids are assumed passable.
Paths are tried in order of preference:

1. known terrain only, avoiding known traps;
2. known terrain, possibly crossing known traps;
3. known or unknown terrain, avoiding known traps;
4. known or unknown terrain, possibly crossing traps.

Damaging terrain such as lava is never used. Floors revealed only by magic
mapping/enlightenment count as unknown. While following a path the game opens
doors and clears rubble *if all their neighbours are known*; otherwise it stops
just before them. Any keypress or disturbance stops you; `n` (`Ctrl-V`)
recomputes the path from where you are and continues.

## 12. The town and its stores

You start in the town: a walled area with eight buildings, some townspeople,
and a down staircase. The town has a day/night cycle (your first visit is in
daylight); at night it is dark and more townspeople roam. The surrounding wall
may contain lava.

### Townspeople

Filthy street urchins mob you (and multiply when excited); village and
blubbering idiots, beggars, lepers, wretches, singing drunks, aimless merchants,
scrawny cats and scruffy dogs are mostly harmless nuisances; squint-eyed rogues
try to steal your gold; mean-looking mercenaries and battle-scarred veterans
can actually hurt a new character. Farmer Maggot wanders about harmlessly.
Townspeople give **no experience**, though they may drop gold or items. Avoid
fights you don't need.

### Using a store

Walk onto a shop's number to enter. You see the owner's name and race, the
store name, the most the owner will pay for one item, and the stock with
prices. Inside a store:

| Key | Action |
|---|---|
| `p` or `g` | Purchase (in the home: take) an item |
| `d` or `s` | Sell — or, with `birth_no_selling`, *give* — an item (in the home: drop/stash) |
| `l` (`x` roguelike) | Examine an item in the store |
| `I` | Inspect an item in your pack |
| `?` | Help |
| `=` | Options |
| Escape | Leave |

Many normal commands also work in stores: `w`ear, `t`/`T` take off, `k`
ignore, `b`/`P` browse, `~` knowledge, `e`/`i`/`|` lists, `{` `}` inscribe,
`C` character sheet, `)` screen dump. Clicking or pressing a stock letter opens
a context menu (buy, buy one, examine…).

- **Selling is off by default** (`birth_no_selling`): stores give you nothing
  for items, but gold found in the dungeon is increased to compensate. You can
  still *give* items to a store to have them **identified** (and learn their
  flavour or runes); some wands and staffs are recharged when given.
- Stores refuse items they know to be worthless or harmful. Unknown items are
  accepted, identified, and then kept (if good) or thrown away.
- Items in stores whose flavour you have not learned are labelled `{unseen}`.
- **Restocking**: every 10,000 game turns spent in the *dungeon* counts as a
  store "day"; the changes are applied when you return to town. Stock never
  changes while you stay in town (even across save/load). Occasionally an owner
  retires and is replaced (buying out a store's whole stock encourages this).
- Things you sell may be bought by other customers while you are away.

### The stores

#### General Store (1)

- **Possible owners (max purse):** Bilbo the Friendly (Hobbit) (5000 gold); Rincewind the Chicken (Human) (10000 gold); Snafu the Midget (Gnome) (20000 gold); Lyar-el the Comely (Elf) (30000 gold)
- **Stock size:** 0–4 item slots; turnover 2
- **Always stocked:** Cloak, Ration of Food, Wooden Torch, Flask of Oil, Shovel, Pick, Iron Shot, Arrow, Bolt
- **May stock:** Pint of Fine Wine, Slime Mold, Flask of Whisky, Hard Biscuit, Stoneskin, Purging
- **Buys / accepts:** light, food, mushroom, flask, digger, cloak, shot, bolt, arrow

#### Armoury (2)

- **Possible owners (max purse):** Kon-Dar the Ugly (Half-Orc) (5000 gold); Darg-Low the Grim (Human) (10000 gold); Decado the Handsome (Dunadan) (25000 gold); Mauglin the Grumpy (Dwarf) (30000 gold)
- **Stock size:** 6–18 item slots; turnover 9
- **May stock:** Pair of Leather Sandals, Pair of Leather Boots, Pair of Iron Shod Boots, Hard Leather Cap, Metal Cap, Iron Helm, Robe, Soft Leather Armour, Studded Leather Armour, Hard Leather Armour, Leather Scale Mail, Metal Scale Mail, Chain Mail, Set of Leather Gloves, Set of Gauntlets, Wicker Shield, Small Metal Shield, Leather Shield
- **Buys / accepts:** boots, gloves, crown, helm, shield, cloak, soft armor, hard armor, dragon armor

#### Weapon Smiths (3)

- **Possible owners (max purse):** Ithyl-Mak the Beastly (Half-Troll) (5000 gold); Arndal Beast-Slayer (Half-Elf) (10000 gold); Tarl Beast-Master (Hobbit) (25000 gold); Oglign Dragon-Slayer (Dwarf) (30000 gold)
- **Stock size:** 3–14 item slots; turnover 9
- **May stock:** Iron Shot, Arrow, Bolt, Dagger, Main Gauche, Rapier, Short Sword, Cutlass, Tulwar, Broad Sword, Long Sword, Scimitar, Katana, Bastard Sword, Spear, Awl-Pike, Trident, Pike, Beaked Axe, Broad Axe, Battle Axe, Lance, Lucerne Hammer, Whip, Sling, Short Bow, Long Bow, Light Crossbow
- **Buys / accepts:** shot, bolt, arrow, bow, digger, sword, polearm, hafted

#### Bookseller (4)

- **Possible owners (max purse):** Ludwig the Humble (Human) (15000 gold); Gunnar the Paladin (Human) (20000 gold); Delilah the Pure (Elf) (25000 gold); Bosk the Wise (Dwarf) (30000 gold)
- **Stock size:** 0–2 item slots; turnover 0
- **Always stocked:** prayer book, magic book, nature book, shadow book
- **Buys / accepts:** magic book, prayer book, nature book, shadow book

#### Alchemy shop (5)

- **Possible owners (max purse):** Mauser the Chemist (Half-Elf) (10000 gold); Wizzle the Chaotic (Hobbit) (10000 gold); Ga-nat the Greedy (Gnome) (15000 gold); Vella the Slender (Human) (15000 gold)
- **Stock size:** 6–10 item slots; turnover 9
- **Always stocked:** Word of Recall, Phase Door, Remove Curse, Cure Light Wounds
- **May stock:** Monster Confusion (scroll), Magic Mapping (scroll), Detect Invisible (scroll), Recharging (scroll), Remove Hunger (scroll), Blessing (scroll), Holy Chant (scroll), Boldness (potion), Berserk Strength (potion), Resist Heat (potion), Resist Cold (potion), Restore Life Levels (potion), Neutralize Poison (potion), Cure Serious Wounds (potion), Cure Critical Wounds (potion)
- **Buys / accepts:** scroll, potion

#### Magic shop (6)

- **Possible owners (max purse):** Ariel the Sorceress (Half-Elf) (15000 gold); Buggerby the Great (Gnome) (20000 gold); Inglorian the Mage (Human) (25000 gold); Luthien Starshine (High-Elf) (30000 gold)
- **Stock size:** 4–14 item slots; turnover 9
- **May stock:** Searching (ring), Feather Falling (ring), Protection (ring), Open Wounds (ring), Slow Digestion (amulet), Resist Acid (amulet), Light (wand), Slow Monster (wand), Confuse Monster (wand), Magic Missile (wand), Stinking Cloud (wand), Stone to Mud (wand), Teleportation (staff), Light (staff), Mapping (staff), Detect Invisible (staff), Detect Evil (staff)
- **Buys / accepts:** magic book, amulet, ring, staff, wand, rod

#### Black market (7)

- **Possible owners (max purse):** Lo-Hak the Awful (Half-Troll) (15000 gold); Histor the Goblin (Kobold) (20000 gold); Durwin the Shifty (Half-Orc) (25000 gold); Drago the Fair (Elf) (30000 gold)
- **Stock size:** 6–18 item slots; turnover 9

#### Home (8)

The **Bookseller** stocks the town books of all four realms. The **Black
Market** can stock anything except artifacts, at extortionate prices — but it
is the main place to buy stat potions and other rare goods. The **Home** stores
items for free; it never changes on its own.

---

## 13. The dungeon

Take the `>` in town to descend. Each dungeon level is 50 ft deeper ("Lev 1" =
50 ft; level 99 = 4950 ft; levels go beyond 100 down to 127). Levels are
generated anew each time you arrive and lost when you leave (unless
*persistent levels* is on). Monsters can use stairs but will not follow you
between levels.

### Map symbols

Use `/` to identify any symbol and `~` for full knowledge menus. Symbols can be
changed with pref files. Default appearances:

**Terrain and features**

| Symbol | Meaning | Symbol | Meaning |
|---|---|---|---|
| `.` | floor (or an unseen trap) | `#` | granite wall / secret door / lava (red) / permanent wall |
| `^` | known trap | `%` | mineral vein (magma or quartz), or a web |
| `;` | glyph of warding, or decoy | `*` | mineral vein with treasure |
| `'` | open or broken door | `+` | closed or locked door |
| `<` | up staircase | `:` | rubble (passable rubble is a different colour) |
| `>` | down staircase | `1`–`8` | store entrances (General, Armoury, Weaponsmith, Bookseller, Alchemist, Magic shop, Black market, Home) |

**Objects**

| Symbol | Meaning | Symbol | Meaning |
|---|---|---|---|
| `!` | potion or flask | `/` | polearm |
| `?` | scroll or book | `\|` | edged weapon (sword) |
| `,` | food or mushroom | `\` | hafted weapon or digger |
| `-` | wand or rod | `}` | launcher (sling, bow, crossbow) |
| `_` | staff | `{` | ammunition (shot, arrow, bolt) |
| `=` | ring | `(` | soft armour or cloak |
| `"` | amulet | `[` | hard armour or dragon armour |
| `$` | gold, gems | `]` | misc. armour (helm, crown, gloves, boots) |
| `~` | light source, chest | `)` | shield |
| `&` | pile of several items | | |

**Monsters** — letters (and a few symbols) by monster type; full table in
[section 22](#22-monsters).

### Light

You need light to see your surroundings, read scrolls, cast spells (except
Necromancers), and notice many things. Rooms may be permanently lit; corridors
are dark, and the dungeon gets darker with depth. Wield a **torch** (light radius 2,
burns for 5000 turns, cannot be refuelled; its light dims when nearly out) or
a **lantern** (light radius 3, holds up to 15,000 turns of fuel, refuel with
flasks of oil using `F`). Artifacts and some
egos give permanent light. Your current light radius is shown on the status
line. Necromancers are the exception: their *Unlight* means they usually do
best without any light.

### Doors, secret doors and stairs

Doors may be closed, locked, open or broken. Moving into a closed door opens
it (picking a lock if needed). You can lock doors with `D`. Secret doors look
exactly like granite and are found simply by walking next to them. Monsters
use secret doors and often leave them open.

Most levels have at least one up and at least two down staircases. Stairs,
permanent rock and shop entrances cannot be destroyed. With
`birth_connect_stairs` on (default), taking stairs places you on a staircase of
the same direction on the new level.

### Traps

Traps are hidden until noticed (you have a chance to notice each one as it
comes into view, based on searching skill) or detected magically. Inside an
area covered by trap detection the status line shows a green `DTrap` (yellow on
the edge). Detection does not reveal traps created later by monsters. Disarm
with `D` (failure may trigger the trap); there are separate skills for
physical and magical traps. Walking onto a known trap with a move tries to
disarm it; `W` walks on regardless.

| Trap / feature | Symbol | Min. depth | Description |
|---|---|---|---|
| glyph of warding | `;` | 0 | A magical rune which monsters cannot pass without difficulty. |
| decoy | `;` | 0 | A magical decoy which sounds and looks like the player. |
| door lock | ` ` | 0 | A device which makes a closed door harder to open. |
| web | `%` | 0 | A tangle of sticky spider webs. |
| trap door | `^` | 2 | It looks like floor, but step incautiously and you will fall through. |
| pit | `^` | 2 | A hole dug to snare the unwary. |
| pit | `^` | 2 | A hole dug to snare the unwary, with spikes. |
| pit | `^` | 2 | A hole dug to snare the unwary, with poisoned spikes. |
| strange rune | `^` | 3 | A magical rune which calls a monster to attack you. |
| strange rune | `^` | 3 | A magical rune which calls up monsters to attack you. |
| strange rune | `^` | 20 | A magical rune which calls the undead to attack you. |
| strange rune | `^` | 20 | A magical rune which calls dragons to attack you. |
| strange rune | `^` | 20 | A magical rune which calls demons to attack you. |
| strange rune | `^` | 1 | A magical rune which teleports you to a random location. |
| discolored spot | `^` | 2 | A strange coloured spot on the ground - beware its fiery attack. |
| discolored spot | `^` | 2 | A strange coloured spot on the ground - beware its acidic attack. |
| dart trap | `^` | 2 | A trap which shoots slowing darts. |
| dart trap | `^` | 6 | A trap which shoots strength-draining darts. |
| dart trap | `^` | 6 | A trap which shoots dexterity-draining darts. |
| dart trap | `^` | 6 | A trap which shoots constitution-draining darts. |
| gas trap | `^` | 2 | A trap that releases gas to blind you. |
| gas trap | `^` | 1 | A trap that releases gas to confuse you. |
| gas trap | `^` | 2 | A trap that releases gas to poison you. |
| gas trap | `^` | 2 | A trap that releases gas to put you to sleep. |
| alarm | `^` | 5 | A trap that wakes nearby monsters and impels them toward you. |
| alarm | `^` | 1 | A trap that wakes nearby monsters. |
| explosion | `^` | 15 | A trap that blows up the floor. |
| explosion | `^` | 25 | A trap that creates an explosion. |
| mind blast trap | `^` | 20 | A trap that blasts your senses. |
| mind blast trap | `^` | 40 | A trap that blasts your senses. |
| ancient mechanism | `^` | 4 | A trap that drops a boulder on you. |
| ancient mechanism | `^` | 30 | A trap that causes an earthquake. |
| ancient mechanism | `^` | 4 | A pressure plate that drops a stone block behind you. |
| ancient mechanism | `^` | 40 | A trap that clears the surrounding area. |
| trap | `^` | 5 | A trap that creates a ball of light. |
| trap | `^` | 5 | A trap that envelops you in darkness. |
| trap | `^` | 5 | A trap that drains your mana. |
| trap | `^` | 20 | A trap designed to bleed you to death. |
| trap | `^` | 5 | A trap that stuns you and gives you stoneskin. |

### Mining

Some treasure can only be dug out of veins: quartz veins hold the most,
magma veins some. Digging is slow without a **shovel**, **pick** or **mattock**
(their `(+N)` is the digging bonus); the best digger in your pack is used
automatically. Strong characters with heavy weapons can dig without one, but
granite needs a real digger. Follow a vein rather than cutting through granite.
Treasure detection reveals treasure in veins in view. Always carrying a digger
is good insurance against being trapped (by spells, traps, or arriving via
recall or disconnected stairs in an enclosed area). Some vaults are sealed in
permanent rock and granite and must be dug or magicked into — and are guarded
by powerful monsters.

### Level types

Most levels use the *classic* or *modified* layouts (rooms joined by
corridors). Other profiles appear occasionally: *caverns* (from level 15),
*labyrinths*, *lairs* and *gauntlets* (from level 20), *hard centre* vaults
(from level 50), and *Moria-style* levels. Rooms may contain **pits** and
**nests** of themed monsters, and **vaults** full of treasure and monsters.
The town has its own layout.

### Level and object feelings

When you arrive (if `birth_feelings` is on) you get a **danger feeling**, which
depends only on the monsters present at generation, relative to depth:

| # | Danger feeling |
|---|---|
| 1 | This seems a quiet, peaceful place |
| 2 | This seems a tame, sheltered place |
| 3 | This place seems reasonably safe |
| 4 | This place does not seem too risky |
| 5 | You feel nervous about this place |
| 6 | You feel anxious about this place |
| 7 | This place seems terribly dangerous |
| 8 | This place seems murderous |
| 9 | Omens of death haunt this place |

After exploring enough you also get a **treasure feeling**:

| # | Treasure feeling |
|---|---|
| 1 | there is naught but cobwebs here. |
| 2 | there are only scraps of junk here. |
| 3 | there aren't many treasures here. |
| 4 | there may not be much interesting here. |
| 5 | there may be something worthwhile here. |
| 6 | there are good treasures here. |
| 7 | there are very good treasures here. |
| 8 | there are excellent treasures here. |
| 9 | there are superb treasures here. |
| $ | you sense an item of wondrous power! (an artifact; only with `birth_lose_arts` on — otherwise an artifact guarantees 5 or better) |

The status line shows `LF:d-t` (danger–treasure, `?` until known). `Ctrl-F`
repeats the feelings.

### Word of Recall and Deep Descent

A **Scroll of Word of Recall** (every class starts with one unless
`birth_no_recall`) teleports you to town when read in the dungeon, or down to
your recall depth (normally your maximum depth) when read in town. If you
read it in the dungeon while shallower than your maximum depth, you are asked
whether to reset the recall depth to the current level. It takes 15–34 player
turns to activate (`Recall` shows on the status line); reading another offers
to cancel it. It is not an emergency escape. **Deep Descent** teleports you
five levels below your maximum depth (max depth + 5 with default settings)
after a delay of 4–7 turns (`Descent` on the status line).

### Quests

There are two fixed quests: **Sauron** on level 99 and **Morgoth** on level
100. A quest level has no down staircase until its quest monster is dead. You
cannot go below 99 until Sauron is dead, nor below 100 until Morgoth is dead.

---

## 14. Objects and equipment

Items lie on the floor, are carried by monsters (dropped when killed), and are
sold in stores. Depth is by far the most important factor in item quality. With
`birth_stacking` (default) many items can share a grid (shown as `&`). Doors,
rubble, traps and stairs cannot hold items.

You can carry **23** pack stacks of up to **40** items, wear **12** items, and
use **10** quiver slots. Total weight beyond your strength-based limit slows
you.

### Object categories

- **Weapons** — swords `|`, hafted weapons `\`, polearms `/`; damage dice
  `XdY` and to-hit/to-damage `(+h,+d)`. Weight matters for blows.
- **Launchers** `}` — slings (shots), bows (arrows), crossbows (bolts); a
  damage **multiplier** `(xN)`.
- **Ammunition** `{` — shots, arrows, bolts; damage dice and `(+h,+d)`.
- **Diggers** — shovels, picks, mattocks; digging bonus `(+N)`; usable as
  weapons.
- **Armour** — body armour (soft `(`, hard `[`, dragon scale `[`), cloaks,
  shields, helms/crowns, gloves, boots; AC `[base,+bonus]`. Heavy body armour
  may show a to-hit penalty `(-N)`.
- **Jewellery** — rings `=` (two slots) and amulets `"`.
- **Light sources** `~` — torches, lanterns, artifact lights.
- **Potions** `!`, **scrolls** `?`, **food and mushrooms** `,`, **flasks of
  oil** `!` (fuel, or throw for fire damage).
- **Magic devices** — **wands** `-` (aimed, charges), **staffs** `_` (area,
  charges), **rods** `-` (recharge over time, never run out). Charges can be
  restored with Recharging (failure can destroy the device) or by giving the
  device to the Magic shop.
- **Books** `?` — one set per realm (four town books and several dungeon books
  per class).
- **Chests** `~` — may be locked and trapped; open (`o`) or disarm (`D`) them on
  the floor (not while carrying them).
- **Gold** `$`.

Full lists of every object kind are in [Appendix B](#appendix-b--object-kinds).

### Egos and artifacts

**Ego items** have a name suffix/prefix such as *of Speed* or *(Holy
Avenger)* and a themed set of extra powers; see
[Appendix C](#appendix-c--ego-item-names). Common ones:

- **(Defender)** — AC, resist the four base elements, feather fall, stealth,
  see invisible, free action, regeneration, one random sustain.
- **(Holy Avenger)** — WIS, AC, slay evil/demon/undead, protection from fear,
  see invisible, random sustain; blessed (good for priests and paladins).
- **(Blessed)** — WIS, one random power, blessed.
- **of Westernesse** — slay orc/troll/giant, STR/DEX/CON, see invisible, free
  action.
- **of Gondolin**, **of Extra Attacks**, **of Morgul** (cursed).
- **Elemental brands** (of Acid, Lightning, Flame, Frost, Venom) — treble dice
  damage against non-resistant monsters and give the matching resistance. Only
  the dice are multiplied, not the `+dam` bonus.
- **Slays** (of Slay Animal / Evil ×2; Orc, Troll, Giant, Dragon, Demon, Undead
  ×3) and **\*Slays\*** (extra power, a stat bonus; \*Slay\* Dragon/Demon/Undead
  ×5).
- **Diggers of Digging** — dig granite easily; acid branded.
- **Launchers** of Accuracy (to-hit), Power (to-dam), Extra Shots, Extra Might
  (higher multiplier — multiplies *everything*, including ammo slays and the
  ammo's and launcher's damage bonuses), of Lothlórien, the Haradrim, Buckland,
  the Nazgûl.
- **Ammo** of Wounding (big bonuses), elemental brands and slays (for
  ammunition slays and brands *do* multiply the damage bonus), of Holy Might,
  of Backbiting (cursed).
- **Armour** of Resist Acid/Lightning/Fire/Cold, of Resistance (all four),
  of Elvenkind (all four plus stealth and one high resist), **Robes of
  Permanence** (all four, sustains everything, hold life, one random resist),
  **Dragon Scale Mails** (resist their dragon's breath, can be activated to
  breathe; can also have egos), (Dwarven), of Preservation shields.
- **Helms/crowns** of Intelligence/Wisdom (raise and sustain), Seeing (see
  invisible, searching, protection from blindness), Infravision, Telepathy,
  Regeneration, Light; **crowns** of the Magi, Might, Lordliness, Serenity,
  Night and Day.
- **Cloaks** of Protection, Stealth, Aman, the Magi. **Gloves** of Free
  Action, Slaying, Agility, Power, Thievery, Combat. **Boots** of Slow Descent,
  Stealth, Free Action, Speed, Stability, Elvenkind.
- **Lights** of Brightness, of Shadows, of True Sight, (Everburning).

Rare *base* items such as Blades of Chaos, Maces of Disruption, Scythes of
Slicing, Mithril/Adamantite armour and Elven cloaks can also carry egos.

**Artifacts** are unique named items, usually far more powerful than egos and
often with an activation. Each can be found only once per game. With
`birth_randarts` you get a freshly generated random set instead.

### Equipment from elsewhere affects attacks

Non-weapon equipment applies its to-hit, to-damage, brands and slays to your
melee (see the character sheet or `I` on the weapon). For missiles and thrown
objects only the to-hit bonuses from other equipment apply (plus to-damage when
`birth_percent_damage` is on).

---

## 15. Identification, runes and curses

Angband uses **identify-by-use**:

- **Flavoured items** (potions, scrolls, mushrooms, wands, staffs, rods, rings,
  amulets) get random appearances each game ("a Red Potion"). You learn a
  flavour by using it, buying it, or giving/selling it to a store. Learning a
  device by use may need a suitable target. Used but unidentified items are
  marked `{tried}`; empty unknown devices `{empty}`. Hobbits know mushrooms and
  Gnomes know devices automatically.
- **Wearable items** have properties called **runes** (each resistance, flag,
  modifier, slay, brand and curse is a rune). Once you learn a rune on any
  object you recognise it everywhere. You learn runes by wielding/wearing the
  item and experiencing the effect (being hit by fire teaches resist fire,
  fighting teaches to-hit/to-dam, etc.), by spells and scrolls of *Identify
  Rune*, or by giving the item to a store. Obvious modifiers (stats, speed,
  light…) are learned on wielding. Items with unknown runes show `{??}`.
- With `birth_know_runes` / `birth_know_flavors` you start knowing everything.

Labels the game adds: `{??}` unknown runes, `{cursed}` known curse,
`{tried}`, `{empty}`, `{ignored}`, `{unseen}` (stores). Several labels are
combined with your own inscription, e.g. `{@m1, cursed, ??}`.

### Curses

Curses are runes too, and any wearable item may carry several, each with a
*power*. A curse might reduce stats or combat values, or periodically do
something unpleasant. You may wear a cursed item anyway if its benefits are
worth it. **Remove Curse** scrolls/staffs/spells let you pick one curse and try
to break it; failure makes the object **fragile**, and a fragile object may be
destroyed by a later attempt. Curse descriptions:

| Curse | Effect | Can appear on |
|---|---|---|
| vulnerability | attracts opponents and weakens your defences | cloak, soft armor, hard armor, dragon armor, shield |
| teleportation | randomly makes you teleport | helm, crown, amulet, ring |
| dullness | makes you mentally slow | helm, crown |
| sickliness | makes you frail | helm, crown |
| enveloping | protects you at the expense of free movement | cloak |
| irritation | annoys monsters and makes it hard for you to hit them | cloak |
| weakness | weakens your grasp | gloves |
| clumsiness | gives you butterfingers | gloves |
| slowness | drags at your feet | boots |
| annoyance | wakes up and annoys nearby monsters, and delays your escape | boots |
| poison | poisons you from time to time | bow, digger, hafted, polearm, sword, boots, gloves, helm, crown, shield, cloak, soft armor, hard armor, dragon armor, light, amulet, ring |
| siren | occasionally makes a loud noise | bow, digger, hafted, polearm, sword, boots, gloves, helm, crown, shield, cloak, soft armor, hard armor, dragon armor, light, amulet, ring |
| hallucination | makes you hallucinate sometimes | bow, digger, hafted, polearm, sword, boots, gloves, helm, crown, shield, cloak, soft armor, hard armor, dragon armor, light, amulet, ring |
| paralysis | paralyses you every now and then | bow, digger, hafted, polearm, sword, boots, gloves, helm, crown, shield, cloak, soft armor, hard armor, dragon armor, light, amulet, ring |
| dragon summon | sometimes summons a dragon | dragon armor |
| demon summon | occasionally summons a demon | digger, hafted, polearm, sword |
| undead summon | occasionally summons an undead | digger, hafted, polearm, sword |
| impair mana recovery | makes you slow to recover mana | helm, crown |
| impair hitpoint recovery | makes you slow to recover hitpoints | boots, gloves, shield, cloak, soft armor, hard armor, dragon armor |
| cowardice | makes you too scared to fight | amulet, ring |
| stone | periodically turns your skin to stone | amulet, ring |
| anti-teleportation | prevents you from teleporting | boots, cloak, soft armor, hard armor, dragon armor, amulet, ring |
| treacherous weapon | makes your weapon attack you | digger, hafted, polearm, sword |
| burning up | makes you vulnerable to fire attacks, but resistant to cold | boots, gloves, helm, crown, shield, cloak, soft armor, hard armor, dragon armor |
| chilled to the bone | makes you vulnerable to cold attacks, but resistant to fire | boots, gloves, helm, crown, shield, cloak, soft armor, hard armor, dragon armor |
| steelskin | makes your skin harder to damage, but it conducts electricity | digger, hafted, polearm, sword, helm, crown, shield, hard armor, dragon armor, light, amulet, ring |
| air swing | makes it hard to hit anything | digger, hafted, polearm, sword |

---

## 16. Inscriptions and ignoring

### Inscriptions

Inscribe with `{`, remove with `}`. Auto-inscriptions for object kinds and runes
can be set from the knowledge menus (`~`) or the options menu (`=` then `{`),
and saved to a pref file. Inscriptions with special meanings:

| Inscription | Meaning |
|---|---|
| `@x#` | Item answers to digit `#` for command key `x`. `@m1` on a spellbook: `m1a` always casts from that book. `@q1` on a potion, `@r2` on a scroll, `@u1` on a staff, etc. Using the same digit on several items picks the first. |
| `@#` | Item answers to digit `#` for every command. |
| `@v#` | Also puts a throwing item in quiver slot `#`. Ammo inscribed `@f#` (`@t#` roguelike) is placed in quiver slot `#`. |
| `!x` | Ask for confirmation before command `x` uses this item (`!d` drop, `!k` ignore, `!v` throw, `!r` read, `!q` quaff…). Several `!x` give several prompts. |
| `!*` | Confirm *any* use of the item. |
| `!!` | Tell me when this item has recharged. |
| `^x` | While this item is worn, confirm before *doing* command `x` at all (`^>` reminds you before taking stairs). |
| `^*` | Confirm every command while worn (very annoying). |
| `=g` | Always pick this item up. `=g4` limits automatic pickup to keep at most 4 in the pack (works with `pickup_inven`). |

Good habits: inscribe spellbooks `@m1`…`@m9` so losing a book never makes you
cast the wrong spell; inscribe Word of Recall `!r` or `!*`; inscribe precious
items `!k!d!v`.

### Ignoring

`k` (`Ctrl-D`) ignores an item: it is dropped and hidden. You can ignore just
that object or every object like it. For wearables you choose by **quality**:

- **bad** — negative AC, to-hit or to-dam;
- **average** — no bonuses at all (non-magical);
- **good** — positive bonuses but no special powers;
- **non-artifact** — everything except artifacts;

or by **ego type**, or both. The full ignore setup is under `=` → `i` ("Item
ignoring setup"). `K` (`O`) temporarily shows ignored items (`Unignoring` on the
status line), so you can un-ignore something with `k` and then switch ignoring
back on.

## 17. Combat

### Melee

Move into a monster to attack it. With a weapon you strike for its damage dice
plus bonuses; without one you get a weak punch. Hits and misses depend on your
fighting skill and to-hit bonuses against the monster's AC; a "miss" may reach
the target but do no damage.

**Blows per round** depend on class (maximum blows, minimum weapon weight,
strength multiplier — see the class table), STR, DEX and weapon weight: light
weapons and high DEX/STR give more blows. Warriors can reach 6 blows,
hybrids 5, pure casters 4. Blows are shown as decimals (e.g. 2.3/turn); the
fraction carries over as saved energy. `I`nspecting a weapon shows blows and
average damage per round, including against monsters vulnerable to its slays
and brands.

**Critical hits** are more likely with heavy weapons, high to-hit and skill,
and against sleeping, held, stunned, confused or frightened monsters. They are
reported as "It was a good / great / superb hit!" and multiply damage.

**Shield bash** (Warriors, Paladins, Blackguards) sometimes adds a bash with
your shield, which can stun.

**Unaware and sleeping monsters** are easier to hit and crit, and stealthy
characters often get the first blow.

**Monsters in walls**: monsters that pass through walls can be attacked only
by tunnelling/altering into the wall (`T` or `+`) with your melee weapon; bolts
and missiles cannot reach them.

### Missiles and throwing

Wield a launcher in the bow slot and keep ammunition in the quiver. A shot's
damage is (ammo dice + ammo to-dam + launcher to-dam) × launcher multiplier,
with brands/slays from the ammunition (and launcher) applying. Shooting skill
and DEX govern the chance to hit; the multiplier governs range. Extra shots
(ego launchers, Rangers) increase the shots per turn. Ammunition can break;
iron shots rarely do, arrows often.

Throwing (`v`) uses the throwing skill; range comes from STR and the object's
weight. Objects made for throwing (marked as such when inspected) do much more
damage when thrown. Flasks of oil are the classic early-game thrown weapon
(fire damage). `I`nspect ammunition or throwing weapons to see expected damage.

### Armour class

AC usually ranges from 0 to about 200. Armour helps in three ways: it makes
monster melee blows miss more, it reduces the damage of blows that land, and
body armour soaks up some acid damage (and may be damaged instead). Armour
shown as `[10,+5]` is base 10, magical bonus +5 (the bonus is learned by being
hit while wearing it). Heavy body armour may show `(-N)` to-hit. Monster AC only
affects your chance to hit it.

### Monster status effects

Spells and devices can affect monsters without damaging them. A monster type
either can or cannot be affected; uniques often resist. Effects on the same
attribute do not stack (except where noted).

| Effect | Result |
|---|---|
| **Hold** | Paralyses the monster until you hit it or the duration ends (normally 3–8 turns). Easier criticals. |
| **Stun** | Melee accuracy and damage −25%; 1-in-10 chance to lose its turn; easier criticals. 5–10 turns. |
| **Confuse** | Spells fail 50% more often (stacks with fear); ranged attacks and spells ≥40% more likely to miss; balls/bolts/breaths may go astray; ≥30% erratic movement (bumping a wall may stun it); easier criticals. 5–10 turns. |
| **Slow** | −2 speed, more if more slowed. 10+ turns. |
| **Sleep** | Puts the monster to sleep; it wakes easily. |
| **Scare** | The monster flees; spells fail 20% more often (stacks with confusion/disenchantment); easier criticals. |
| **Disenchant** | Spells fail 50% more often (stacks with fear). 5–10 turns. |

Other monster-affecting effects include teleport other (sends a monster away;
no saving throw applies), polymorph, banishment, drain life
(not on the lifeless undead) and Necromancer **command**.

### Speed

Monsters have speeds described as *slowly* (−10), *fairly quickly* (+5),
*quickly* (+10), *very quickly* (+20) and *incredibly quickly* (+30, e.g.
Morgoth); non-unique monsters vary by up to ±2. +10 is double normal speed,
−10 half; +20 about triple; +30 not quite quadruple; beyond that changes
little. Permanent speed comes from items (boots of speed give +1d10, rings of
speed are usually up to just over +10, occasionally more; some artifacts).
**Temporary haste** (potion, staff, rod, spell, activation) always adds
exactly +10, and several sources only extend the duration. Slowing effects
likewise subtract exactly 10. Being over-weight or Full also slows you. The
`effective_speed` option shows speed as a multiplier (e.g. `Fast (x2.0)`).

---

## 18. Elements, resistances and status effects

### Bolts, balls, beams and breaths

Monster spells and breaths (and your own) need no hit roll and ignore AC.
**Bolts** hit the first monster (or you) in the line of fire; **beams** hit
everything along a line; **balls** explode at the target, damaging everything
in the radius, less with distance; **arcs/cones** spread from the caster.
**Breaths** are proportional to the monster's *current* HP (wounding a dragon
weakens its breath), with a cap per element (higher caps for common elements,
which are also easier to resist), and lose power with distance. Bolt and ball
damage usually depends on the caster's level.

### The four base elements and poison

**Fire, cold, lightning (electricity), acid** and **poison**. Permanent
resistance (from race or equipment) reduces damage to **1/3**; temporary
resistance (potion, spell, activation) also reduces to 1/3, and both together
give **1/9**. Several sources of the same kind do not add up (only extend a
temporary one's duration). **Immunity** (a few rare artifacts) to fire, cold,
lightning or acid means no damage and full protection for your gear. Some
items make you **vulnerable** to an element: you take 4/3 damage without a
permanent resistance (normal damage with one), and you cannot gain a temporary
resistance to it — unless you also have immunity.

Elements destroy inventory: **fire** burns scrolls, staffs, books and arrows;
**acid** dissolves scrolls, staffs, arrows and bolts and damages armour;
**lightning** destroys wands, rods, rings and amulets; **cold** shatters
potions. Pack items get a saving throw (better with resistance). Items lying on
the floor are destroyed without a saving throw, and on the floor even weapons,
armour and chests can be destroyed. On the floor: cold, shards, sound and force
always destroy potions; fire and plasma burn scrolls, staffs, books and
non-metal gear; acid destroys scrolls, staffs and non-mithril gear; lightning
and plasma destroy rings, amulets, wands and rods; mana storms destroy almost
everything.

### High elements

| Element | Unresisted side effect | Notes |
|---|---|---|
| Light | blindness (unless protected) | Elves/High-Elves resist |
| Dark | blindness | Half-Orcs resist; Necromancers resist (Unlight) |
| Sound | stunning | protection from stunning prevents the stun |
| Shards | cuts | resisting shards also stops ice cuts |
| Nexus | teleports you to/from the monster, up/down a level, or **scrambles** two stats | resist nexus also prevents level-teleport |
| Nether | experience drain | undead are immune; Necromancers resist |
| Chaos | hallucination and confusion, may drain experience | |
| Disenchantment | reduces the bonuses of your equipment | |
| Time | drains experience *ignoring hold life* or stats *ignoring sustains* | no resistance at all |
| Inertia | slows you (even with free action) | no resistance |
| Gravity | blinks you, stuns and slows | no resistance (protection from stunning stops the stun) |
| Force | stuns | no resistance |
| Plasma | stuns | no resistance |
| Water | confusion and stun | no resistance (confusion protection stops the confusion) |
| Ice | cuts and stun, shatters potions | |
| Mana | — | no resistance; includes magic missiles and mana storms |

Resisting chaos, disenchantment, nexus, sound, shards, light, dark or nether
reduces their damage to a random fraction between about 1/2 and 2/3 and
prevents their side effects.

**Experience drain** — *hold life* blocks many drains (95%, 90%, 75% or 50% of
the time depending on the monster) and cuts what is lost by 90%. Nether
resistance prevents drain from nether bolts/balls/breaths but not from
"touch to drain" melee. Lost experience is restored by Restore Life Levels or
regained naturally.

**Vital resistances**: fire, cold, acid, lightning, poison and confusion
protection are essential for the deep dungeon; blindness protection, chaos and
nether resistance come next. Free action and see invisible are needed by the
mid-game; hold life is very valuable.

### Saving throws and mental attacks

Blindness, confusion, slowness, fear and paralysis spells, and "cause wounds"
curses, are resisted by your **saving throw**, and prevented by the matching
protection (free action stops magical paralysis; it does not stop being
knocked out by stunning or fainting from hunger or over-casting). Status
effects inflicted by *melee* blows give no saving throw but are still stopped
by the matching protection. Uniques usually pass their saving throws against
your effects.

Some monsters teleport you (no save, except level teleports, which nexus
resistance stops). Some steal gold or items and teleport away (high DEX helps).

### Stuns and cuts

A stunned character has worse to-hit and much worse spell and device failure;
a heavy stun can knock you out. A cut bleeds HP each turn until healed (cure
potions, healing spells, or natural recovery for small cuts). Both are shown on
the status line by severity.

### Timed effects on the player

These are all the temporary states the game tracks, with their status-line
labels and what (if anything) prevents them:

| Effect | Description | Status-bar label(s) | Prevented / resisted by |
|---|---|---|---|
| FAST | haste | Haste | — |
| SLOW | slowness | Slow | free action |
| BLIND | blindness | Blind | protection from blindness |
| PARALYZED | paralysis | Paralyzed! | free action |
| CONFUSED | confusion | Confused | protection from confusion |
| AFRAID | fear | Afraid | protection from fear |
| IMAGE | hallucination | Halluc | chaos resistance |
| POISONED | poisoning | Poisoned | poison resistance, OPP_POIS |
| CUT | wounds | Graze, Light Cut, Bad Cut, Nasty Cut, Severe Cut, Deep Gash, Mortal Wound | ROCK |
| STUN | stunning | Stun, Heavy Stun, Knocked Out | protection from stunning |
| FOOD | nourishment | Starving, Faint, Weak, Hungry, Fed, Full | — |
| PROTEVIL | protection from evil | ProtEvil | — |
| INVULN | invulnerability | Invuln | — |
| HERO | heroism | Hero | — |
| SHERO | berserk rage | Berserk | — |
| SHIELD | mystic shield | Shield | — |
| BLESSED | your AC and to-hit bonus | Blssd | — |
| SINVIS | see invisible | SInvis | — |
| SINFRA | enhanced infravision | Infra | — |
| OPP_ACID | acid resistance | RAcid | acid immunity |
| OPP_ELEC | electricity resistance | RElec | lightning immunity |
| OPP_FIRE | fire resistance | RFire | fire immunity |
| OPP_COLD | cold resistance | RCold | cold immunity |
| OPP_POIS | poison resistance | RPois | poison resistance |
| OPP_CONF | confusion resistance | RConf | — |
| AMNESIA | amnesia | Amnesiac | — |
| TELEPATHY | telepathy | ESP | — |
| STONESKIN | stone skin | Stone | — |
| TERROR | terror | Terror | — |
| SPRINT | sprinting | Sprint | — |
| BOLD | fearlessness | Bold | — |
| SCRAMBLE | scrambled | Scrambled | nexus resistance |
| TRAPSAFE | safety from traps | TrapSafe | — |
| FASTCAST | mana channelling | FastCast | — |
| ATT_ACID | temporary acid brand | AttAcid | — |
| ATT_ELEC | temporary lightning brand | AttElec | — |
| ATT_FIRE | temporary fire brand | AttFire | — |
| ATT_COLD | temporary cold brand | AttCold | — |
| ATT_POIS | temporary poison brand | AttPois | — |
| ATT_CONF | temporary monster confusion | AttConf | — |
| ATT_EVIL | temporary slay evil | AttEvil | — |
| ATT_DEMON | temporary slay demon | AttDemon | — |
| ATT_VAMP | temporary hitpoint siphoning | AttVamp | — |
| HEAL | turn-by-turn healing through increased metabolism | Heal | — |
| COMMAND | commanding a monster | Cmd | — |
| ATT_RUN | escape on attack | HitRun | — |
| COVERTRACKS | leave no scent trail, reduce visibility | NoTrack | — |
| POWERSHOT | piercing shot | PShot | — |
| TAUNT | taunting | Taunt | — |
| BLOODLUST | desire to kill | Bloodlust | — |
| BLACKBREATH | black breath | BlkBrth | — |
| STEALTH | stealth | Stealth | — |
| FREE_ACT | free action | FrAct | — |

---

## 19. Magic and spellcasting

### Realms

| Realm | Classes | Stat | Books | "Spells" are called |
|---|---|---|---|---|
| Arcane | Mage, Rogue | INT | magic books | spells (*cast*) |
| Divine | Priest, Paladin | WIS | prayer books | prayers (*recite*) |
| Nature | Druid, Ranger | WIS | nature books | verses (*chant*) |
| Shadow | Necromancer, Blackguard | INT | shadow books | rituals (*perform*) |

Book *objects* are shared within a realm — a Rogue reads the same
*[First Spells]* a Mage buys — but **each class gets its own list of spells
from a book**, at its own levels and costs, and can only use the books its
class definition lists. Pure casters use five books (two or three sold in
town, the rest found only in the dungeon); hybrids (Paladin, Rogue, Ranger,
Blackguard) use two or three.

### Learning

When you have a new spell slot, `Study` appears on the status line. Use `G`
(or browse a book and pick a spell) to learn. Priests and Paladins receive a
random prayer from the chosen book; all other classes choose. The number of
spells you can know depends on your level and spell stat. The first time you
successfully cast a spell you gain the experience listed in its table.

### Casting and failure

Casting (`m`) needs the book in your inventory, the spell learned, and you
must be able to see and read: not blind, not confused, not amnesiac, and with
light (Necromancers excepted). Each spell has a base failure rate, reduced by
your level relative to the spell's level and by your spell stat, increased by
stunning and amnesia, and wearing more armour weight than the class's limit
reduces your mana. Classes with *Full Spellcaster* can reach 0%
failure; others always keep a minimum chance (5%). You can cast with
insufficient mana after confirming; the failure rate rises sharply and you may
faint (paralysis) and lose constitution.

Mana regenerates over time; resting (`R`) speeds it. Blackguards are
different: see *Combat Regeneration* in [section 6](#6-classes).

### Spell descriptions in-game

Browsing (`b`/`P`) a spell shows its current damage, duration, radius etc.,
calculated for your level, so the in-game text is more precise than the
generic descriptions below.

## 20. Complete spell lists by class

For each class: abilities, titles, starting kit (with the default
`birth_start_kit` on; with it off you get gold instead of most of the kit), and
every book with every spell. Columns: **Level** (character level needed),
**Mana**, **Base fail %** (before level/stat adjustments), **First-cast XP
factor** (the first successful cast of a spell gives this number × the
spell's level in experience). Town books are sold by the Bookseller; dungeon books must be
found.

### Warrior

**Class abilities:** Relentless [30] (You become immune to fear at level 30); No Magic (You cannot cast spells); Shield Bash (You can bash monsters with a shield in melee)

**Titles (by level band, every 5 levels):** Rookie, Soldier, Swordsman, Swashbuckler, Veteran, Myrmidon, Commando, Champion, Hero, Lord

**Starting kit:** 1–3× Ration of Food; 1–3× Wooden Torch; 1× Berserk Strength; 1× Dagger; 1× Soft Leather Armour; 1× Word of Recall (not with birth_no_recall)

This class cannot cast spells.

### Mage

**Class abilities:** Full Spellcaster (You may obtain a perfect success rate with magic); Extra Spell Beaming (Your bolt spells frequently turn into beams); Spell Choice (You may choose your own spells to study)

**Titles (by level band, every 5 levels):** Novice, Apprentice, Trickster, Illusionist, Spellbinder, Evoker, Conjurer, Warlock, Sorcerer, Arch-Mage

**Starting kit:** 1–3× Ration of Food; 1–3× Wooden Torch; 1× Rapier; 1× Word of Recall (not with birth_no_recall); 1× [First Spells]

**Magic realm:** arcane (spells, cast from magic books using INT; the verb used is "cast").

#### [First Spells] — sold in town (Bookseller), base cost 25 gold, found at dungeon levels 1 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Magic Missile | 1 | 1 | 22 | 4 | Fires a magic missile that always hits its target and does unresistable damage. Sometimes a beam is fired instead that hurts each monster in its path. The chance to get a beam goes up with your character level. |
| Light Room | 1 | 2 | 26 | 4 | Lights up all squares within 2 grids of the player, and hurts all light-sensitive monsters in the area of effect. If you are in a room, the entire room will be lit up as well. |
| Find Traps, Doors & Stairs | 1 | 1 | 20 | 2 | Detects all traps, doors, and stairs in the immediate area. |
| Phase Door | 2 | 2 | 22 | 4 | Teleports you randomly up to 10 squares away. |
| Electric Arc | 2 | 2 | 34 | 4 | Creates an electrical discharge that hurts monsters and destroys vulnerable items in its path. Length of the effect increases with player level. |
| Detect Monsters | 3 | 2 | 24 | 4 | Detects all non-invisible monsters in the immediate area, for one turn only. |
| Fire Ball | 6 | 5 | 33 | 5 | Shoots a radius-2 fire ball. |

#### [Attacks and Knowledge] — sold in town (Bookseller), base cost 400 gold, found at dungeon levels 10 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Recharging | 5 | 5 | 28 | 6 | Adds charges to a stack of wands or staves. Chance of success and number of charges gained increase with your level and decrease with level of wand or staff and number of charges. A failed attempt to recharge destroys one wand or staff from the stack. |
| Identify Rune | 8 | 7 | 25 | 8 | Reveals an unknown rune on an object. |
| Treasure Detection | 10 | 3 | 60 | 5 | Detects gold and senses objects in the immediate area. |
| Frost Bolt | 13 | 5 | 40 | 6 | Shoots a bolt of frost that always hits its target. Sometimes a beam is fired instead that hurts each monster in its path. The chance to get a beam goes up with your character level. |
| Reveal Monsters | 15 | 6 | 40 | 8 | Detects all monsters in the immediate area, for one turn only. |
| Acid Spray | 20 | 5 | 30 | 12 | Sprays acid in a 60-degree cone, hitting all monsters up to 10 grids away. |

#### [Magical Defences] — sold in town (Bookseller), base cost 1600 gold, found at dungeon levels 30 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Disable Traps, Destroy Doors | 5 | 5 | 30 | 6 | Disables traps and destroys doors within a 1-square radius of you. |
| Teleport Self | 7 | 6 | 35 | 5 | Teleports you randomly within the current level. |
| Teleport Other | 15 | 10 | 30 | 12 | Produces a bolt that teleports away the first monster in its path. Distance teleported increases with player level. |
| Resistance | 20 | 20 | 65 | 20 | Grants you resistance to acid, cold, fire and lightning for 20+1d20 turns. |
| Tap Magical Energy | 22 | 2 | 30 | 12 | Turns wand or staff energy into mana; tapping stuns the player briefly. |
| Mana Channel | 25 | 10 | 50 | 20 | Increases spellcasting speed by 33% for 5+1d5 turns. |

#### [Arcane Control] — dungeon only, base cost 30000 gold, found at dungeon levels 40 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Door Creation | 13 | 9 | 40 | 12 | A door is created on each empty floor space that is directly adjacent to you. These doors are closed, but not locked. |
| Mana Bolt | 25 | 8 | 60 | 30 | Fires a magic missile that always hits its target and does unresistable damage. Sometimes a beam is fired instead that hurts each monster in its path. The chance to get a beam goes up with your character level. |
| Teleport Level | 28 | 17 | 65 | 20 | Teleports you 1 level up or 1 level down (chosen at random). This spell may have no effect when the forced descent option is set. |
| Detection | 30 | 10 | 70 | 30 | Detects all traps, doors, stairs, treasure, objects and monsters in the immediate area. |
| Dimension Door | 35 | 30 | 80 | 40 | Teleports you to (or near to) a square of your choice. |
| Thrust Away | 40 | 12 | 90 | 40 | Fires a short range beam of force. |

#### [Wizard's Tome of Power] — dungeon only, base cost 50000 gold, found at dungeon levels 60 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Shock Wave | 20 | 5 | 40 | 16 | Shoots a radius-2 sound ball. Apart from doing sound damage, this also has a chance to stun each monster in the area of effect. |
| Explosion | 30 | 10 | 50 | 20 | Shoots a radius-2 shards ball and throws around anything in the area of effect. |
| Banishment | 35 | 45 | 95 | 25 | Removes all monsters represented by a chosen symbol from the level. Uniques are not affected. You take 1d4 points of damage for every monster removed. |
| Mass Banishment | 40 | 75 | 90 | 100 | Removes all monsters within 20 squares of you. Uniques are not affected. You take 1d3 points of damage for every monster removed. |
| Mana Storm | 45 | 16 | 85 | 200 | Shoots a radius-3 mana ball that does unresistable damage to each monster in its area of effect. |

### Druid

**Class abilities:** Full Spellcaster (You may obtain a perfect success rate with magic); Spell Choice (You may choose your own spells to study); Charms (You are extra persuasive to monsters)

**Titles (by level band, every 5 levels):** Wanderer, Tamer, Nurturer, Gardener, Forester, Creator, Earth Warder, Windrider, Stormwielder, High Mystic

**Starting kit:** 1–3× Ration of Food; 1× Whip; 1–3× Wooden Torch; 1× Word of Recall (not with birth_no_recall); 1× [Lesser Charms]

**Magic realm:** nature (verses, cast from nature books using WIS; the verb used is "chant").

#### [Lesser Charms] — sold in town (Bookseller), base cost 25 gold, found at dungeon levels 1 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Detect Life | 1 | 1 | 23 | 2 | Detects all living monsters in the immediate area, for one turn only. |
| Fox Form | 1 | 2 | 20 | 10 | Changes you into the shape of a quick and stealthy fox. While shapechanged you cannot use items. |
| Remove Hunger | 2 | 2 | 25 | 2 | Magically renders you comfortably fed (but not satiated). |
| Stinking Cloud | 3 | 2 | 27 | 4 | Shoots a radius-2 poison ball. |
| Confuse Monster | 5 | 3 | 30 | 5 | Confuses a single monster; more effective against animals. |
| Slow Monster | 6 | 4 | 30 | 5 | Slows down a monster; more effective against animals. |

#### [Gifts of Nature] — sold in town (Bookseller), base cost 400 gold, found at dungeon levels 10 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Cure Poison | 4 | 4 | 30 | 4 | Neutralizes poison. |
| Resist Poison | 7 | 6 | 32 | 5 | Grants you resistance to poison for 20+1d20 turns. |
| Turn Stone to Mud | 8 | 3 | 25 | 5 | Produces a beam that destroys the first section of wall it hits, unless it is a permanent wall. Each monster caught in the beam that is susceptible to rock remover takes 20+1d30 points of damage. |
| Sense Surroundings | 9 | 4 | 35 | 6 | Maps the nearby area. |
| Lightning Strike | 12 | 6 | 35 | 8 | Strikes a target from above with a lightning bolt, followed by a clap of thunder. If targeting a direction or the target is not a passable grid in the line of sight, the player will be the target. |
| Earth Rising | 14 | 5 | 40 | 12 | Causes shards to explode in a line out of the earth, striking objects and monsters. Length of the line increases with player level. |

#### [Creature Dominion] — dungeon only, base cost 10000 gold, found at dungeon levels 30 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Trance | 20 | 10 | 45 | 15 | Holds every adjacent, non-resistant monster for a level-dependent number of turns; animals are entranced for longer. |
| Mass Sleep | 25 | 15 | 50 | 15 | Attempts to put to sleep each monster within line of sight. Monsters that resist sleep are not affected. |
| Become Pukel-man | 30 | 20 | 75 | 50 | Gives you the form of a statue of the Drúedain, slow but hard to hurt. |
| Eagle's Flight | 35 | 20 | 80 | 80 | Gives you the form of one of the great eagles of Manwë. |
| Bear Form | 40 | 20 | 85 | 100 | Allows you to take the form of a great bear of the mountains, sweeping aside your enemies with your mighty paws. |

#### [Nature Craft] — dungeon only, base cost 30000 gold, found at dungeon levels 40 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Tremor | 20 | 20 | 60 | 16 | Causes a targeted 4-square radius earthquake. Earthquakes may hurt the player, damage monsters and destroy items in some squares in the area of effect. Artifacts will not be destroyed by this spell. This spell has no effect when used in the town. |
| Haste Self | 25 | 12 | 65 | 15 | Hastes you (+10 to speed) for a level-dependent duration. |
| Revitalize | 35 | 70 | 90 | 90 | Restores all stats and experience points to maximum. |
| Rapid Regeneration | 37 | 20 | 90 | 100 | Speeds up digestion to increase regeneration of hitpoints for the next 5+1d3 turns (or until hungry). |
| Herbal Curing | 40 | 20 | 90 | 100 | Cures all cuts, poison, stunning and black breath, and leaves you well-nourished. |

#### [Wild Forces] — dungeon only, base cost 50000 gold, found at dungeon levels 60 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Meteor Swarm | 30 | 14 | 85 | 20 | Shoots a swarm of 3 or 4 meteors. Meteors will travel until hitting a wall, a monster, or reaching the target location. Meteors will explode as a radius-1 ball that does unresistable damage to each monster in its area of effect. If this kills the monster that blocked the path to the target location, the next meteor may continue to find another target. |
| Rift | 35 | 20 | 60 | 25 | Fires a gravity beam that hurts each monster in its path and has a chance of teleporting it away. |
| Ice Storm | 37 | 25 | 75 | 24 | Strikes every monster in line of sight with sheets of ice from above. |
| Volcanic Eruption | 40 | 30 | 75 | 50 | Creates a radius 5 ball of fire, followed by a radius 5 earthquake, around the player. |
| River of Lightning | 43 | 35 | 75 | 100 | Spews plasma in a long narrow cone from the player. |

### Priest

**Class abilities:** Blessed Fighter (You get improved melee with blessed or hafted weapons); Full Spellcaster (You may obtain a perfect success rate with magic)

**Titles (by level band, every 5 levels):** Believer, Acolyte, Devotee, Adept, Evangelist, Priest, Elder, Prophet, Patriarch, High Priest

**Starting kit:** 1–3× Ration of Food; 1–3× Wooden Torch; 1× Mace; 2× Cure Serious Wounds; 1× Word of Recall (not with birth_no_recall); 1× [Novice's Handbook]

**Magic realm:** divine (prayers, cast from prayer books using WIS; the verb used is "recite").

#### [Novice's Handbook] — sold in town (Bookseller), base cost 25 gold, found at dungeon levels 1 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Call Light | 1 | 1 | 10 | 2 | Lights up all squares in a level-dependent area, and hurts all light-sensitive monsters in the area of effect. If you are in a room, the entire room will be lit up as well. |
| Detect Evil | 1 | 1 | 10 | 4 | Detects all evil monsters in the immediate area, for one turn only. |
| Minor Healing | 1 | 2 | 15 | 4 | Heals a level-dependent amount or percentage of hitpoints, and cures 20 points worth of cuts and of stunning. |
| Bless | 1 | 2 | 20 | 4 | Blesses you, giving a bonus of +5 to AC and +10 to-hit, for a level-dependent number of turns. |
| Sense Invisible | 3 | 4 | 25 | 4 | Enables you to see invisible monsters for 24+1d24 turns. |
| Heroism | 5 | 2 | 30 | 5 | Cures 10 points of damage, removes fear and, from character level 20, grants you heroism (resistance to fear, and a bonus of +12 to-hit) for a level-dependent duration. |

#### [Cleansing Power] — sold in town (Bookseller), base cost 100 gold, found at dungeon levels 5 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Orb of Draining | 7 | 7 | 40 | 4 | Shoots a radius-2 or radius-3 ball that does unresistable damage to each monster in its area of effect. Evil monsters take double damage. The bigger area of effect is attained at character level 30. |
| Spear of Light | 7 | 6 | 30 | 5 | Fires a beam that lights up each square and hurts each light-sensitive monster in its path. |
| Dispel Undead | 10 | 14 | 55 | 6 | Inflicts unresistable damage on each undead monster within line of sight. |
| Dispel Evil | 12 | 20 | 70 | 10 | Inflicts unresistable damage on each evil monster within line of sight. |
| Protection from Evil | 14 | 8 | 42 | 5 | Protects you from evil for a level-dependent duration: all melee attacks by evil monsters have a chance to be repelled, unless the monster's level is higher than your character level. |
| Remove Curse | 16 | 6 | 38 | 8 | Attempts to remove a curse from an item, more effective at higher character levels. |

#### [Healing and Sanctuary] — dungeon only, base cost 10000 gold, found at dungeon levels 30 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Portal | 10 | 4 | 30 | 8 | Teleports you randomly over a short distance. |
| Remembrance | 20 | 30 | 90 | 50 | Restores experience points to maximum if it is currently below maximum. |
| Word of Recall | 25 | 30 | 75 | 10 | Teleports you from the dungeon to the town or from the town to the deepest level you have visited in the dungeon. The recall effect is not immediate; it is delayed by 14+1d20 turns. During that delay, the prayer can be canceled by invoking the prayer of recall again. This prayer has no effect when the option to restrict the use of stairs and recall is set, unless Morgoth is dead. |
| Healing | 30 | 50 | 80 | 100 | Heals 2000 points of damage and cures all stunning, cuts, poison and amnesia. |
| Restoration | 35 | 70 | 90 | 130 | Restores to maximum all stats that are currently below maximum. |
| Clairvoyance | 37 | 50 | 80 | 150 | Lights up, maps out, and senses all objects on the complete current dungeon level. |

#### [Battle Blessings] — dungeon only, base cost 30000 gold, found at dungeon levels 40 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Enchant Weapon | 35 | 50 | 80 | 230 | Attempts to improve the to-hit bonus and the to-dam bonus of a weapon. |
| Enchant Armour | 37 | 60 | 85 | 250 | Attempts to improve the armour class bonus of a piece of armour. |
| Smite Evil | 25 | 20 | 70 | 40 | Temporarily makes your melee attacks slay evil creatures. |
| Glyph of Warding | 20 | 40 | 90 | 35 | Inscribes a glyph of warding beneath you. Summoned monsters can't appear on the glyph. If a monster attempts to move onto the glyph or melee you while you are standing on the glyph, it must first succeed in breaking the glyph. Higher level monsters are more likely to break the glyph. The glyph will remain where it is, until a monster succeeds in breaking it, or until you leave the level. |
| Demon Bane | 42 | 40 | 80 | 300 | Temporarily makes your melee attacks powerfully slay demons. |

#### [Wrath of the Valar] — dungeon only, base cost 50000 gold, found at dungeon levels 60 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Banish Evil | 25 | 25 | 80 | 250 | Teleports away each evil monster within line of sight. |
| Word of Destruction | 35 | 35 | 80 | 115 | Destroys everything in a 15-square radius circle around you. All monsters, objects, and terrain features in the area of effect are destroyed, except stairs and permanent walls. You will also be blinded for 10+1d10 turns, unless you have resistance to blindness or light. This prayer has no effect when used in the town. |
| Holy Word | 39 | 32 | 95 | 20 | Inflicts unresistable damage on each evil monster within line of sight, cures 1000 points of damage, cures all stunning, heals all cut damage, neutralizes poison, and removes fear. |
| Spear of Oromë | 40 | 10 | 75 | 130 | Fires a beam of holy magic that does extra damage to evil creatures. |
| Light of Varda | 42 | 40 | 85 | 200 | Casts a radius 4 ball of strong light, and then bathes the entire area in light doing 100 damage to every light-sensitive creature. |

### Necromancer

**Class abilities:** Full Spellcaster (You may obtain a perfect success rate with magic); Spell Choice (You may choose your own spells to study); Unlight (You gain stealth in, can see in, and resist the dark); Evil (You resist nether, but are hurt by holy attacks)

**Titles (by level band, every 5 levels):** Acolyte, Curser, Dark Student, Initiate, Slavemaster, Summoner, Controller, Commander, Dark Master, Night Lord

**Starting kit:** 1–3× Ration of Food; 1× Main Gauche; 1× Word of Recall (not with birth_no_recall); 1× [Into the Shadows]

**Magic realm:** shadow (rituals, cast from shadow books using INT; the verb used is "perform").

#### [Into the Shadows] — sold in town (Bookseller), base cost 25 gold, found at dungeon levels 1 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Nether Bolt | 1 | 1 | 22 | 2 | Fires a bolt of nether magic. |
| Sense Invisible | 2 | 4 | 25 | 3 | Enables you to see invisible monsters for 24+1d24 turns. |
| Create Darkness | 3 | 3 | 10 | 4 | Darkens all squares in a level-dependent area. If you are in a room, the entire room will be darkened as well. |
| Bat Form | 3 | 4 | 20 | 10 | Changes you into the shape of a bat, weak but with excellent senses. While shapechanged you cannot use items. |
| Read Minds | 4 | 4 | 10 | 6 | Detects all monsters which have a spirit in the immediate area, and then maps in a region around them, increasing in size with character level. |

#### [Dark Rituals] — sold in town (Bookseller), base cost 400 gold, found at dungeon levels 10 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Tap Unlife | 7 | 0 | 50 | 3 | Targets the nearest undead monster and damages it to gain player mana. |
| Crush | 10 | 6 | 40 | 5 | Instantly kills any monster in line of sight with less than four times the player's level in hitpoints, also hurting the player. Other monsters are unharmed. |
| Sleep Evil | 12 | 5 | 50 | 5 | Attempts to put to sleep each evil monster within line of sight. Monsters that resist sleep are not affected. |
| Shadow Shift | 15 | 10 | 50 | 10 | Displaces your spirit a short distance, hurting your body as it is dragged along. |
| Disenchant | 16 | 12 | 50 | 15 | Casts a bolt which wounds, and makes spellcasting monsters less successful in casting spells. |

#### [Fear and Torment] — sold in town (Bookseller), base cost 1600 gold, found at dungeon levels 30 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Frighten | 17 | 4 | 40 | 12 | Attempts to scare a single monster for a level-dependent duration. Monsters that resist fear are not affected. |
| Vampire Strike | 20 | 5 | 50 | 10 | Teleports you to the nearest living monster and drains its hitpoints to heal and nourish you. The number of hitpoints drained is twice your level or one more than the monster's current hitpoints, whichever is smaller. |
| Dispel Life | 22 | 20 | 50 | 10 | Inflicts unresistable damage on each living monster within line of sight. |
| Dark Spear | 25 | 10 | 60 | 12 | Fires a beam of darkness. |
| Warg Form | 28 | 20 | 60 | 80 | Changes you into the form of a warg, and gives you temporary berserk strength, at the cost of some hitpoints. |

#### [Deadly Powers] — dungeon only, base cost 30000 gold, found at dungeon levels 40 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Banish Spirits | 25 | 25 | 80 | 25 | Teleports away each monster with a spirit within line of sight. |
| Annihilate | 30 | 20 | 75 | 25 | Inflicts damage equal to four times player level on a single monster that is not undead, an elemental, a golem, or a vortex. |
| Grond's Blow | 35 | 35 | 80 | 115 | Destroys everything in a 15-square radius circle around you. All monsters, objects, and terrain features in the area of effect are destroyed, except stairs and permanent walls. You will also be blinded for 10+1d10 turns, unless you have resistance to blindness or dark. This ritual has no effect when used in the town. |
| Unleash Chaos | 38 | 15 | 90 | 100 | Fires a chaos beam, arc or ball in the chosen direction, wakes everything in the area, and attempts to frighten everything in line of sight. |
| Fume of Mordor | 40 | 50 | 80 | 150 | Darkens, maps out, and detects all objects on the complete current dungeon level. |
| Storm of Darkness | 43 | 16 | 80 | 200 | Invokes a radius-4 darkness ball. |

#### [Corruption of Spirit] — dungeon only, base cost 50000 gold, found at dungeon levels 60 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Power Sacrifice | 27 | 0 | 20 | 20 | Exchanges 50 hitpoints for 50 mana. |
| Zone of Unmagic | 32 | 16 | 60 | 110 | Produces a zone of disenchantment affecting the player and monsters within radius 4. |
| Vampire Form | 37 | 20 | 60 | 140 | Allows you to assume the form of a vampire at the cost of half your current hitpoints, then teleports you to the nearest living monster and drains a level-dependent number of hitpoints, healing and nourishing you. When first transformed you will temporarily be able to take hitpoints from living monsters with your bite attack. |
| Curse | 40 | 50 | 60 | 180 | Deals direct damage to a single monster, more the more damaged the monster is already, at the cost of 100 hitpoints. |
| Command | 45 | 40 | 60 | 250 | Allows you to take control of a monster, and move it instead of yourself. Possible actions are move, stand still, 'd'rop, 'm'agic, or 'r'elease. |

### Paladin

**Class abilities:** Blessed Fighter (You get improved melee with blessed or hafted weapons); Shield Bash (You can bash monsters with a shield in melee)

**Titles (by level band, every 5 levels):** Gallant, Keeper, Protector, Defender, Warder, Knight, Guardian, Chevalier, Paladin, Paladin Lord

**Starting kit:** 1–3× Ration of Food; 1–3× Wooden Torch; 1× Main Gauche; 1× Protection from Evil; 1× Word of Recall (not with birth_no_recall); 1× [Novice's Handbook]

**Magic realm:** divine (prayers, cast from prayer books using WIS; the verb used is "recite").

#### [Novice's Handbook] — sold in town (Bookseller), base cost 25 gold, found at dungeon levels 1 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Bless | 1 | 2 | 20 | 4 | Blesses you, giving a bonus of +5 to AC and +10 to-hit, for a level-dependent number of turns. |
| Detect Evil | 3 | 2 | 10 | 4 | Detects all evil monsters in the immediate area, for one turn only. |
| Call Light | 5 | 3 | 10 | 2 | Lights up all squares in a level-dependent area, and hurts all light-sensitive monsters in the area of effect. If you are in a room, the entire room will be lit up as well. |
| Minor Healing | 7 | 3 | 15 | 4 | Heals a level-dependent amount or percentage of hitpoints, and cures 20 points worth of cuts and of stunning. |
| Sense Invisible | 8 | 4 | 25 | 4 | Enables you to see invisible monsters for 24+1d24 turns. |
| Heroism | 12 | 5 | 30 | 5 | Cures 10 points of damage, removes fear and, from character level 15, grants you heroism (resistance to fear, and a bonus of +12 to-hit) for a level-dependent duration. |

#### [Healing and Sanctuary] — dungeon only, base cost 10000 gold, found at dungeon levels 30 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Protection from Evil | 15 | 8 | 42 | 5 | Protects you from evil for a level-dependent duration: all melee attacks by evil monsters have a chance to be repelled, unless the monster's level is higher than your character level. |
| Remove Curse | 20 | 12 | 38 | 8 | Attempts to remove a curse from an item, more effective at higher character levels. |
| Word of Recall | 25 | 30 | 75 | 10 | Teleports you from the dungeon to the town or from the town to the deepest level you have visited in the dungeon. The recall effect is not immediate; it is delayed by 14+1d20 turns. During that delay, the prayer can be canceled by invoking the prayer of recall again. This prayer has no effect when the option to restrict the use of stairs and recall is set, unless Morgoth is dead. |
| Healing | 30 | 50 | 80 | 100 | Heals 2000 points of damage and cures all stunning, cuts, poison and amnesia. |
| Clairvoyance | 37 | 50 | 80 | 150 | Lights up, maps out, and senses all objects on the complete current dungeon level. |

#### [Battle Blessings] — dungeon only, base cost 30000 gold, found at dungeon levels 40 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Smite Evil | 25 | 20 | 70 | 40 | Temporarily makes your melee attacks slay evil creatures. |
| Demon Bane | 30 | 40 | 80 | 150 | Temporarily makes your melee attacks powerfully slay demons. |
| Enchant Weapon | 35 | 50 | 80 | 230 | Attempts to improve the to-hit bonus and the to-dam bonus of a weapon. |
| Enchant Armour | 37 | 60 | 85 | 250 | Attempts to improve the armour class bonus of a piece of armour. |
| Single Combat | 40 | 50 | 30 | 300 | Encases you and the targeted monster in a stone cell from which only the winner can emerge. |

### Rogue

**Class abilities:** Spell Choice (You may choose your own spells to study); Pickpocketing (You can steal from monsters)

**Titles (by level band, every 5 levels):** Vagabond, Cutpurse, Footpad, Robber, Burglar, Filcher, Sharper, Rogue, Thief, Master Thief

**Starting kit:** 1–3× Ration of Food; 1–3× Wooden Torch; 1× Dagger; 1× Soft Leather Armour; 1× Word of Recall (not with birth_no_recall); 1× [First Spells]

**Magic realm:** arcane (spells, cast from magic books using INT; the verb used is "cast").

#### [First Spells] — sold in town (Bookseller), base cost 25 gold, found at dungeon levels 1 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Detect Monsters | 5 | 1 | 50 | 4 | Detects all non-invisible monsters in the immediate area, for one turn only. |
| Phase Door | 7 | 2 | 55 | 4 | Teleports you randomly up to 10 squares away. |
| Object Detection | 10 | 3 | 60 | 5 | Detects all objects in the immediate area. |
| Detect Stairs | 12 | 3 | 50 | 5 | Detects all stairs in a wide area. |
| Recharging | 20 | 10 | 50 | 6 | Adds charges to a stack of wands or staves. Chance of success and number of charges gained increase with your level and decrease with level of wand or staff and number of charges. A failed attempt to recharge destroys one wand or staff from the stack. |
| Reveal Monsters | 25 | 3 | 40 | 8 | Detects all monsters in the immediate area, for one turn only. |

#### [Arcane Control] — dungeon only, base cost 30000 gold, found at dungeon levels 40 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Teleport Self | 17 | 6 | 35 | 5 | Teleports you randomly within the current level. |
| Hit and Run | 23 | 20 | 40 | 20 | Prepares you to teleport a short distance after stealing from a monster. |
| Teleport Other | 30 | 10 | 30 | 50 | Produces a bolt that teleports away the first monster in its path. Distance teleported increases with player level. |
| Teleport Level | 35 | 17 | 65 | 80 | Teleports you 1 level up or 1 level down (chosen at random). This spell may have no effect when the forced descent option is set. |

### Ranger

**Class abilities:** Extra Shots (Your shooting speed improves every 3 levels); Spell Choice (You may choose your own spells to study)

**Titles (by level band, every 5 levels):** Runner, Strider, Scout, Courser, Tracker, Guide, Explorer, Pathfinder, Ranger, Ranger Lord

**Starting kit:** 1–3× Ration of Food; 1–3× Wooden Torch; 1× Main Gauche; 1× Short Bow; 15–20× Arrow; 1× Word of Recall (not with birth_no_recall); 1× [Lesser Charms]

**Magic realm:** nature (verses, cast from nature books using WIS; the verb used is "chant").

#### [Lesser Charms] — sold in town (Bookseller), base cost 25 gold, found at dungeon levels 1 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Remove Hunger | 3 | 1 | 25 | 2 | Magically renders you comfortably fed (but not satiated). |
| Detect Life | 5 | 2 | 23 | 2 | Detects all living monsters in the immediate area, for one turn only. |
| Herbal Curing | 9 | 6 | 30 | 4 | Cures all cuts, poison, stunning and black breath. |
| Resist Poison | 12 | 10 | 32 | 5 | Grants you resistance to poison for 20+1d20 turns. |
| Turn Stone to Mud | 15 | 5 | 25 | 5 | Produces a beam that destroys the first section of wall it hits, unless it is a permanent wall. Each monster caught in the beam that is susceptible to rock remover takes 20+1d30 points of damage. |
| Sense Surroundings | 20 | 8 | 35 | 8 | Maps the nearby area. |

#### [Nature Craft] — dungeon only, base cost 30000 gold, found at dungeon levels 40 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Cover Tracks | 20 | 20 | 40 | 40 | Prevents you from leaving scent for monsters to track you by for a level-dependent duration. |
| Create Arrows | 22 | 20 | 40 | 40 | Allows the player to turn a staff into a stack of arrows, with quality depending on staff level and player level. |
| Haste Self | 25 | 12 | 65 | 15 | Hastes you (+10 to speed) for a level-dependent duration. |
| Decoy | 30 | 30 | 40 | 60 | Leaves a magical decoy that looks and sounds like the player. |
| Brand Ammunition | 40 | 60 | 95 | 120 | Brands one stack of ammunition with fire, cold, or poison (selected at random) and at the same time attempts to improve the to-hit bonus and the to-dam bonus of the same ammunition. The spell has no effect if the ammunition is already branded, has a slay, is broken, or is cursed. |

### Blackguard

**Class abilities:** impaired hitpoint recovery; Spell Choice (You may choose your own spells to study); Shield Bash (You can bash monsters with a shield in melee); Combat Regeneration (You draw power from the thrill of combat, represented by Spell Points (SP). You gain SP when damaged by an enemy or when you attack in melee. As your blood cools your power fades; you lose SP at half the rate other classes gain them.  When you spend SP by casting a spell you regain some health, and fading SP replenish your health more efficiently. The more damaged you are, the bigger these health gains will be)

**Titles (by level band, every 5 levels):** Rat, Bully, Thug, Ruffian, Brigand, Raider, Tormentor, Marauder, Destroyer, Tyrant

**Starting kit:** 1× [Into the Shadows]; 1–3× Ration of Food; 1–3× Wooden Torch; 1× Tulwar; 1× Leather Shield; 1× Word of Recall (not with birth_no_recall); 1× Cure Light Wounds

**Magic realm:** shadow (rituals, cast from shadow books using INT; the verb used is "perform").

#### [Into the Shadows] — sold in town (Bookseller), base cost 25 gold, found at dungeon levels 1 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Seek Battle | 1 | 1 | 15 | 2 | Detects all monsters susceptible to fear in the immediate area, for one turn only. |
| Berserk Strength | 3 | 2 | 20 | 4 | Removes fear and grants you berserk strength (resistance to fear, +75 melee skill, -10 to AC, and prevents healing over time) for a level-dependent duration. |
| Whirlwind Attack | 5 | 4 | 25 | 6 | Deals a level-dependent number of melee blows to each adjacent enemy. At level 5 you do one blow and gain an additional blow every 15 levels (2 at 20, 3 at 35, and 4 at 50). |
| Shatter Stone | 7 | 3 | 20 | 8 | Destroys an adjacent section of wall or rubble, unless it is a permanent wall. Deals 20+1d30 damage to monsters vulnerable to rock remover. |
| Leap into Battle | 10 | 5 | 30 | 10 | You run up to 4 spaces towards the targeted enemy, then perform a melee blow. You gain an additional blow at levels 25 and 40. If you moved, the number of blows is reduced by 25% for each space, then rounded. |
| Grim Purpose | 13 | 8 | 30 | 12 | Your mind becomes too focused to be slowed, paralyzed or confused for 12+1d12 turns. |

#### [Fear and Torment] — sold in town (Bookseller), base cost 1600 gold, found at dungeon levels 30 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Maim Foe | 15 | 6 | 35 | 14 | Attempts a level-dependent number of melee blows against the targeted enemy. You get 1 blow for every 15 levels. If any of these blows damage the enemy, it will be stunned for 6 turns. |
| Howl of the Damned | 18 | 8 | 30 | 16 | Attempts to scare every monster in line of sight for a duration dependent on player level and monster depth. Monsters that resist fear are not affected. |
| Relentless Taunting | 24 | 10 | 32 | 20 | Makes any monster that can move more inclined to run toward you, and less likely to use a ranged attack (including spells and innate abilities). |
| Venom | 28 | 18 | 35 | 30 | Coats all your melee weapons with poison for 18+1d18 turns. |
| Werewolf Form | 32 | 30 | 40 | 100 | With a blood-curdling cry you change into a half-man half-wolf! In this form you move, attack, and heal faster. |

#### [Deadly Powers] — dungeon only, base cost 30000 gold, found at dungeon levels 40 to 100

| Spell | Level | Mana | Base fail % | First-cast XP factor | Description |
|---|---|---|---|---|---|
| Bloodlust | 30 | 25 | 37 | 80 | Gives you a powerful desire to kill, increased every time you satisfy it. Gives you extra melee damage and extra blows, but comes with dangerous side-effects. |
| Unholy Reprieve | 34 | 45 | 60 | 120 | Restores STR, INT, CON, and experience at the cost of 66 HP. |
| Forceful Blow | 38 | 16 | 50 | 150 | Attempts a melee blow that deals additional force damage when it hits, knocking the target back with a chance to stun it. |
| Quake | 44 | 39 | 55 | 175 | Causes a radius 13 earthquake centered around the player. |

---

## 21. Shapechanges

Druids (and a few spells/items for other classes) can change shape. While
shapechanged:

- you cannot use items in your pack or quiver, and can only interact with the
  floor to eat or pick up;
- equipped items stay worn and keep giving their stats, resistances, blows and
  damage, but you **cannot activate** them;
- the shape gives its own bonuses and penalties and its own melee attacks;
- to return to normal, cast a spell or use an item command (e.g. drop) — you
  will be asked whether to change back.

`~` → *shapechange effects* describes each shape in game. The shapes are:

- **fox** — to-hit -3, to-dam -3, AC +3; feather falling, free action; STR[-3], STEALTH[5], BLOWS[1], MOVES[1]; attacks: bite, claw
- **Pukel-man** — to-hit 0, to-dam +5, AC +20; save +20; sustain strength, sustain constitution, protection from stunning, regeneration, hold life; Rock; STR[4], CON[4], STEALTH[-2], SPEED[-5], RES_POIS[3], RES_SHARD[1]; DAM_RED[10]; attacks: punch, crush, thump, strike
- **bear** — to-hit +15, to-dam +15, AC +5; disarm-phys -5; disarm-magic -10; melee +10; protection from fear;  STR[3], INT[-2], CON[2], INFRA[1], STEALTH[-3], BLOWS[1]; attacks: cuff, clout, claw, hit, hit, swipe, slash
- **eagle** — to-hit +5, to-dam 0, AC +10; sustain wisdom, protection from blindness, protection from confusion, protection from fear, see invisible, free action; trap immunity; MOVES[3]; attacks: peck, claw, claw, strike, tear
- **bat** — to-hit 0, to-dam -10, AC +10; feather falling, see invisible, protection from blindness; SPEED[3], STEALTH[3], INFRA[5]; attacks: bite, scratch
- **warg** — to-hit +5, to-dam +5, AC 0; BLOWS[2]; attacks: bite, bite, claw, tear
- **vampire** — to-hit +3, to-dam +3, AC -5; see invisible, hold life; LIGHT[-1], STEALTH[5], INFRA[5], SPEED[5]; attacks: bite
- **werewolf** — to-hit +5, to-dam +5, AC 0; regeneration; BLOWS[1], MOVES[1]; attacks: bite, bite, slash, tear

(Value codes: STR/INT/CON etc. = stat bonus; STEALTH, INFRA, SPEED, LIGHT,
MOVES = extra moves per turn, BLOWS = extra blows, DAM_RED = damage reduction,
RES_x = resistance level.)

---

## 22. Monsters

There are about 600 kinds of monster, from harmless townsfolk to Morgoth.
Monsters may be awake or asleep, and noise (low stealth) wakes them. Some move
erratically, some never move, some pass through walls or bore through them,
some are invisible (needs *see invisible* or telepathy), some are cold-blooded
(invisible to infravision), some **breed explosively** (worm masses, lice,
hummerhorns — leave the level if they get out of hand), some drop treasure,
some steal, some summon. **Uniques** are named individuals who appear until
killed, once per game; with `purple_uniques` they are shown in purple.

### Monster memory

The game remembers what you have learned about each monster kind — attacks,
spells, resistances, speed, HP, drops, how many you have killed and how many
have killed your characters — across all your characters (in `lore.txt` in the
user directory). Look at a monster (`l`), use `/` on its symbol, or the
monster knowledge menu (`~`) to recall it. Spell and breath damage in the
recall is coloured: **red/orange** = you do not resist, **yellow** = partially,
**green** = resisted or immune. Detection and some spells reveal more lore
without fighting.

### Monster symbols

| Symbol | Monster type |
|---|---|
| `$` | Creeping Coins |
| `,` | Mushroom |
| `?` | Mimic |
| `@` | You |
| `A` | Ainu/Maia |
| `a` | Ant |
| `B` | Bird |
| `b` | Bat |
| `C` | Canine (Dog) |
| `c` | Centipede |
| `D` | Ancient Dragon/Wyrm |
| `d` | Dragon |
| `E` | Elemental |
| `e` | Floating Eye |
| `F` | Dragon Fly |
| `f` | Feline (Cat) |
| `G` | Ghost |
| `g` | Golem |
| `H` | Hybrid |
| `h` | Hobbit/Elf/Dwarf |
| `I` | Insect |
| `i` | Icky Thing |
| `J` | Snake |
| `j` | Jelly |
| `K` | Killer Beetle |
| `k` | Kobold |
| `L` | Lich |
| `l` | Tree/Ent |
| `M` | Hydra |
| `m` | Mold |
| `n` | Naga |
| `O` | Ogre |
| `o` | Orc |
| `P` | Giant Humanoid |
| `P` | Morgoth |
| `p` | Person/Human |
| `Q` | Quylthulg |
| `q` | Quadruped |
| `R` | Reptile/Amphibian |
| `r` | Rodent |
| `S` | Spider/Scorpion/Tick |
| `s` | Skeleton |
| `T` | Troll |
| `t` | Townsfolk |
| `U` | Major Demon |
| `u` | Minor Demon |
| `V` | Vampire |
| `v` | Vortex |
| `W` | Wight/Wraith |
| `w` | Worm/Worm Mass |
| `X` | Xorn/Xaren |
| `x` | Lurker |
| `Y` | Yeti |
| `y` | Yeek |
| `Z` | Zephyr Hound |
| `z` | Zombie/Mummy |

Mimics may also appear as `!`, `?`, `=`, `$`, `|` or other object symbols, and
lurkers/trappers look like floor. Hallucination makes everything look wrong.

This manual deliberately does not list individual monsters. For spoilers —
every monster, every unique, monster spells and all artifacts — see the
companion appendices in [`docs/strategy/`](strategy/README.md).

### The monster list and health bar

`[` lists all visible monsters with counts, sleep status and direction. The
sidebar shows the tracked monster's health bar (`[----------]`), coloured by
its health and status (asleep, afraid, confused, stunned, held…).

---

## 23. Options

Open the options menu with `=`. Its entries:

| Key | Entry |
|---|---|
| `a` | User interface options |
| `b` | Birth (difficulty) options |
| `x` | Cheat options |
| `w` | Subwindow setup |
| `i` | Item ignoring setup |
| `{` | Auto-inscription setup |
| `d` | Set base delay factor |
| `h` | Set hitpoint warning |
| `m` | Set movement delay |
| `o` | Set sidebar mode (Left / Top / None) |
| `s` | Save subwindow setup to pref file |
| `t` | Save autoinscriptions to pref file |
| `u` | Save char screen options to pref file |
| `p` | Load a user pref file |
| `e` | Edit keymaps (advanced) |
| `c` | Edit colours (advanced) |
| `v` | Save visuals (advanced) |

In the interface and birth option pages, `s` saves the current choices as the
default for future characters, `r` resets to the defaults for a new character,
and `x` resets to the maintainer's defaults. Options are stored in the savefile.

### User interface options

| Option (internal name) | Default | Effect |
|---|---|---|
| Use the roguelike command keyset (`rogue_like_commands`) | off | Roguelike keys (good without a numeric keypad). |
| Use autoexplore commands (`autoexplore_commands`) | off | `<`/`>` walk to the nearest known staircase of that kind when you aren't on one; `p` walks to the nearest unexplored location. |
| Use sound (`use_sound`) | off | Sound effects, if supported. |
| Show damage player deals to monsters (`show_damage`) | off | Damage numbers in messages, for melee, missiles, spells and devices; damage to you accounts for reduction/invulnerability. |
| Use old target by default (`use_old_target`) | off | Directional commands use the current target automatically. Dangerous with location targets. |
| Always pickup items (`pickup_always`) | off | Pick up items you walk onto when safe. |
| Always pickup items matching inventory (`pickup_inven`) | on | Pick up items that stack with something you carry. |
| Show flavors in object descriptions (`show_flavors`) | off | Show "Red Potion of …" even when known. |
| Highlight target with cursor (`show_target`) | on | |
| Highlight player with cursor between turns (`highlight_player`) | off | |
| Disturb whenever viewable monster moves (`disturb_near`) | on | Also when one comes into or goes out of view. Ignores telepathy. |
| Show walls as solid blocks (`solid_walls`) | off | Veins darker than granite. |
| Show walls with shaded background (`hybrid_walls`) | off | `#`/`%` on grey; overrides solid walls. |
| Color: Illuminate torchlight in yellow (`view_yellow_light`) | off | |
| Color: Shimmer multi-colored things (`animate_flicker`) | off | Multi-hued monsters/items shimmer in real time. |
| Center map continuously (`center_player`) | off | Otherwise the map scrolls by panels. |
| Color: Show unique monsters in purple (`purple_uniques`) | off | |
| Automatically clear '-more-' prompts (`auto_more`) | off | Convenient but you may miss messages. |
| Color: Player color indicates % hit points (`hp_changes_color`) | on | `@` goes from white to red as HP drops. |
| Allow mouse clicks to move the player (`mouse_movement`) | on | Click to pathfind. |
| Notify on object recharge (`notify_recharge`) | off | Like `!!` on every rechargeable item. |
| Show effective speed as multiplier (`effective_speed`) | off | e.g. `Fast (x2.0)` instead of `Fast (+10)`. |

### Birth options

Can only be set at character creation (or quick restart).

| Option (internal name) | Default | Effect |
|---|---|---|
| Generate a new, random artifact set (`birth_randarts`) | off | Random artifacts replacing the standard set, of comparable number and power. |
| Generate connected stairs (`birth_connect_stairs`) | on | Arrive on a staircase going the same way. Off: never start on stairs. |
| Force player descent (never make up stairs) (`birth_force_descend`) | off | Up stairs don't work; down stairs (including town) take you one level below your previous maximum; recall from town goes to max+1; cannot recall from a quest level until the quest is done; random up/down effects always go down. |
| Word of Recall has no effect (`birth_no_recall`) | off | With forced descent, this recreates the old "ironman" mode. |
| Restrict creation of artifacts (`birth_no_artifacts`) | off | No artifacts at all. |
| Stack objects on the floor (`birth_stacking`) | on | Off: dropped items spread out and vanish if there is no free grid within 3 squares. |
| Lose artifacts when leaving level (`birth_lose_arts`) | off | Off: an artifact you never saw may be generated again later. On: any artifact left on a level is gone forever. Artifacts you have seen/picked up are always lost if left behind. |
| Show level feelings (`birth_feelings`) | on | |
| Increase gold drops but disable selling (`birth_no_selling`) | on | Stores pay nothing but still identify; more gold in the dungeon. |
| Start with a kit of useful gear (`birth_start_kit`) | on | Off: more gold, buy your own gear. |
| Monsters learn from their mistakes (`birth_ai_learn`) | on | Monsters learn which of their spells you resist and choose better attacks. |
| Know all runes on birth (`birth_know_runes`) | off | All object properties known: artifacts and egos identified on sight. |
| Know all flavors on birth (`birth_know_flavors`) | off | All consumable flavours known. |
| Persistent levels (experimental) (`birth_levels_persist`) | off | Each level is remembered, including monsters, items and traps, when you return; recall lets you choose the depth. |
| To-damage is a percentage of dice (experimental) (`birth_percent_damage`) | off | Each +1 to-dam adds 5% to dice damage instead of +1. Not well balanced. |

### Cheat options

Using any of these marks the character so it will not appear on the score
list ("Cheaters never win").

| Option | Effect |
|---|---|
| `cheat_hear` — Peek into monster creation | Messages about monster generation. |
| `cheat_room` — Peek into dungeon creation | Messages about room/vault generation. |
| `cheat_xtra` — Peek into something else | Debugging messages. |
| `cheat_live` — Allow player to avoid death | On death you are fully healed and returned to town instead. |

Each cheat option has a hidden "score" twin that records it was ever used.

### Subwindow setup (`=` `w`)

For each sub-window choose any of: *Display inven/equip*, *Display
equip/inven*, *Display player (basic)* (stats and skills incl. blows/shots),
*Display player (extra)* (flags by equipment item), *Display player (compact)*,
*Display map view* (area around you or the target; lets tiles show at native
size), *Display messages*, *Display overhead view*, *Display monster recall*,
*Display object recall* (last selected object, e.g. spellbooks), *Display
monster list*, *Display status*, *Display item list*. You may need your front
end's own controls to make a sub-window visible.

### Other values

- **Base delay factor** (`=` `d`) — slows missile/bolt/ball animations; the
  delay is the value squared in milliseconds. Also controls Borg speed.
- **Hitpoint warning** (`=` `h`) — percentage of max HP at which you get a
  warning and HP/SP turn red (0 = off).
- **Movement delay** (`=` `m`, `lazymove_delay`) — lets you move diagonally by
  pressing two arrow keys within the delay (useful without a keypad).
- **Sidebar mode** (`=` `o`) — Left, Top or None.

## 24. Customising the game

You can customise: options (section 23), ignoring and inscriptions
(section 16), sub-windows, keymaps, visuals (symbols/colours/tiles), colours,
message colours and sounds, and front-end details (fonts, window layout,
tiles). Everything except options (stored in the savefile) and front-end
details (stored by the front end) can be saved to **user pref files**.

### User pref files

Plain-text files holding visuals, auto-inscriptions, keymaps, colours,
sub-window settings, message colours and sound mappings. The defaults are in
`lib/customize/` (`pref.prf`, `font.prf`, `message.prf`, `sound.prf`, and
per-front-end `font-*.prf`) — do not edit those; put your own in the user
directory. With graphics on, settings from `lib/tiles/` are also loaded.

After loading a character the game loads, in order: `pref.prf`, `font.prf`
(and tiles prefs), then from your user directory **`window.prf`**,
**`<Race>.prf`** (e.g. `Dwarf.prf`), **`<Class>.prf`** (e.g. `Paladin.prf`) and
**`<Name>.prf`** (e.g. `Balin.prf`) if present. So keymaps saved to `Mage.prf`
apply only to mages. Use `"` to enter a single pref line, `=` `p` to load a
file, and `Ctrl-R` to redraw after visual changes.

### Keymaps

A keymap maps one *trigger* key to a sequence of keys (the *action*). Under
`=` → `e` (Edit keymaps): `c` query a key, `d` create a keymap (press the
trigger, type the action, `Ctrl-U` to erase what you typed, `=` to finish,
then confirm), `e` remove a keymap, and save/load keymaps to a file. Keymaps
last only for the session unless saved.

- Put `(` … `)` around an action to suppress `-more-` prompts inside it.
- Keymaps belong to the keyset in force when created; you can bind the same
  trigger in both keysets.
- Keymaps are not recursive.
- Modifiers are written in braces before the key: `{^S}&` Ctrl-Shift-&,
  `{AK}0` Alt-keypad-0, `^d` Ctrl-d, `A` Shift-a. Special keys are in brackets:
  `[F1]`, `^[F1]`, `{^S}[Tab]`, `[Escape]`.
- In a pref file:

  ```
  keymap-act:maa
  keymap-input:0:[F1]
  keymap-input:1:[F1]
  ```

  (`0` = original keyset, `1` = roguelike; several input lines may share one
  action.)
- A multi-command keymap (e.g. `m1dm1f` to cast two prayers) stops early if you
  are disturbed, if the next key is not a command, if a command's prerequisite
  is missing (no launcher, can't read…), or if an inscription confirmation is
  refused.
- If an action starts with Escape, its trigger can break out of a direction,
  target, item, spell, curse or effect prompt (not text or yes/no prompts); the
  rest of the action is then discarded.
- The built-in keymaps (movement keys → `;` + direction, etc.) are in
  `pref.prf`.

### Visuals and colours

Edit how monsters, objects, features and traps look from the knowledge menus
(`~`): the prompt tells you when visuals can be edited (characters/colours in
text mode, tiles in graphics mode). Save with `=` → `v`. Colours themselves
can be tweaked under `=` → `c`: `n`/`N` next/previous colour, `r` `g` `b`
(upper/lower case) adjust components; save to a pref file. (A common fix is
brightening the dark colours used for e.g. cave spiders.)

---

## 25. Front ends

Each front end controls fonts, window placement and tiles its own way.

**Windows** — several sub-windows shown by default with David Gervais's tiles.
Window menu: *Visibility* (show/hide sub-windows), *Font*, *Term Options*
(sub-window contents), *Reset Layout*, *Bizarre Display* (alternative text
drawing; leave on unless text is garbled), Increase/Decrease Tile Width/Height,
*Term 0 Font Tile Size*. Options menu: *Graphics* (None or a tile set),
*Enable Nice Graphics* (auto tile sizing), *Tile Multiplier*. Settings are
saved in `angband.INI` next to the executable. Closing the main window saves
and exits.

**X11** — `./angband -mx11 -- -n4` opens the main window and three
sub-windows. Per window *z* (0–7) set environment variables
`ANGBAND_X11_FONT_z`, `ANGBAND_X11_AT_X_z`, `ANGBAND_X11_AT_Y_z`,
`ANGBAND_X11_COLS_z`, `ANGBAND_X11_ROWS_z`. X11 needs legacy bitmap fonts
(see the FAQ).

**SDL** — all terminals inside one application window. The top status bar has
buttons: version/about, current terminal, visibility toggle, font, Options
(tile set, fullscreen), Quit. Drag a title bar to move a terminal; drag the
lower-right corner to resize. Saved to `sdlinit.txt` in the user directory.

**SDL2** — one main application window plus up to three more, each holding
any sub-windows. Menu bar: *Menu* (per-terminal font, tiles, borders,
always-on-top, alpha, purpose; Fullscreen; *Send Keypad Modifier*; *Menu
Shortcuts…*; *Windows*; About; SDL Details; Quit), one-letter terminal toggles,
and *Move* / *Size* toggles for rearranging. Saved to `sdl2init.txt`.

**macOS (Cocoa)** — standard window controls; reopen closed sub-windows from
the *Window* menu; fonts via *Settings → Edit Font*; tiles via *Settings →
Graphics* ("Classic ASCII" for text) and *Tile Size*. Preferences in
`~/Library/Preferences/org.rephial.angband.plist`; reset with
`defaults delete org.rephial.angband`.

**Curses (GCU)** — plays in a text terminal: `./angband -mgcu`. Run
`./angband -mgcu -- -h` for its options (e.g. extra sub-windows in a large
terminal). `Ctrl-Z` suspends.

---

## 26. Death, retirement, winning and the high-score list

**Death.** At 0 HP or below you die (Blackguards can sometimes survive below
zero for a while). A tombstone appears; you may view or save a full character
record with everything identified. The savefile keeps only your options;
loading it creates a new character with the same settings.

**Cheating death.** `cheat_live` (and the debug-mode resurrection) heal you and
return you to town — but mark the character as a cheater, off the score list.

**Winning.** Kill Sauron on level 99; a staircase to level 100 appears. Morgoth
lives there (and tends to come looking for you with Grond in hand). Kill him
and you are a **WINNER**. You may keep playing — rumours speak of a powerful
ring and a dragon armour that resists everything — but you can still die until
you retire.

**Retiring.** `Q` retires the character and records it on the score list
(winners are recorded as winners).

**Scores.** The hall of fame (`~`) ranks characters by how far they got.
Characters that used cheat options or debug mode are excluded; Borg
characters are excluded unless the game was compiled with `SCORE_BORGS`.

---

## 27. Strategy guide

Condensed advice from the bundled *Players' Guide* (written for 3.5 but still
largely valid) and long community experience.

### The basics

- Your only obligatory tasks: **go down a lot of stairs, kill Sauron (dl 99),
  kill Morgoth (dl 100)**. You do not need to clear levels or fight anything
  else. Treat the game as raids for loot, not a killing spree.
- **Don't take unnecessary risks.** Enough small chances of death guarantee
  death. Anything that can kill you in one turn should be avoided or escaped.
- **Leave levels freely.** Don't like the feeling, the monsters, or the vault
  guardians? Take the stairs. There is always another level.
- **Depth is the main source of better items and experience.** Diving
  (sensibly) is often *safer* than lingering shallow, and character level comes
  quickly deeper down.

### Starting out

- Begin as a **Warrior** (simplest) — e.g. Dúnadan, High-Elf, Dwarf, Half-Orc
  or Half-Troll; aim for 3–4 blows with a light weapon (Dagger, Main Gauche,
  Rapier), raising DEX to 18/10 and STR to 17–18 with point-based stats.
- Damage output, not HP, is the measure of a fighter. Buff with Heroism,
  Blessing and Berserk Strength for hard fights; get the first blow.
- Always carry: **Word of Recall**, **3+ Phase Door**, **Cure Light/Serious
  Wounds** potions (also cure blindness/confusion), **Heroism** (fear), **flasks
  of oil** to throw at dangerous/valuable targets, and a stack of shots/arrows.
- Don't spend early gold on armour; you'll find plenty. Starting kits include
  enough food and light for a first trip.
- A first trip to dl 5–10 is realistic. Return when you run out of escapes,
  cures or ammunition.

### What to fight, ignore and avoid

- **Fight**: groups you can defeat quickly that give good XP or drops, uniques
  with good drops (Bullroarer, Grip/Fang, Wormtongue…), and easy kills.
- **Ignore**: monsters that destroy gear (jellies, water hounds), low-value
  monsters that waste consumables, mobs you can't dominate, uniques with escorts
  you can't handle, runaway breeders (close the door and leave), low-depth
  monsters with poor drops.
- **Avoid**: anything that can kill you in a turn.

### Habits that keep characters alive

- **Focus.** Don't play tired. Use detection on arrival; know what's around.
- **Use your stuff.** Potions, scrolls, wands, rods, activations exist to be
  used — an unused Potion of Healing is worth nothing when you die.
- **Rangers shoot; mages cast.** Don't play a ranger like a warrior, or a mage
  like Gandalf with a sword.
- **Stockpile** essentials (Phase, CLW/CSW, food, light, Recall, spare books).
  Monsters steal and elements destroy.
- **Carry escapes that work now** — Phase Door, Teleportation, Teleport Level,
  Deep Descent — Recall is too slow for emergencies.
- **Light is life**; never drop your light for loot. Necromancers excepted.
- **Identify safely**: try unknown scrolls on stairs; test devices on weak
  monsters; sell/give unknown potions to learn them if you're cautious.
- **Check the Black Market** each visit for stat potions and speed.
- **Turn on sub-windows** (monster list, messages, equipment) and use a bigger
  font — information at a glance saves lives.
- **Take breaks** after big wins or close calls.
- **Keep shooting** even when your melee is strong.
- Entering levels by stairs (recall up, walk down) guarantees you start on a
  staircase — a free escape route.
- **Don't let a full pack stop you diving**: drop low-value loot.

### Key milestones

Free action (by dl ~20–30), see invisible (by ~30), resist the four base
elements and poison, protection from confusion and blindness, a good speed
item, and plenty of Cure Critical / Healing potions before the deep levels.

---

## 28. The Borg — Angband's automatic player

The **Borg** ("APW Borg", named after Star Trek's cyborgs) is an
**automatic Angband player** built into the game. It was first written for
2.8.0 as a separate program, merged into the game around 3.3, removed in 4.0,
and reintegrated around 4.2.3; this branch carries a large number of Borg fixes
and improvements. Its source is in `src/borg/` (about 80 modules: danger
assessment, fighting, flowing/pathing, item valuation, shopping, spell use…).
It is mainly entertainment — it's fun to watch — and also a powerful testing
tool that finds bugs by playing millions of turns.

> **Warning:** don't run the Borg on a character you care about. Use a
> separate savefile or a copy. Borg characters are flagged and do not go on
> the score list (unless compiled with `SCORE_BORGS`).

### Enabling it

The Borg is compiled in when `ALLOW_BORG` is defined — the default for most
builds and official binaries. If it's missing, enable `ALLOW_BORG` in
`config.h` or pass `-DALLOW_BORG` to the compiler. The optional
`SCORE_BORGS` define lets Borg characters onto the high-score list.

### Running it

1. Start or load a game (preferably a throwaway character).
2. Press **`Ctrl-Z`** (or `^` then `z`) for the Borg command prompt. The first
   time, you get a warning that continuing marks the character; accept it.
3. Press **`z`** to activate the Borg. It now plays on its own.
4. Press **any key** to stop it and take back control.

If it moves too slowly, lower the **base delay factor** (`=` `d`); to slow it
down, raise that or set `borg_delay_factor` in `borg.txt` (the delay is
`base_delay² + borg_delay²` ms).

### Borg commands (after `Ctrl-Z`)

| Key | Command |
|---|---|
| `?` | Help |
| `z` | Activate the Borg |
| `x` | Step the Borg (one action) |
| `u` | Update the Borg's variables (as if taking zero steps) |
| `c` | Toggle the cheat-death flag (then `d`) |
| `C` | Count of "nasties" (dangerous monster types) |
| `f` | Toggle flags (change any Borg configuration value at run time) |
| `F` | Fear levels of the current location |
| `G` | Display features of selected grids |
| `I` | Display information for selected grids |
| `D` | Display danger of selected grids |
| `h` | `borg_has` — what the Borg owns: `a` any, `i` inventory, `w` worn, `r` artifacts, `s` skills (listed in messages) |
| `l` | Create a snapshot log (`<name>.map` in the archive directory) |
| `m` | Map information: `a` avoidances (danger), `f` features, `g` glyphs, `m` monsters, `o` objects |
| `o` | Object information |
| `p` | Borg "power" (its evaluation of the character) |
| `P` | Level preparation information (what it needs before diving deeper) |
| `R` | Respawn the Borg (new character) |
| `s` | Search mode: stop when a given string appears in messages (default "plain gold ring" — The One Ring) |
| `S` | Dump spell information |
| `t` | Display targeting |
| `v` | Version stamp |
| `w` | Swap weapon/armour information |
| `y` | Last 75 steps |
| `!` | Time |
| `$` | Reload `borg.txt` |
| `@` | Borg line of sight |
| `^` | Flow pathway |
| `0` | Borg stats (STR/INT etc.) |

### Configuring it: `borg.txt`

The Borg reads `borg.txt` from your user directory (`~/.angband/Angband/`,
`~/Documents/Angband/`, or `lib/user/`). If none exists, a stripped-down one is
generated on first run. The full commented sample is `src/borg/borg.txt` (on
Linux binary packages it may be in `/usr/share/doc/angband/`; on macOS in the
`.dmg`; on Windows it is already in `lib/user`). Edit it and reload with
`Ctrl-Z` `$`. Settings (default in brackets):

| Setting | Meaning |
|---|---|
| `borg_worships_damage`, `_speed`, `_hp`, `_mana`, `_ac` [FALSE] | Give extra weight to that attribute when choosing equipment. |
| `borg_worships_gold` [FALSE] | Return to town to sell when full (more early income; may sell unidentified artifacts; climbs stairs instead of recalling; recalls only from dl 8; no effect after clevel 20). |
| `borg_plays_risky` [FALSE] | Dive faster than its level would suggest and stay in fights longer. |
| `borg_kills_uniques` [FALSE] | Don't dive more than about three uniques' depths past living uniques. |
| `borg_uses_swaps` [TRUE] | Carry swap weapons/armour for missing resistances (costs pack slots, allows faster diving). |
| `borg_allow_strange_opts` [FALSE] | Run even with birth options it doesn't handle well. |
| `borg_cheat_death` [FALSE] | Set cheat-death and keep playing after dying. If false but the game's cheat-death is on, it respawns a new character; if both false it stops on death. |
| `borg_respawn_race`, `borg_respawn_class` [-1 = random] | Race/class index for respawned characters (races 0 Human … 10 Kobold; classes 0 Warrior … 8 Blackguard). |
| `borg_respawn_winners` [FALSE] | After killing Morgoth, start a new character (a map file of the final battle is saved). |
| `borg_dump_level` [1], `borg_save_death` [1] | Minimum clevel for a death character dump; whether to save the dead game in the archive directory. |
| `borg_autosave` [FALSE] | Extra save (and archive backup) at the start of every level. |
| `borg_stop_dlevel` [128], `borg_stop_clevel` [51], `borg_no_deeper` [127], `borg_stop_king` [TRUE] | Auto-stop at a depth/level, never go deeper than a depth, stop after winning. |
| `borg_stop_on_bell` [FALSE] | Stop when the game rings the error bell (debugging; logs the last 50 keypresses). |
| `borg_chest_fail_tolerance` [7] | Acceptable failure risk when disarming/opening chests. |
| `borg_delay_factor` [0] | Borg-specific delay. |
| `borg_money_scum_amount` [0] | Stay in town killing townspeople until it has this much gold (needs permanent light; flees scary town monsters). |
| `borg_self_scum` [TRUE] | Allowed to scum town for gold to buy an item it wants (e.g. a BM stat potion); needs clevel 10+, permanent light, good theft avoidance, food. |
| `borg_lunal_mode` [FALSE] | Dive by stair-scumming while low level, recall up, walk down again; a one-way trip. |
| `borg_self_lunal` [FALSE] | Use stair-scumming on its own initiative to return to a comfortable depth. |
| `borg_verbose` [FALSE] | Very chatty messages about its reasoning. |
| `borg_munchkin_start` [FALSE], `borg_munchkin_level` [12], `borg_munchkin_depth` [16] | Early stair-scumming at shallow depth for loot until a level/depth is reached. |
| `borg_enchant_limit` [12] | How far it enchants equipment with scrolls/spells. |
| `borg_restore_ignore_settings` | Keep the ignore settings from before the Borg ran. |
| `borg_uses_dynamic_calcs` [FALSE] | Use the formula section of `borg.txt` instead of the built-in C logic (more customisable, slower, less maintained). |

### Watching and logging

The Borg suppresses most messages. Open sub-windows for **equipment**,
**inventory**, **messages** and **monster recall** before starting it.
Turn on `borg_verbose` (in `borg.txt`, or `Ctrl-Z` `f`) for detailed output on
its decisions. `Ctrl-Z` `l` writes a snapshot to `<name>.map` in the archive
directory: an ASCII map, recent messages, full equipment/inventory/quiver/home,
swap items and settings, and every internal trait value.

### Borg screensaver (Windows)

A renamed Windows executable (`angband.scr`) can run the Borg as a
screensaver in continuous-play mode, respawning on death. Copy `angband.scr`
and its `angband.ini` to the Windows directory, set `AngbandPath` (ending with
`\`) and `SaverFile` (a character name; created if absent) in the ini, and test
in Display Properties. It runs at low priority (toggle with *Options → Low
priority* when used as a normal exe). Limitations: no preview, no password
protection, manual ini editing, "show scores" while running may crash, the same
savefile cannot be open twice. Screensaver Borgs never get high-score entries.

---

## 29. Debug (wizard) mode

Debug commands are for testing and cheating; using them marks the character
as a cheater (no high score). Press `Ctrl-A` and then a command key (you are
asked to confirm the first time). `Ctrl-W` toggles wizard mode itself.

| Key | Command |
|---|---|
| `c` | Create any object (menu), dropped at your feet |
| `C` | Create any artifact |
| `g` | Create N good objects nearby |
| `v` | Create N very good (excellent) objects nearby |
| `o` | Play with an object: reroll as normal/good/excellent, edit quantity, ego, curses, combat values, modifiers; generation statistics |
| `V` | Test kind: one of each sval of a tval (by number), including instant artifacts |
| `d` | Detect everything nearby |
| `m` | Magic map the area |
| `l` | Learn all object kinds up to level 100 |
| `r` | Full monster recall (all or one) |
| `W` | Wipe monster recall (all or one) |
| `u` | Reveal all monsters |
| `w` | Wizard-light the level |
| `"` | Write spoiler files (objects or monsters) |
| `j` | Teleport to any dungeon level |
| `p` | Phase door (10 squares) |
| `t` | Teleport (100 squares) |
| `b` | Teleport to the target grid |
| `a` | Cure everything, restore stats/XP/HP/SP, sate hunger |
| `A` | Advance to level 50, max stats, 1,000,000 gold |
| `e` | Edit base stats, XP and gold |
| `x` | Gain experience (up to 9999) |
| `h` | Re-roll hit points |
| `n` | Summon a named (or numbered) monster |
| `s` | Summon N random monsters |
| `z` | Delete all monsters within a distance |
| `H` | Hit every monster in line of sight for 10,000 damage |
| `T` | Create a trap on your grid |
| `E` | Perform any effect with chosen parameters |
| `X` | Quit without saving |
| `q` | Highlight grids with a given square flag |
| `F` | Highlight grids of a given terrain type |
| `f` / `S` | Collect level-generation statistics into `stats.log` |
| `D` | Disconnection statistics (`disconnect.html`, `disconnect_gstat.txt`) |
| `P` | Pit/nest statistics |
| `_` | Show sound/scent flow distances |
| `>` | Push objects off the targeted grid |
| `M` | Write an HTML map of the level |
| `G` | Projection animation demo |
| `L` | Recent keystroke log |

The command-line `-w` option resurrects a dead character in a similar
cheating way.

---

## 30. Frequently asked questions

**How do I get help in the game?** Press `?`. Press Enter for the command menu.

**How do I report a bug?** On the forum (<https://angband.live/forums/>) or
GitHub, with your OS, the version, and steps to reproduce; a savefile helps.

**Dark monsters are hard to see.** Adjust your screen, or brighten the dark
colours under `=` → `c` (move with `n` to the colour, adjust with `r`/`g`/`b`).

**"Couldn't load the requested X11 font (10x20)".** The X11 front end needs
legacy bitmap fonts: install e.g. `xfonts-base` (Debian/Ubuntu),
`xorg-x11-fonts-misc` (Red Hat family) or `xorg-fonts-misc` (Arch), or set
`ANGBAND_X11_FONT` to an installed bitmap font, or use SDL/SDL2/GCU instead.

**How do I stop the menu that pops up when I press Enter?** In `=` → `e`
create a keymap with Enter as the trigger and a single space as the action;
save it to a pref file that loads automatically. You can map Tab to `\n` if you
still want the menu on another key.

**My spellbook got burned and now `maa` casts the wrong thing!** Inscribe
books `@m1`, `@m2`… and cast with `m1a`.

**Why can't I sell anything?** `birth_no_selling` is on by default; you get
more gold from the dungeon instead. Stores still identify items you give them.

**I can't find the stairs.** Look for secret doors, use Magic Mapping / stair
detection, Deep Descent or Teleport Level, or dig. With `autoexplore_commands`
`>` walks to the nearest known down stair and `p` explores.

**My character is stuck in a sealed area.** Dig out (always carry a digger),
or use teleportation, Stone to Mud, Deep Descent or Recall.

**Where is my savefile / pref files?** See [Appendix H](#appendix-h--files-and-directories).

**How do I contribute?** Pull requests on GitHub or patches on the forum;
contributions are dual-licensed under the Angband licence and GPLv2. See
`CONTRIBUTING.md`.

---

## Appendix A — Experience table

Experience needed to *reach* each level for a race/class with a 100%
experience factor (Human). Multiply by your race's XP% (e.g. ×1.2 for most
races, ×1.45 for High-Elves).

| Level | XP (100%) | XP (120%) | XP (145%) |
|---|---|---|---|
| 2 | 10 | 12 | 14 |
| 3 | 25 | 30 | 36 |
| 4 | 45 | 54 | 65 |
| 5 | 70 | 84 | 101 |
| 6 | 100 | 120 | 145 |
| 7 | 140 | 168 | 203 |
| 8 | 200 | 240 | 290 |
| 9 | 280 | 336 | 406 |
| 10 | 380 | 456 | 551 |
| 11 | 500 | 600 | 725 |
| 12 | 650 | 780 | 942 |
| 13 | 850 | 1,020 | 1,232 |
| 14 | 1,100 | 1,320 | 1,595 |
| 15 | 1,400 | 1,680 | 2,030 |
| 16 | 1,800 | 2,160 | 2,610 |
| 17 | 2,300 | 2,760 | 3,335 |
| 18 | 2,900 | 3,480 | 4,205 |
| 19 | 3,600 | 4,320 | 5,220 |
| 20 | 4,400 | 5,280 | 6,380 |
| 21 | 5,400 | 6,480 | 7,830 |
| 22 | 6,800 | 8,160 | 9,860 |
| 23 | 8,400 | 10,080 | 12,180 |
| 24 | 10,200 | 12,240 | 14,790 |
| 25 | 12,500 | 15,000 | 18,125 |
| 26 | 17,500 | 21,000 | 25,375 |
| 27 | 25,000 | 30,000 | 36,250 |
| 28 | 35,000 | 42,000 | 50,750 |
| 29 | 50,000 | 60,000 | 72,500 |
| 30 | 75,000 | 90,000 | 108,750 |
| 31 | 100,000 | 120,000 | 145,000 |
| 32 | 150,000 | 180,000 | 217,500 |
| 33 | 200,000 | 240,000 | 290,000 |
| 34 | 275,000 | 330,000 | 398,750 |
| 35 | 350,000 | 420,000 | 507,500 |
| 36 | 450,000 | 540,000 | 652,500 |
| 37 | 550,000 | 660,000 | 797,500 |
| 38 | 700,000 | 840,000 | 1,015,000 |
| 39 | 850,000 | 1,020,000 | 1,232,500 |
| 40 | 1,000,000 | 1,200,000 | 1,450,000 |
| 41 | 1,250,000 | 1,500,000 | 1,812,500 |
| 42 | 1,500,000 | 1,800,000 | 2,175,000 |
| 43 | 1,800,000 | 2,160,000 | 2,610,000 |
| 44 | 2,100,000 | 2,520,000 | 3,045,000 |
| 45 | 2,400,000 | 2,880,000 | 3,480,000 |
| 46 | 2,700,000 | 3,240,000 | 3,915,000 |
| 47 | 3,000,000 | 3,600,000 | 4,350,000 |
| 48 | 3,500,000 | 4,200,000 | 5,075,000 |
| 49 | 4,000,000 | 4,800,000 | 5,800,000 |
| 50 | 4,500,000 | 5,400,000 | 6,525,000 |

## Appendix B — Object kinds

Every object kind defined in `lib/gamedata/object.txt`, grouped by type, with
native level (typical depth), weight, base cost, and damage dice / base AC /
multiplier where relevant. Flavoured items appear here by their true names.

#### Light  (`~`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Wooden Torch | 1 | 2.2 | 2 |
| Lantern | 3 | 5 | 35 |

#### Flask  (`!`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Flask of oil | 1 | 2 | 3 |

#### Food  (`,`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Hard Biscuit | 0 | 0.2 | 1 |
| Apple | 0 | 0.4 | 3 |
| Handful of Dried Fruits | 0 | 0.3 | 3 |
| Slime Mold | 1 | 0.2 | 2 |
| Ration of Food | 0 | 0.8 | 3 |
| Scrap of Flesh | 0 | 0.5 | 0 |
| Slice of Meat | 0 | 1 | 3 |
| Honey-cake | 5 | 1 | 10 |
| Piece of Elvish Waybread | 5 | 0.3 | 10 |
| Flask of Whisky | 0 | 0.5 | 1 |
| Pint of Fine Wine | 0 | 1 | 2 |
| Sip of Miruvor | 0 | 0.2 | 100 |
| Swig of Orcish Liquor | 0 | 0.2 | 1 |
| Draught of the Ents | 0 | 1 | 2000 |

#### Mushroom  (`,`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Second Sight | 0 | 0.1 | 200 |
| Fast Recovery | 0 | 0.1 | 200 |
| Vigor | 0 | 0.1 | 200 |
| Clear Mind | 0 | 0.1 | 200 |
| Emergency | 0 | 0.1 | 60 |
| Terror | 0 | 0.1 | 60 |
| Stoneskin | 0 | 0.1 | 60 |
| Turbulence | 0 | 0.1 | 60 |
| Debility | 0 | 0.1 | 20 |
| Sprinting | 0 | 0.1 | 100 |
| Purging | 0 | 0.1 | 60 |
| Shadows | 0 | 0.1 | 300 |

#### Sword  (`|`)

| Name | Native level | Weight (lb) | Base cost | Damage dice |
|---|---|---|---|---|
| Dagger | 0 | 1.2 | 30 | 1d4 |
| Main Gauche | 3 | 3 | 45 | 1d5 |
| Rapier | 5 | 4 | 70 | 1d6 |
| Short Sword | 5 | 8 | 90 | 1d7 |
| Cutlass | 5 | 11 | 120 | 1d8 |
| Tulwar | 5 | 10 | 170 | 2d4 |
| Scimitar | 10 | 15 | 300 | 4d2 |
| Long Sword | 10 | 13 | 300 | 2d5 |
| Broad Sword | 10 | 15 | 300 | 2d5 |
| Bastard Sword | 15 | 14 | 400 | 3d4 |
| Katana | 20 | 12 | 600 | 3d5 |
| Zweihander | 30 | 20 | 700 | 3d6 |
| Executioner's Sword | 40 | 26 | 850 | 4d5 |
| Blade of Chaos | 70 | 18 | 4000 | 6d5 |

#### Hafted  (`\`)

| Name | Native level | Weight (lb) | Base cost | Damage dice |
|---|---|---|---|---|
| Whip | 3 | 3 | 30 | 1d3 |
| Ball-and-Chain | 20 | 15 | 200 | 2d4 |
| Morning Star | 10 | 15 | 396 | 2d6 |
| Flail | 10 | 15 | 353 | 2d6 |
| Two-Handed Great Flail | 45 | 28 | 590 | 3d6 |
| Mace | 5 | 12 | 130 | 2d4 |
| Lead-Filled Mace | 15 | 18 | 502 | 4d3 |
| Quarterstaff | 10 | 15 | 200 | 1d9 |
| Throwing Hammer | 40 | 5.5 | 250 | 2d4 |
| War Hammer | 5 | 12 | 225 | 3d3 |
| Maul | 5 | 20 | 130 | 4d4 |
| Great Hammer | 5 | 18 | 500 | 8d1 |
| Mace of Disruption | 80 | 40 | 4300 | 5d8 |
| Mighty Hammer | 15 | 20 | 1000 | 3d9 |

#### Polearm  (`/`)

| Name | Native level | Weight (lb) | Base cost | Damage dice |
|---|---|---|---|---|
| Spear | 5 | 5 | 36 | 1d6 |
| Awl-Pike | 10 | 16 | 340 | 1d8 |
| Trident | 5 | 7 | 120 | 1d10 |
| Halberd | 25 | 19 | 430 | 3d5 |
| Pike | 15 | 16 | 358 | 2d5 |
| Throwing Axe | 25 | 4 | 150 | 2d3 |
| Beaked Axe | 15 | 18 | 408 | 2d6 |
| Broad Axe | 15 | 16 | 304 | 2d6 |
| Battle Axe | 15 | 17 | 334 | 2d8 |
| Lochaber Axe | 45 | 25 | 750 | 3d8 |
| Great Axe | 40 | 23 | 500 | 4d4 |
| Scythe | 45 | 25 | 800 | 5d3 |
| Glaive | 20 | 19 | 363 | 2d6 |
| Lance | 10 | 30 | 230 | 2d8 |
| Scythe of Slicing | 60 | 25 | 3500 | 8d4 |
| Lucerne Hammer | 10 | 12 | 376 | 2d5 |

#### Bow  (`}`)

| Name | Native level | Weight (lb) | Base cost | Multiplier |
|---|---|---|---|---|
| Sling | 1 | 0.5 | 5 | x? |
| Short Bow | 3 | 3 | 50 | x? |
| Long Bow | 10 | 4 | 120 | x? |
| Light Crossbow | 15 | 11 | 140 | x? |
| Heavy Crossbow | 30 | 20 | 300 | x? |

#### Arrow  (`{`)

| Name | Native level | Weight (lb) | Base cost | Damage dice |
|---|---|---|---|---|
| Arrow | 3 | 0.2 | 1 | 1d4 |
| Seeker Arrow | 55 | 0.2 | 20 | 4d4 |
| Mithril Arrow | 55 | 0.1 | 25 | 3d4 |

#### Bolt  (`{`)

| Name | Native level | Weight (lb) | Base cost | Damage dice |
|---|---|---|---|---|
| Bolt | 3 | 0.3 | 2 | 1d5 |
| Seeker Bolt | 65 | 0.3 | 25 | 4d5 |
| Mithril Bolt | 50 | 0.2 | 30 | 3d5 |

#### Shot  (`{`)

| Name | Native level | Weight (lb) | Base cost | Damage dice |
|---|---|---|---|---|
| Rounded Pebble | 0 | 0.4 | 1 | 1d2 |
| Iron Shot | 3 | 0.5 | 2 | 1d4 |
| Mithril Shot | 40 | 0.2 | 20 | 2d4 |

#### Digger  (`\`)

| Name | Native level | Weight (lb) | Base cost | Damage dice |
|---|---|---|---|---|
| Shovel | 1 | 6 | 10 | 1d2 |
| Pick | 30 | 18 | 300 | 1d3 |
| Mattock | 50 | 25 | 700 | 1d8 |

#### Boots  (`]`)

| Name | Native level | Weight (lb) | Base cost | Base AC |
|---|---|---|---|---|
| Pair of Leather Sandals | 0 | 1.5 | 4 | 1 |
| Pair of Leather Boots | 3 | 2 | 7 | 2 |
| Pair of Iron Shod Boots | 5 | 3.5 | 12 | 4 |
| Pair of Steel Shod Boots | 20 | 6 | 50 | 7 |
| Pair of Mithril Shod Boots | 60 | 4 | 500 | 8 |
| Pair of Ethereal Slippers | 0 | 0 | 4 | 0 |

#### Helm  (`]`)

| Name | Native level | Weight (lb) | Base cost | Base AC |
|---|---|---|---|---|
| Hard Leather Cap | 3 | 2 | 12 | 2 |
| Metal Cap | 10 | 2.5 | 30 | 3 |
| Iron Helm | 20 | 5 | 75 | 7 |
| Steel Helm | 40 | 6 | 200 | 9 |

#### Crown  (`]`)

| Name | Native level | Weight (lb) | Base cost | Base AC |
|---|---|---|---|---|
| Iron Crown | 45 | 2 | 500 | 0 |
| Golden Crown | 45 | 3 | 1000 | 0 |
| Jewel Encrusted Crown | 50 | 4 | 2000 | 0 |
| Massive Iron Crown | 44 | 2 | 1000 | 0 |

#### Soft Armor  (`(`)

| Name | Native level | Weight (lb) | Base cost | Base AC |
|---|---|---|---|---|
| Robe | 1 | 2 | 4 | 2 |
| Soft Leather Armour | 3 | 8 | 20 | 8 |
| Studded Leather Armour | 10 | 10 | 100 | 12 |
| Hard Leather Armour | 5 | 12 | 150 | 16 |
| Leather Scale Mail | 15 | 14 | 370 | 20 |

#### Hard Armor  (`[`)

| Name | Native level | Weight (lb) | Base cost | Base AC |
|---|---|---|---|---|
| Metal Scale Mail | 25 | 25 | 550 | 38 |
| Chain Mail | 25 | 22 | 750 | 32 |
| Augmented Chain Mail | 30 | 27 | 900 | 42 |
| Bar Chain Mail | 35 | 28 | 950 | 45 |
| Metal Brigandine Armour | 35 | 29 | 1100 | 48 |
| Partial Plate Armour | 45 | 26 | 1200 | 40 |
| Metal Lamellar Armour | 45 | 34 | 1250 | 58 |
| Full Plate Armour | 45 | 36 | 1350 | 62 |
| Ribbed Plate Armour | 50 | 38 | 1500 | 66 |
| Mithril Chain Mail | 55 | 15 | 7000 | 28 |
| Mithril Plate Mail | 60 | 30 | 15000 | 60 |
| Adamantite Plate Mail | 75 | 42 | 20000 | 80 |

#### Cloak  (`(`)

| Name | Native level | Weight (lb) | Base cost | Base AC |
|---|---|---|---|---|
| Cloak | 1 | 1 | 3 | 1 |
| Fur Cloak | 1 | 2.5 | 600 | 3 |
| Elven Cloak | 60 | 0.5 | 2000 | 6 |
| Ethereal Cloak | 50 | 0 | 4500 | 0 |

#### Gloves  (`]`)

| Name | Native level | Weight (lb) | Base cost | Base AC |
|---|---|---|---|---|
| Set of Leather Gloves | 0 | 1 | 5 | 1 |
| Set of Gauntlets | 0 | 2.5 | 45 | 3 |
| Set of Mithril Gauntlets | 0 | 1.5 | 700 | 6 |
| Set of Caestus | 0 | 4 | 100 | 5 |
| Set of Alchemist's Gloves | 0 | 0.5 | 500 | 0 |

#### Shield  (`)`)

| Name | Native level | Weight (lb) | Base cost | Base AC |
|---|---|---|---|---|
| Wicker Shield | 1 | 3 | 20 | 2 |
| Small Metal Shield | 10 | 6 | 80 | 5 |
| Leather Shield | 15 | 9 | 200 | 8 |
| Large Metal Shield | 30 | 12 | 500 | 12 |
| Knight's Shield | 50 | 16 | 750 | 15 |
| Mithril Shield | 70 | 10 | 10000 | 16 |

#### Ring  (`=`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Strength | 30 | 0.2 | 500 |
| Intelligence | 30 | 0.2 | 500 |
| Dexterity | 30 | 0.2 | 500 |
| Constitution | 30 | 0.2 | 500 |
| Speed | 75 | 0.2 | 100000 |
| Searching | 5 | 0.2 | 250 |
| Bodykeeping | 30 | 0.2 | 750 |
| Soulkeeping | 30 | 0.2 | 600 |
| Resist Poison | 40 | 0.2 | 16000 |
| Resist Fire and Cold | 10 | 0.2 | 250 |
| Light | 0 | 0.2 | 400 |
| Flames | 30 | 0.2 | 500 |
| Acid | 25 | 0.2 | 500 |
| Ice | 28 | 0.2 | 500 |
| Lightning | 32 | 0.2 | 500 |
| Damage | 20 | 0.2 | 500 |
| Accuracy | 20 | 0.2 | 500 |
| Slaying | 40 | 0.2 | 1000 |
| Protection | 10 | 0.2 | 500 |
| Teleportation | 5 | 0.2 | 1000 |
| Reckless Attacks | 0 | 0.2 | 200 |
| Open Wounds | 5 | 0.2 | 700 |
| Escaping | 30 | 0.2 | 4000 |
| the Mouse | 30 | 0.2 | 100 |
| the Dog | 0 | 0.2 | 100 |
| Slow Digestion | 5 | 0.2 | 250 |
| Feather Falling | 5 | 0.2 | 200 |
| Free Action | 20 | 0.2 | 1500 |
| See Invisible | 30 | 0.2 | 340 |
| Digging | 0 | 0.2 | 800 |

#### Amulet  (`"`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Wisdom | 30 | 0.3 | 500 |
| Resist Lightning | 10 | 0.3 | 300 |
| Resist Acid | 10 | 0.3 | 300 |
| Resistance | 60 | 0.3 | 20000 |
| Sustenance | 60 | 0.3 | 20000 |
| the Magi | 70 | 0.3 | 30000 |
| ESP | 50 | 0.3 | 25000 |
| Devotion | 70 | 0.3 | 30000 |
| Weaponmastery | 70 | 0.3 | 30000 |
| Trickery | 70 | 0.3 | 30000 |
| Regeneration | 30 | 0.3 | 600 |
| Infravision | 10 | 0.3 | 200 |
| Searching | 15 | 0.3 | 600 |
| Teleportation | 10 | 0.3 | 10 |
| Slow Digestion | 15 | 0.3 | 200 |
| Adornment | 10 | 0.3 | 0 |
| Inertia | 15 | 0.3 | 10 |

#### Scroll  (`?`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Phase Door | 1 | 0.5 | 15 |
| Teleportation | 10 | 0.5 | 40 |
| Teleport Level | 20 | 0.5 | 800 |
| Magic Mapping | 5 | 0.5 | 65 |
| Treasure Detection | 0 | 0.5 | 70 |
| Detect Invisible | 1 | 0.5 | 15 |
| Identify Rune | 1 | 0.5 | 200 |
| Enchant Weapon To-Hit | 15 | 0.5 | 125 |
| Enchant Weapon To-Dam | 15 | 0.5 | 125 |
| Enchant Armour | 15 | 0.5 | 125 |
| *Enchant Weapon* | 50 | 0.5 | 500 |
| *Enchant Armour* | 50 | 0.5 | 500 |
| Remove Curse | 10 | 0.5 | 100 |
| *Remove Curse* | 50 | 0.5 | 8000 |
| Summon Monster | 1 | 0.5 | 0 |
| Summon Undead | 15 | 0.5 | 0 |
| Acquirement | 20 | 0.5 | 5000 |
| *Acquirement* | 60 | 0.5 | 10000 |
| Dispel Undead | 40 | 0.5 | 200 |
| Banishment | 40 | 0.5 | 2500 |
| Mass Banishment | 50 | 0.5 | 3000 |
| Remove Hunger | 5 | 0.5 | 10 |
| Light | 0 | 0.5 | 15 |
| Word of Recall | 5 | 0.5 | 125 |
| Recharging | 40 | 0.5 | 200 |
| Door Destruction | 10 | 0.5 | 50 |
| Deep Descent | 10 | 0.5 | 250 |
| Blessing | 1 | 0.5 | 15 |
| Holy Chant | 10 | 0.5 | 40 |
| Holy Prayer | 25 | 0.5 | 80 |
| Protection from Evil | 30 | 0.5 | 50 |
| Monster Confusion | 5 | 0.5 | 30 |
| Rune of Protection | 60 | 0.5 | 500 |
| *Destruction* | 40 | 0.5 | 1000 |
| Aggravate Monster | 5 | 0.5 | 0 |
| Curse Weapon | 50 | 0.5 | 0 |
| Curse Armour | 50 | 0.5 | 0 |
| Darkness | 1 | 0.5 | 0 |
| Trap Creation | 10 | 0.5 | 0 |

#### Potion  (`!`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Strength | 30 | 0.4 | 8000 |
| Intelligence | 30 | 0.4 | 8000 |
| Wisdom | 30 | 0.4 | 8000 |
| Dexterity | 30 | 0.4 | 8000 |
| Constitution | 30 | 0.4 | 8000 |
| Augmentation | 40 | 0.4 | 50000 |
| Experience | 65 | 0.4 | 25000 |
| Cure Light Wounds | 0 | 0.4 | 20 |
| Cure Serious Wounds | 3 | 0.4 | 70 |
| Cure Critical Wounds | 5 | 0.4 | 200 |
| Healing | 15 | 0.4 | 800 |
| *Healing* | 40 | 0.4 | 1500 |
| Life | 60 | 0.4 | 5000 |
| Neutralize Poison | 5 | 0.4 | 75 |
| Restore Mana | 25 | 0.4 | 350 |
| Restore Life Levels | 40 | 0.4 | 400 |
| Brawn | 3 | 0.4 | 1000 |
| Intellect | 20 | 0.4 | 1000 |
| Contemplation | 20 | 0.4 | 1000 |
| Nimbleness | 5 | 0.4 | 1000 |
| Toughness | 10 | 0.4 | 1000 |
| Enlightenment | 25 | 0.4 | 800 |
| *Enlightenment* | 70 | 0.4 | 12000 |
| Speed | 1 | 0.4 | 75 |
| Heroism | 1 | 0.4 | 25 |
| Berserk Strength | 3 | 0.4 | 80 |
| Boldness | 1 | 0.4 | 8 |
| Resist Heat | 30 | 0.4 | 30 |
| Resist Cold | 30 | 0.4 | 30 |
| Resist Poison | 20 | 0.4 | 30 |
| True Seeing | 3 | 0.4 | 50 |
| Infravision | 3 | 0.4 | 20 |
| Slime Mold Juice | 0 | 0.4 | 2 |
| Sleep | 0 | 0.4 | 0 |
| Blindness | 0 | 0.4 | 0 |
| Confusion | 0 | 0.4 | 0 |
| Poison | 3 | 0.4 | 0 |
| Slowness | 1 | 0.4 | 0 |
| Surprise | 10 | 0.4 | 0 |
| Salt Water | 0 | 0.4 | 0 |
| Dragon Breath | 0 | 0.4 | 100 |

#### Wand  (`-`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Magic Missile | 3 | 1 | 200 |
| Lightning Bolts | 15 | 1 | 600 |
| Frost Bolts | 20 | 1 | 800 |
| Fire Bolts | 30 | 1 | 1000 |
| Acid Bolts | 30 | 1 | 950 |
| Stinking Cloud | 5 | 1 | 400 |
| Lightning Balls | 35 | 1 | 1200 |
| Cold Balls | 40 | 1 | 1500 |
| Fire Balls | 55 | 1 | 1800 |
| Acid Balls | 50 | 1 | 1650 |
| Slow Monster | 15 | 1 | 500 |
| Confuse Monster | 5 | 1 | 500 |
| Hold Monster | 5 | 1 | 500 |
| Stun Monster | 5 | 1 | 500 |
| Scare Monster | 10 | 1 | 500 |
| Light | 3 | 1 | 200 |
| Stone to Mud | 10 | 1 | 300 |
| Polymorph | 20 | 1 | 400 |
| Darkness | 3 | 1 | 100 |
| Clone Monster | 15 | 1 | 0 |
| Teleport Other | 20 | 1 | 3500 |
| Disable Traps | 20 | 1 | 700 |
| Wonder | 3 | 1 | 250 |
| Dragon's Flame | 55 | 1 | 2400 |
| Dragon's Frost | 50 | 1 | 2400 |
| Dragon's Breath | 60 | 1 | 2400 |
| Drain Life | 50 | 1 | 1200 |
| Annihilation | 60 | 1 | 3000 |
| Heal Monster | 3 | 1 | 0 |
| Haste Monster | 3 | 1 | 0 |

#### Staff  (`_`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Summoning | 10 | 5 | 0 |
| Haste Monsters | 10 | 5 | 0 |
| Darkness | 5 | 5 | 0 |
| Earthquakes | 40 | 5 | 350 |
| *Destruction* | 50 | 5 | 6000 |
| Confuse Monsters | 10 | 5 | 800 |
| Slow Monsters | 20 | 5 | 800 |
| Sleep Monsters | 10 | 5 | 700 |
| Detect Invisible | 5 | 5 | 200 |
| Detect Evil | 20 | 5 | 350 |
| Mapping | 20 | 5 | 600 |
| Curing | 25 | 5 | 500 |
| Cure Light Wounds | 5 | 5 | 350 |
| Healing | 70 | 5 | 5000 |
| Dispel Evil | 50 | 5 | 1200 |
| Banishment | 70 | 5 | 5000 |
| Power | 70 | 5 | 4000 |
| Holiness | 70 | 5 | 3500 |
| Light | 5 | 5 | 250 |
| Starlight | 20 | 5 | 800 |
| Teleportation | 20 | 5 | 2000 |
| Speed | 40 | 5 | 2000 |
| Remove Curse | 40 | 5 | 1500 |
| the Magi | 70 | 5 | 4500 |

#### Rod  (`-`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Treasure Location | 10 | 1.5 | 1000 |
| Detection | 30 | 1.5 | 3000 |
| Magic Mapping | 35 | 1.5 | 2000 |
| Probing | 30 | 1.5 | 1000 |
| Fire Bolts | 35 | 1.5 | 3000 |
| Frost Bolts | 25 | 1.5 | 2500 |
| Lightning Bolts | 20 | 1.5 | 2000 |
| Acid Bolts | 40 | 1.5 | 3500 |
| Fire Balls | 75 | 1.5 | 5000 |
| Cold Balls | 60 | 1.5 | 4500 |
| Lightning Balls | 55 | 1.5 | 4000 |
| Acid Balls | 70 | 1.5 | 5500 |
| Slow Monster | 40 | 1.5 | 1500 |
| Hold Monster | 30 | 1.5 | 1500 |
| Curing | 40 | 1.5 | 1000 |
| Healing | 80 | 1.5 | 20000 |
| Restoration | 80 | 1.5 | 25000 |
| Polymorph | 35 | 1.5 | 1200 |
| Drain Life | 75 | 1.5 | 3600 |
| Teleport Other | 35 | 1.5 | 5000 |
| Disable Traps | 35 | 1.5 | 2100 |
| Light | 10 | 1.5 | 500 |
| Illumination | 20 | 1.5 | 1000 |
| Recall | 30 | 1.5 | 4000 |
| Speed | 55 | 1.5 | 25000 |

#### Chest  (`~`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| Small wooden chest | 5 | 25 | 20 |
| Large wooden chest | 15 | 50 | 60 |
| Small iron chest | 25 | 30 | 100 |
| Large iron chest | 35 | 100 | 150 |
| Small steel chest | 45 | 50 | 200 |
| Large steel chest | 55 | 100 | 250 |

#### Dragon Armor  (`[`)

| Name | Native level | Weight (lb) | Base cost | Base AC |
|---|---|---|---|---|
| Black Dragon Scale Mail | 27 | 12 | 40000 | 16 |
| Blue Dragon Scale Mail | 30 | 10 | 40000 | 12 |
| White Dragon Scale Mail | 25 | 11 | 40000 | 14 |
| Red Dragon Scale Mail | 32 | 16 | 40000 | 24 |
| Green Dragon Scale Mail | 30 | 14 | 60000 | 20 |
| Multi-Hued Dragon Scale Mail | 50 | 26 | 150000 | 45 |
| Shining Dragon Scale Mail | 37 | 20 | 60000 | 32 |
| Law Dragon Scale Mail | 45 | 22 | 80000 | 36 |
| Gold Dragon Scale Mail | 30 | 18 | 40000 | 28 |
| Chaos Dragon Scale Mail | 45 | 24 | 80000 | 40 |
| Balance Dragon Scale Mail | 52 | 28 | 100000 | 50 |
| Power Dragon Scale Mail | 57 | 30 | 300000 | 60 |

#### Gold  (`$`)

| Name | Native level | Weight (lb) | Base cost |
|---|---|---|---|
| copper |  | 0 |  |
| silver |  | 0 |  |
| garnets |  | 0 |  |
| gold |  | 0 |  |
| opals |  | 0 |  |
| sapphires |  | 0 |  |
| rubies |  | 0 |  |
| diamonds |  | 0 |  |
| emeralds |  | 0 |  |
| mithril |  | 0 |  |
| adamantite |  | 0 |  |

## Appendix C — Ego item names

Ego types grouped by the kinds of item they can appear on:

- **arrow, bolt, shot:** of Slay Animal, of Slay Evil, of Slay Undead, of Slay Demon, of Acid, of Lightning, of Slay Giant, of Slay Dragon, of Holy Might, of Venom, of Flame, of Frost, of Wounding, of Backbiting
- **boots:** of Slow Descent, of Stealth, of Free Action, of Speed, of Stability, of Elvenkind
- **bow:** of Accuracy, of Power, of Lothlórien, of the Haradrim, of Extra Might, of Extra Shots, of Buckland, of the Nazgûl
- **cloak:** of Protection, of Stealth, of Aman, of the Magi
- **crown:** of the Magi, of Might, of Lordliness, of Light, of Serenity, of Night and Day
- **digger:** of Digging, of Earthquakes
- **dragon armor:** of Craftsmanship, of Stealth, of Resistance, of Elvenkind, (Dwarven), of Speed
- **gloves:** of Free Action, of Slaying, of Agility, of Power, of Thievery, of Combat
- **hafted, polearm, sword:** (Holy Avenger), (Defender), (Blessed), of Gondolin, of Westernesse, of Extra Attacks, of Acid, of Lightning, of Flame, of Frost, of Venom, of Slay Animal, of Slay Evil, of Slay Undead, of Slay Demon, of Slay Orc, of Slay Troll, of Slay Giant, of Slay Dragon, of *Slay Animal*, of *Slay Evil*, of *Slay Undead*, of *Slay Demon*, of *Slay Orc*, of *Slay Troll*, of *Slay Giant*, of *Slay Dragon*, of Morgul
- **hard armor:** (Dwarven)
- **hard armor, soft armor:** of Resist Acid, of Resist Lightning, of Resist Fire, of Resist Cold, of Resistance, of Elvenkind
- **helm:** of Intelligence, of Wisdom, of Seeing, of Infravision, of Telepathy, of Regeneration
- **light:** of Shadows, of Brightness, (Everburning), of True Sight
- **shield:** (Dwarven), of Resist Acid, of Resist Lightning, of Resist Fire, of Resist Cold, of Resistance, of Elvenkind, of Preservation
- **soft armor:** of Permanence

## Appendix D — Object properties (runes)

All the properties objects can have (stats, modifiers, flags). Elemental
resistances, brands, slays and curses are further runes.

| Property | Kind | Description |
|---|---|---|
| strength | stat |  |
| intelligence | stat |  |
| wisdom | stat |  |
| dexterity | stat |  |
| constitution | stat |  |
| stealth | mod |  |
| searching skill | mod |  |
| infravision | mod |  |
| tunneling | mod |  |
| speed | mod |  |
| damage reduction | mod |  |
| extra blows | mod |  |
| shooting speed | mod |  |
| shooting power | mod |  |
| extra moves | mod |  |
| light resistance | resistance |  |
| sustain strength | flag |  |
| sustain intelligence | flag |  |
| sustain wisdom | flag |  |
| sustain dexterity | flag |  |
| sustain constitution | flag |  |
| protection from fear | flag | fear |
| protection from blindness | flag | blindness |
| protection from confusion | flag | confusion |
| protection from stunning | flag | stunning |
| slow digestion | flag | Slows your metabolism |
| feather falling | flag | Feather Falling |
| regeneration | flag | Speeds regeneration |
| telepathy | flag | Grants telepathy |
| see invisible | flag | Grants the ability to see invisible things |
| free action | flag | Prevents paralysis |
| hold life | flag | Sustains your life force |
| earthquakes | flag | Sometimes creates earthquakes on impact |
| blessed melee | flag | Blessed by the Valar (combat bonuses for holy casters) |
| burns out | flag |  |
| takes fuel | flag |  |
| no fuel | flag |  |
| impaired hitpoint recovery | flag | Impairs hitpoint recovery |
| impaired mana recovery | flag | Impairs mana recovery |
| constant fear | flag | Makes you afraid of melee, and worse at shooting and casting spells |
| teleportation ban | flag | Prevents teleportation |
| aggravation | flag | Aggravates creatures nearby |
| experience drain | flag | Drains experience |
| stuck on | flag | Can't be removed |
| fragile | flag | Can be destroyed if you attempt to remove its curses |
| intensity 2 light | flag |  |
| intensity 3 light | flag |  |
| power 1 digging | flag |  |
| power 2 digging | flag |  |
| power 3 digging | flag |  |
| explode | flag |  |
| trap immunity | flag | Gives immunity to traps |
| throwing | flag |  |
| multiply weight | flag |  |
| acid immunity | immunity |  |
| lightning immunity | immunity |  |
| fire immunity | immunity |  |
| cold immunity | immunity |  |
| poison resistance | resistance |  |
| dark resistance | resistance |  |
| sound resistance | resistance |  |
| shards resistance | resistance |  |
| nexus resistance | resistance |  |
| nether resistance | resistance |  |
| chaos resistance | resistance |  |
| disenchantment resistance | resistance |  |

## Appendix E — Player abilities

Special abilities that races, classes and shapes can grant:

| Ability | Description |
|---|---|
| Rock | You are made of rock. |
| Mushroom Lore | You easily recognize mushrooms. |
| Device Lore | You easily recognize magic devices. |
| Miner | You can sense ore in the walls. |
| Extra Shots | Your shooting speed improves every 3 levels. |
| Relentless [30] | You become immune to fear at level 30. |
| Blessed Fighter | You get improved melee with blessed or hafted weapons. |
| Full Spellcaster | You may obtain a perfect success rate with magic. |
| Extra Spell Beaming | Your bolt spells frequently turn into beams. |
| Spell Choice | You may choose your own spells to study. |
| No Magic | You cannot cast spells. |
| Charms | You are extra persuasive to monsters. |
| Unlight | You gain stealth in, can see in, and resist the dark. |
| Pickpocketing | You can steal from monsters. |
| Shield Bash | You can bash monsters with a shield in melee. |
| Evil | You resist nether, but are hurt by holy attacks. |
| Combat Regeneration | You draw power from the thrill of combat, represented by Spell Points (SP). You gain SP when damaged by an enemy or when you attack in melee. As your blood cools your power fades; you lose SP at half the rate other classes gain them.  When you spend SP by casting a spell you regain some health, and fading SP replenish your health more efficiently. The more damaged you are, the bigger these health gains will be. |
| Sustain Strength | Your strength is sustained. |
| Sustain Intelligence | Your intelligence is sustained. |
| Sustain Wisdom | Your wisdom is sustained. |
| Sustain Dexterity | Your dexterity is sustained. |
| Sustain Constitution | Your constitution is sustained. |
| Fear Immunity | You are immune to fear. |
| Blindness Immunity | You cannot be blinded. |
| Confusion Immunity | You cannot be confused. |
| Stun Immunity | You cannot be stunned. |
| Slow Digestion | You have a slow metabolism. |
| Feather Falling | You fall lightly. |
| Regeneration | You regenerate quickly. |
| Telepathy | You have telepathy. |
| See Invisible | You can see invisible creatures. |
| Free Action | You are immune to paralysis. |
| Hold Life | Your life force is sustained. |
| Earthquakes | You sometimes create earthquakes on impact. |
| Slow Regeneration | Your hitpoint recovery is impaired. |
| Slow Mana Regeneration | Your mana recovery is impaired. |
| Constant Fear | You are afraid of melee, and bad at shooting and casting spells. |
| Teleport Ban | You cannot teleport. |
| Aggravate | You aggravate creatures nearby. |
| Experience Drain | Your experience constantly drains away. |
| Trap Immune | You are immune to traps. |
| Vulnerability | You are vulnerable to |

## Appendix F — Terrain

| Terrain | Symbol | Description |
|---|---|---|
| open floor | `.` | An open floor or bare ground. Can be walked across by anything, and can hold traps or items. Does not break line of sight. |
| closed door | `+` | Doors may be locked; getting through them may not be easy. Your disarming skill makes all the difference to your ability to handle locks, but you can also take a pickaxe to them, or blast them open with magic. Monsters are also slowed down by doors; you can earn yourself valuable time by locking them. |
| open door | `'` | A door that is already open. Player, monster, spell and missile can pass through as long as it stays open. |
| broken door | `'` | A door so broken that it cannot be shut. |
| up staircase | `<` | A staircase up to a shallower part of the dungeon. |
| down staircase | `>` | A staircase down to a deeper part of the dungeon. |
| General Store | `1` | The General Store sells food, ammunition and lighting supplies. |
| Armoury | `2` | The armour sold here will give you some protection against the blows of your enemies. |
| Weapon Smiths | `3` | Weapons for hitting and shooting your enemies are forged in the hot, acrid backroom of this enticing shop. |
| Bookseller | `4` | A quiet, reflective place of refuge, lined with shelves of mystical tomes. |
| Alchemy Shop | `5` | A dim, scented room where potions and scrolls are traded. |
| Magic Shop | `6` | A shop for devices and adornments with magic trapped within. |
| Black Market | `7` | Watch your back and hold onto your purse as you enter this disreputable haunt - and do not expect friendly service or good bargains. |
| Home | `8` | Your safe piece of Middle-earth, and the only place you can store goods apart from on your person. |
| secret door | `#` | A door that appears like a granite wall to untrained eyes. |
| pile of rubble | `:` | Ends LOS, stops missiles, bolts, and beams. May dissolve or be tunnelled to normal floor. |
| magma vein | `%` | A seam of soft rock. It can be removed by digging or magic, and passed through by immaterial monsters. It stops any spells, missiles or line of sight. |
| quartz vein | `%` | A seam of hardish rock. It can be removed by digging or magic, and passed through by immaterial monsters. It stops any spells, missiles or line of sight. |
| magma vein with treasure | `*` | A seam of soft rock. It can be removed by digging or magic, and passed through by immaterial monsters. It stops any spells, missiles or line of sight. It contains visible treasure. |
| quartz vein with treasure | `*` | A seam of hardish rock. It can be removed by digging or magic, and passed through by immaterial monsters. It stops any spells, missiles or line of sight. It contains visible treasure. |
| granite wall | `#` | A seam of hard rock. It can be removed by digging or magic, and passed through by immaterial monsters. It stops any spells, missiles or line of sight. |
| permanent wall | `#` | You can dig through most walls but these are impenetrable. The dungeon is surrounded by these kinds of walls and some special rooms are made of them. |
| lava | `#` | A fiery pool of glowing lava. Step in it at your peril! |
| pile of passable rubble | `:` | Ends LOS, stops missiles, bolts, and beams, and reduces the radius of ball spells. May dissolve or be tunnelled to normal floor, and can be walked through by the player and monsters. |

## Appendix G — Game constants

From `lib/gamedata/constants.txt` (values for this build):

| Constant | Value |
|---|---|
| Pack size | 23 slots |
| Quiver | 10 slots; 40 missiles per pack slot; throwing weapons count 5× |
| Max stack size | 40 |
| Floor pile limit | 23 objects |
| Store inventory max | 24 |
| Store "day" | 10,000 game turns in the dungeon |
| Store owner shuffle | 1 in 25 per store day |
| Day length | 10,000 game turns (half day, half night) |
| Dungeon size | 198 × 66 grids; town 66 × 22 |
| Maximum depth | 127 (levels 0–127) |
| Max sight / projection range | 20 grids |
| Starting gold (base) | 600 |
| Torch fuel | 5,000 turns; lantern 15,000 max (7,500 when found) |
| Monsters per level (max) | 1,024 |
| Out-of-depth monster chance | 1 in 25, up to 10 levels deeper |
| Max monster group | 25 |
| Glyph of warding break chance | 550 (vs monster level) |
| Normal move energy | 100 |
| Level feeling needs | 10 of 100 "feeling squares" explored |

## Appendix H — Files and directories

| Path | Contents |
|---|---|
| `lib/gamedata/` | Game data files (races, classes, monsters, objects, artifacts, vaults, stores, constants…). Copies placed in your user directory override them. |
| `lib/customize/` | Default pref files (`pref.prf`, `font*.prf`, `message.prf`, `sound.prf`, `user.prf`). |
| `lib/help/` | In-game help text. |
| `lib/screens/` | Title and death screens. |
| `lib/tiles/`, `lib/fonts/`, `lib/sounds/`, `lib/icons/` | Graphics, fonts, sounds, icons. |
| `lib/user/` (Windows) / `~/.angband/Angband/` (Linux) / `~/Documents/Angband/` (macOS) | Your pref files, `lore.txt` (monster memory), `borg.txt`, character dumps. |
| `…/save/` | Savefiles. |
| `…/scores/` | High-score file. |
| `…/panic/` | Emergency saves after a crash. |
| `…/archive/` | Borg logs, `.map` snapshots and death saves. |
| `src/borg/borg.txt` | Sample Borg configuration. |
| `docs/` | The reStructuredText manual this document is based on (build with Sphinx). |
| `changes.txt` | Change log since 3.5.1. |

---

*This manual was compiled from the documentation in `docs/`, the in-game help,
the source in `src/` and the data in `lib/gamedata/` of this repository.
Angband is copyright the Angband developers and contributors, released under
the Angband licence and GPLv2 — see `docs/copying.rst`.*
