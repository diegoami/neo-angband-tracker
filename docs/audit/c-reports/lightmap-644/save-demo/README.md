# lightmap-644 synthetic savefile demonstration

Four real savefiles for [issue #644](https://github.com/diegoami/neo-angband-tracker/issues/644)
(the borg's light map takes `BI_LIGHT` as its radius, one grid more than
the game lights): a dark one-grid-wide corridor (no glow anywhere, so
the torch is the only light) that the character walked before the save,
with one awake monster standing in the dark just beyond the torch's
real light — and, for the lure, a west room with a kobold in it and an
up staircase east. The saves are
written by `savegen.c` through the production savefile writer, loaded by
`saveplay.c` through the production loader (`savefile_load` +
`on_new_level`), and played through the borg's real per-turn observation
path (`borg_update()`: the map scan through `map_info()`, the view, the
light map, the missing-monster pass).

## The saves

| Save | Situation |
| --- | --- |
| `lightmap644-retreat.sav` | A soldier ant holds the corridor's last floor, two grids east of the player, in the dark outside the torch's real light (the corridor was walked, so the level's memory covers that floor). |
| `lightmap644-worm.sav` | The same dead end with a **white worm mass** (poison touch, slow, a random-mover): the monster that lingers in the dark ring — the live in-game forget. |
| `lightmap644-lure.sav` | Played by a **level 1 Human Druid with a whip** (12 HP, 1d3 damage — too weak to out-damage a breeder): the corridor runs east to an **up staircase** — the way home — and west of her a small room holds a **kobold**, a scary guy for her level, so she is heading back east for the stairs. A **white worm mass** (poison touch, breeding) has spawned on the corridor's middle grid, two grids east, in the dark. She carries **two Scrolls of Phase Door** (no Word of Recall), and two **sealed pockets** — floor caverns with no way in on foot — sit inside the scroll's reach on her map. |
| `lightmap644-stay.sav` | Control: an ant stands one grid east, inside the real light, visible. |

Load them in the game itself, or replay them with `saveplay.c`:

```sh
LM644S_PROFILE=<user dir with borg.txt> LM644S_MODE=retreat \
    LM644S_SAVE=lightmap644-retreat.sav ./LM644SP -v
```

## What the retreat shows

`saveplay.c` steps the borg one grid east (the torch lights the ant's
grid, the game sees it, the scan notes its record), then steps back
west; one turn passes. The next `borg_update()` runs the missing-monster
pass on the record:

| Build | The ant's grid after the pass | The record | Danger the borg expects there |
| --- | --- | --- | --- |
| base `574b618c3` | `light=1` — the borg's light map still marks the dark grid lit from two grids away | deleted as one that "did not really move": `KILLS kills=0`, the map keeps only `@` and `a` | `DANGER ant_grid=0` — the grid looks known, torch-lit and safe |
| fix `d8d31359e` (`task/lightmap-644`) | `light=0` — the grid is dark | kept at (12,12): `?` on the map, right at the ant | `DANGER ant_grid=2` — the kept record prices the ant in |

So on the original the borg's model of the grid one step east is "known
floor, torch-lit, zero danger" while an awake soldier ant stands on it:
any flow that heads east (exploring the corridor's continuation, an item
or stairs beyond) walks straight into the ant it just forgot, without
the caution a dark grid or a remembered monster would trigger.

The control (`lightmap644-stay.sav`) keeps the record in both builds,
prices the ant identically (`DANGER ant_grid=2`), and is byte-identical.

## What the lure shows (the acted-out consequence)

`lightmap644-lure.sav`: a **level 1 Human Druid with a whip** (12 HP,
1d3 damage — too weak to out-damage a breeder) who came down the **up
staircase** east of the corridor and walked west into the room; the
level memory covers the corridor, the room and the staircase. A
**kobold** lurks in the dark of the west room — awake, mobile, and in
the borg's own scary-guy list for characters of level 8 and under —
and a **white worm mass** (poison touch, `MULTIPLY` — it breeds)
spawned on the corridor's middle grid, two grids east, in the dark
outside the torch's real light. Her escape kit is **two Scrolls of
Phase Door** (the level has no Word of Recall for her), and two
**sealed pockets** — 4x4 floor caverns with no walking route in or
out, on her map inside the scroll's 5..10 grid landing ring. The
saveplay does both glimpses — step west and see the kobold, retreat;
step east and see the worms, retreat — then runs the production
per-turn brain (`borg_think()`) and reads the borg's own notes:

| Build | The records after the passes | The flight from the kobold |
| --- | --- | --- |
| base `574b618c3` | kobold kept (3 grids west, dark and not viewable), worm **forgotten**: `# Forgetting a monster 'white worm mass' at (12,12)` — its grid still marked `light=1` | `# Scary guy on level.` → `# Fleeing and leaving the level. (scaryguy)` → `# Flowing toward up-stairs at cost 4` → `# Desperate for Stairs (three)` — its first key is `'6'`, **a step east**: the flight to the stairs, straight through the grid the forgotten worm stands on. |
| fix `d8d31359e` | kobold kept, worm kept — the record sits on the worm's grid, priced at **35** against 12 HP | `# Scary guy on level.` → `# Fleeing and leaving the level. (scaryguy)` → the stairs east are refused (the flow through the corner is priced over the fear line) — its first key is `'1'`, **a step away** into the room's south edge. When pressed, its escape is the Phase Door: the sealed pockets make `borg_caution_phase()` approve the jump. |

Both builds are running from the same kobold; the forget does not
touch what the borg wants, it deletes the *fear on the way*. The
original's flight route east looks clear — the breeding poison mass
that stands on it is a monster it just forgot — so its first movement
key is a step east, and a 12 HP druid with a 1d3 whip walks into
`MULTIPLY`. The corrected borg remembers the worm, cannot afford the
path, and turns away — and with the Phase Door kit it has the escape
the level was built for. A level 20 Warrior prices the same worm at
13 and shrugs; the level 1 Druid prices it at 35 against 12 hit
points, and the forget zeroes exactly that.

### The escape, verified in the real game

The lure was also played live in the built game (Xvfb-driven, both
builds, 40 borg steps each, then save-and-quit; `saveplay.c`'s
`LM644S_DUMP` mode reads the message log back out of the savefile —
see `live-lure-*.log` here). The message log, verbatim:

```
# Stepping Borg 40 times
# Needing to get back on correct depth
# Flowing toward up-stairs at cost 4
# Looking for stairs. Goal_less, Fleeing.
# Creating a monster 'white worm mass' at (12,12), HP: 11, Index: 26
# Scary guy on level.
# Fleeing and leaving the level. (scaryguy)
# Loc:12,11 Dep:5 Lev:1 HP:12/12 SP:2/2 Danger:p=32
# Safe to Phase. scary squares: 0
# Reading Scrolls of Phase Door.
# Danger Level 1.
You have a Scroll of Phase Door (b).
```

The druid steps east toward the stairs, the torch reveals the worm at
the corner (danger 32), and the borg reads the Phase Door — `# Safe to
Phase. scary squares: 0`: the sealed pockets on its map are landing
zones it can price as safe, so the scroll is approved. It lands inside
one (`player=(4,6)`, 12/12 HP, turn 71) with the worm and the chasing
kobold both stranded outside. An extended 120-step run shows the rest:
bored in the sealed pocket (`# Bored but unable to flow to up stairs`),
it burns the second scroll (`# Considering Phase (twitchy)` →
`# Reading Scroll of Phase Door.`) and phases back out.

Both builds play this opening identically — the savefile carries no
borg state, so the borg of either build starts blind and learns the
worm at the same step. The forget itself is what the frozen matrix
above pins down: the divergence needs the seen-then-dark window, which
the saveplay's glimpses provide deterministically.

## Replay it

```sh
python3 demo/run-save-demo.py            # builds base and fix, writes both saves, replays
python3 demo/run-save-demo.py --skip-build   # reuse existing out/source-*/build-* trees
```

The runner asserts the kept/deleted matrix (retreat/worm/lure: base 0,
fix 1; stay: 1 and 1, logs byte-identical), and for the lure that the
base borg keeps only the kobold (1 record, the worm forgotten) while
the fix keeps both (2 records), that the base's first movement key is
the step east (`'6'`, 54) while the fixed borg's is the step away
(`'1'`, 49, into the room's south edge), and writes
`demo-result.json` with the
sha256 of every log and save. Heavy steps hold
`/tmp/angband-validation.lock`. This run:

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix
  `d8d31359ec2ef9c73c9d9aa4b92579dc73a3b5fb` (branch `task/lightmap-644`,
  PR #660).
- `lightmap644-retreat.sav` sha256 `c1a6857a0d1663be`...,
  `lightmap644-worm.sav` sha256 `50cf6fba50bcef86`...,
  `lightmap644-lure.sav` sha256 `dc9c58e2a42373c4`...,
  `lightmap644-stay.sav` sha256 `b3682099aa84b9d0`...
  (full digests in `demo-result.json`).

`saveplay.c` also has a dump mode for reading back a save the real
game wrote: `LM644S_DUMP=1 LM644S_SAVE=<file> ./LM644SP -v` (run from
the game dir) prints the player, every monster, and the whole message
log the savefile carries. The `live-lure-*.log` files here were
produced that way from the live runs described below.

## Load a save in the real game (manual play test)

The games are built at `out/build-{base,fix}/game/angband` (X11 front
end, auto-enabled on non-Windows; `lib/` is staged next to the binary).
These builds keep their savefiles in **`<game dir>/lib/save/`** (the
four saves are staged there as `lm644-{retreat,worm,lure,stay}`) and
read the borg config from `out/test-user/Angband/` (staged with the
same `borg.txt` as the fixture). Always run from the `game/` directory:

```sh
cd /tmp/opencode/lightmap644-save/out/build-base/game
ANGBAND_X11_FONT=fixed ./angband -ulm644-lure      # note: -u<name>, no space
# and the fixed build:
cd /tmp/opencode/lightmap644-save/out/build-fix/game
ANGBAND_X11_FONT=fixed ./angband -ulm644-lure
```

`ANGBAND_X11_FONT=fixed` is needed on WSLg/XWayland (the default
`10x20` bitmap font is missing there; installing `xfonts-*` or building
with `-DSUPPORT_GCU_FRONTEND=ON` plus `libncurses-dev` are alternatives).
In the game: press a key at the startup banner, then **Ctrl+Z** (confirm
the borg warning once), **`z`** to activate the borg — **`x`** steps one
borg action at a time.

Who the defect actually bites (worth knowing when playing these):
`borg_follow_kill()` explicitly spares sleepers and never-moving
monsters — "Don't 'forget' certain ones" — so the forget only ever
deletes records of **awake, mobile** monsters: the hunters. But a fast
pursuer steps back into your light the same turn and is re-observed, so
the live forget needs a monster that *lingers* one turn in the dark ring
— an ambusher pausing, a wanderer, a slow pursuer that falls two grids
behind. (The port's 44-of-50 forgotten worms were exactly that.)

One mechanic matters when playing these live: **the borg only observes
on turns it plays itself** — `borg_think()` calls `borg_update()`, and
the `Ctrl+Z` key hook drives `borg_think()`. Walking around yourself
never feeds the borg's memory: a monster you saw and stepped away from
is, to a freshly activated borg, a monster it has never seen. The
step-east/see-it/step-back dance of `saveplay.c` therefore has to be
done *by the borg* in the live game, which it does on its own whenever
its light reveals the monster and it then retreats — or you can walk
east yourself to put the monster in your torch's light and press
`Ctrl+Z` `x` `1` (one borg step: its first update records the visible
monster, and its reaction usually steps it back west out of the light).

What to watch in game — the sequence, verified live on both builds:

1. `ANGBAND_X11_FONT=fixed ./angband -ulm644-lure`, press a key at the
   banner.
2. **Yes, start the borg immediately**: `Ctrl+Z` → **space** (the borg
   warning's `-more-` pause — without it the confirm eats your next
   key) → `y` (confirm) → `z` to run it continuously, or `x` then a
   number + Return to step that many actions at a time.
3. Both builds open identically (the savefile carries no borg state,
   so the borg starts blind and the worm is news to either): the notes
   show `# Needing to get back on correct depth`, `# Flowing toward
   up-stairs at cost 4`, and the druid steps east for the stairs.
4. One step in, the torch reveals the worm: `# Creating a monster
   'white worm mass' at (12,12)` → `Danger:p=32` → `# Scary guy on
   level.` → `# Fleeing and leaving the level. (scaryguy)`.
5. The escape your kit was built for: `# Safe to Phase. scary
   squares: 0` → `# Reading Scrolls of Phase Door.` — the sealed
   pockets on the map are priced as safe landings, so the scroll is
   approved — and the druid phases into one, with the worm and the
   chasing kobold stranded outside (verified landing: `player=(4,6)`,
   12/12 HP). Later, bored in the sealed pocket, it burns the second
   scroll to phase back out.
6. The **forget** lines are the base build's signature, and they
   appear whenever the borg has seen the worm and then stands two
   grids from it in the dark — `# There was a monster 'White worm
   mass' at (12,12)` followed by `# Forgetting a monster 'White worm
   mass' at (12,12)`, and the fear evaporates while the fixed build's
   record holds. The saveplay matrix above pins that moment
   deterministically; single-stepping (`x`, `1`) around the corner
   after a reveal shows it live.

The other saves: `lightmap644-worm.sav` / `lightmap644-retreat.sav`
step the borg once while the monster is in your torch's light (its
first update records it), let it step out of the light, and watch the
next update's log — base forgets, fix keeps. `lightmap644-stay.sav`
is the control: the ant inside the real light is re-observed every
pass and both builds keep it.

For the deterministic before/after comparison, always use `saveplay.c`
(the runner above): it freezes the world right after the step-back
(the issue's "monster waiting just outside its light") and shows the
one turn where the two builds diverge — the base's step east versus
the fix's step away.
