# lightmap-644 synthetic savefile demonstration

Four real savefiles for [issue #644](https://github.com/diegoami/neo-angband-tracker/issues/644)
(the borg's light map takes `BI_LIGHT` as its radius, one grid more than
the game lights): a dark one-grid-wide corridor (no glow anywhere, so
the torch is the only light) that the character walked before the save,
with one awake monster standing in the dark just beyond the torch's
real light. The saves are
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
| `lightmap644-lure.sav` | Played by a **level 1 Human Druid with a whip** (12 HP, 1d3 damage — too weak to out-damage a breeder): the corridor runs from her west dead end east to an **up staircase** — the way she came down, and the only thing on her map worth walking to — and a **white worm mass** (poison touch, breeding) has spawned on the middle grid, two grids east, in the dark. |
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
1d3 damage — too weak to out-damage a breeder) in the corridor she
came down: the level memory covers the whole corridor and the **up
staircase** at its east end, and nothing lies west of her. A **white
worm mass** (poison touch, `MULTIPLY` — it breeds) spawned on the
middle grid, two grids east, in the dark outside the torch's real
light. The saveplay does the same step-east/step-back dance — the
moment she glimpses the worms and steps back — then runs the
production per-turn brain (`borg_think()`) and reads the borg's own
notes:

| Build | The worm after the pass | Danger on its grid | What the borg does next |
| --- | --- | --- | --- |
| base `574b618c3` | forgotten: `# Forgetting a monster 'white worm mass' at (12,12)`, its grid still marked `light=1` | **0** | `# Needing to get back on correct depth` → `# Flowing toward up-stairs at cost 4` → `# Looking for stairs. Goal_less, Fleeing.` — its first key is `'6'`, **a step east**: the climb to the staircase it remembers, straight through the grid the forgotten worm stands on. |
| fix `d8d31359e` | remembered: the record sits on the worm's grid | **35** (vs 12 HP) | `# Needing to get back on correct depth` — the same goal — then `# Scary guy on level.` → `# Fleeing and leaving the level. (scaryguy)` → `# Reading Scroll of Word of Recall.` It evacuates. |

Same druid, same goal, same stairs: the forget does not touch what the
borg wants, it deletes the *fear on the way*. The original's first
movement key is a step east toward a breeding poison mass that its
light map swears is a lit, empty floor — the walk back into the thing
it just saw. The corrected borg keeps the record, prices that grid at
35 against its 12 hit points, and spends the recall scroll instead.
A level 20 Warrior prices the same worm at 13 and shrugs; the level 1
Druid prices it at 35 against 12 hit points, and the forget zeroes
exactly that.

## Replay it

```sh
python3 demo/run-save-demo.py            # builds base and fix, writes both saves, replays
python3 demo/run-save-demo.py --skip-build   # reuse existing out/source-*/build-* trees
```

The runner asserts the kept/deleted matrix (retreat/worm/lure: base 0,
fix 1; stay: 1 and 1, logs byte-identical), and for the lure that the
base borg's first movement key is the step east (`'6'`, 54) while the
fixed borg queues no step at all (it recalls), and writes
`demo-result.json` with the
sha256 of every log and save. Heavy steps hold
`/tmp/angband-validation.lock`. This run:

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix
  `d8d31359ec2ef9c73c9d9aa4b92579dc73a3b5fb` (branch `task/lightmap-644`,
  PR #660).
- `lightmap644-retreat.sav` sha256 `c1a6857a0d1663be`...,
  `lightmap644-worm.sav` sha256 `50cf6fba50bcef86`...,
  `lightmap644-lure.sav` sha256 `699675b3b4202206`...,
  `lightmap644-stay.sav` sha256 `b3682099aa84b9d0`...
  (full digests in `demo-result.json`).

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

What to watch in game:

- **`lightmap644-lure.sav` — the walk and the forget.** Activate the
  borg (`Ctrl+Z`, confirm once, then `z` to run it, or `x` to step it
  one action at a time). Its notes show the climb east it remembers:
  `# Needing to get back on correct depth`, `# Flowing toward
  up-stairs`. When its torch reveals the worm at the corner it reacts
  to the *visible* danger — and the moment it is two grids west again
  with the worm in the dark: on the **base** build the message log
  shows `# There was a monster 'White worm mass' at (12,12)` followed
  by `# Forgetting a monster 'White worm mass' at (12,12)`, and the
  fear evaporates — the climb east resumes straight through the corner
  while the worms breed and bite a 12 HP druid; on the **fixed** build
  no forget line ever appears, the worm stays on the borg's map, and
  the notes stay on fleeing (scary guy, the Word of Recall).
- **`lightmap644-worm.sav` / `lightmap644-retreat.sav`** — the same
  watch with the weak monster / the ant pair: put the monster in your
  torch's light, step the borg once so it records it, let it step out
  of the light, then watch the next update's message log — base
  forgets (`# There was a monster ...` then the expunge), fix keeps.
- **`lightmap644-stay.sav`** — the control: the ant stands inside the
  real light, every update re-observes it, and both builds keep it.

For the deterministic before/after comparison, always use `saveplay.c`
(the runner above): it freezes the world right after the step-back
(the issue's "monster waiting just outside its light") and shows the
one turn where the two builds diverge — the base's step east versus
the fix's recall.
