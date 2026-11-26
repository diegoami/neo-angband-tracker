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
| `lightmap644-lure.sav` | Played by a **level 1 Human Druid with a whip** (12 HP, 1d3 damage — too weak to out-damage a breeder) and **no escape kit at all**: no spellbook, no recall, no phase door — whatever happens, happens on foot. The corridor runs east to an **up staircase** — the way home — past a wall of **three white worm masses** (poison touch, breeding) standing side by side in the dark; west of her a room holds an **awake kobold**, a scary guy for her level, and past a doorway a second room holds a second kobold, **asleep**. |
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

`lightmap644-lure.sav` (the level, west to east; `k` awake west,
`s` asleep far west, `w` the worm wall, `<` the way home):

```
#...##......w.##########################
#.k....k..@.w.<#########################
#...##......w.##########################
```

A **level 1 Human Druid with a whip** (12 HP, 1d3 damage — too weak
to out-damage a breeder) who came down the **up staircase** east of
the corridor and walked west into the room; the level memory covers
the corridor, the room and the staircase. In the dark of the first
west room an **awake kobold** lurks — in the borg's own scary-guy
list for characters of level 8 and under — past the doorway a second
room continues the level with a second kobold **asleep** at its far
end (asleep, so it is exempt from the missing-monster pass — "Don't
'forget' certain ones"), and **three white worm masses** (poison
touch, `MULTIPLY` — they breed) spawned abreast on the corridor's
far corner, two grids east, in the dark outside the torch's real
light. She carries **no escape kit**: no book, no recall, no phase
door. The saveplay does both glimpses — step west and see the
kobold, retreat; step east and see the worms, retreat — then runs
the production per-turn brain (`borg_think()`) and reads the borg's
own notes:

| Build | The records after the passes | The flight from the kobold |
| --- | --- | --- |
| base `574b618c3` | kobold kept, **all three worms forgotten**: `# Forgetting a monster 'white worm mass' at (12,12)` (x3) — their grids still marked `light=1` | `# Scary guy on level.` → `# Fleeing and leaving the level. (scaryguy)` → `# Flowing toward up-stairs at cost 4` → `# Desperate for Stairs (three)` — its first key is `'6'`, **a step east**: the flight to the stairs, straight into the forgotten wall. |
| fix `d8d31359e` | kobold kept, **all three worms kept** — the corner priced at ~111 against 12 HP | `# Scary guy on level.` → the east route refused — its first key is `'2'`, **a step away** into the corridor's wide row. On foot, with no kit, what it can do is keep its distance. |

Both builds are running from the same kobold; the forget does not
touch what the borg wants, it deletes the *fear on the way*. The
original's flight route east looks clear — a wall of breeding poison
stands on it that the borg just deleted from its own memory — so its
first movement key is a step east, and a 12 HP druid with a 1d3 whip
walks into `MULTIPLY`. A level 20 Warrior prices the same worm at 13
and shrugs; the level 1 Druid prices three of them at 111 against 12
hit points, and the forget zeroes exactly that.

### The forget driving a death, live in the real game

The lure was played live in the built game (Xvfb-driven, 150 borg
steps each; the borg's own map-and-notes dump at death is committed
as `live-lure-{base,fix}-150.map`). The **base** build's log, verbatim
highlights:

```
# Flowing toward up-stairs at cost 4
# Creating a monster 'white worm mass' at (13,12)   <- the reveal: three at once
# Creating a monster 'white worm mass' at (12,12)
# Creating a monster 'white worm mass' at (11,12)
# Loc:12,11 ... HP:12/12 ... Danger:p=111
You hit the white worm mass.
The white worm mass crawls on you.  You are poisoned!   (x3)
# Creating a monster 'kobold' at (12,10)                <- the chaser arrives
# There was a monster 'white worm mass' at (12,12)
# Forgetting a monster 'white worm mass' at (12,12)     <- the forget, live
# Loc:12,11 ... HP:8/12 ... Danger:p=80                 <- the fear collapses
# Flowing toward up-stairs at cost 3                    <- east, again
# There was a monster 'kobold' at (12,10)
# Forgetting a monster 'kobold' at (12,10)
# Flowing toward up-stairs at cost 2 ... cost 1
You enter a maze of up staircases.
You die.   (Killed on level 4 by poison, turn 100)
```

The borg meets the wall, fights, and every time a worm goes dark
behind it the light map bug deletes the record: `# There was a
monster ...` → `# Forgetting ...`, the priced danger collapses
(111 → 80), and the flight to the stairs resumes straight through
the mass. Three poisonings later it climbs the stairs and dies of
the accumulated poison one level up. That is the issue's consequence
played out: the amnesia does not change the goal, it deletes the
fear on the way.

The **fix** build's run in the same layout is filed beside it, and
honestly: it also fights through and dies of the same poison. Two
measured reasons. First, this borg *attacks* an adjacent killable
monster rather than retreating — even at danger 111 against an
attack value of 31 — and the scenario boxes the druid in by design
(the kobold west closes within a handful of turns, the worm wall
east), so cornered fighting is the least bad option on either
build. Second, the worms are random movers, so the *legitimate*
forget (a monster that genuinely moved away) also fires on the fix —
its one `# Forgetting` line is that case, a worm that really had
moved. The clean build-vs-build divergence — the base deleting a
record of a monster that is still standing there — is what the
frozen matrix above pins down deterministically, which is why it,
not free play, is the evidence of record.

## Replay it

```sh
python3 demo/run-save-demo.py            # builds base and fix, writes both saves, replays
python3 demo/run-save-demo.py --skip-build   # reuse existing out/source-*/build-* trees
```

The runner asserts the kept/deleted matrix (retreat/worm/lure: base 0,
fix 1; stay: 1 and 1, logs byte-identical), and for the lure that the
base borg keeps only the kobold (1 record, all three worms forgotten)
while the fix keeps all four records, that the base's first movement
key is the step east (`'6'`, 54) while the fixed borg's is the step
away (`'2'`, 50), and writes `demo-result.json` with the
sha256 of every log and save. Heavy steps hold
`/tmp/angband-validation.lock`. This run:

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix
  `d8d31359ec2ef9c73c9d9aa4b92579dc73a3b5fb` (branch `task/lightmap-644`,
  PR #660).
- `lightmap644-retreat.sav` sha256 `c1a6857a0d1663be`...,
  `lightmap644-worm.sav` sha256 `50cf6fba50bcef86`...,
  `lightmap644-lure.sav` sha256 `e9ec1ebc376c2657`...,
  `lightmap644-stay.sav` sha256 `b3682099aa84b9d0`...
  (full digests in `demo-result.json`).

`saveplay.c` also has a dump mode for reading back a save the real
game wrote: `LM644S_DUMP=1 LM644S_SAVE=<file> ./LM644SP -v` (run from
the game dir) prints the player, every monster, and the whole message
log the savefile carries. The `live-lure-*.map` files here are the
borg's own dumps from the live runs described below.

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
   so the borg starts blind and the worm wall is news to either): the
   notes show `# Needing to get back on correct depth`, `# Flowing
   toward up-stairs at cost 4`, and the druid steps east for the
   stairs.
4. One step in, the torch reveals all three worms at once:
   `# Creating a monster 'white worm mass'` x3 → `Danger:p=111` →
   `# Scary guy on level.` With no escape kit, the borg fights — and
   this is where the builds' logs start to differ: on **base**, every
   time a worm goes dark two grids away, `# There was a monster
   'white worm mass' at ...` → `# Forgetting a monster ...` fires and
   the priced danger collapses (111 → 80), and the flight to the
   stairs resumes straight through the wall — three poisonings later
   it climbs the stairs and dies of the accumulated poison (verified:
   `Killed on level 4 by poison`, turn 100, `live-lure-base-150.map`).
   On **fix** the records of worms that are still standing there hold,
   and only the genuine movers are forgotten.
5. The clean single-turn divergence — the base deleting a record of a
   monster that is still there, the fix keeping it — is what the
   saveplay matrix above pins down deterministically; single-stepping
   (`x`, `1`) around the corner after a reveal shows the same lines
   live.

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
