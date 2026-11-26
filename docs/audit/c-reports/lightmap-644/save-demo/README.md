# lightmap-644 synthetic savefile demonstration

Two real savefiles for [issue #644](https://github.com/diegoami/neo-angband-tracker/issues/644)
(the borg's light map takes `BI_LIGHT` as its radius, one grid more than
the game lights): a Human Warrior with a wooden torch in a dark
one-grid-wide dead-end corridor, and one awake soldier ant. The saves are
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
| `lightmap644-lure.sav` | The corridor turns south at a **mind flayer** (INT/WIS drain, brain smash, paralysis — a caster that lingers in the dark ring) holding the dark corner and ends in a down staircase; the borg wants the stairs, and the flayer is something to retreat from. |
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

`lightmap644-lure.sav`: the corridor turns south at a **mind flayer**
(INT/WIS drain, brain smash, paralysis — a caster, so it lingers in the
dark ring blasting you instead of closing, since monsters see without
light) holding the dark corner, and ends in a **down staircase** the
borg knows about and wants. After the same step-east/step-back dance,
the saveplay runs the production per-turn brain (`borg_think()`) and
reads the borg's own notes:

| Build | The flayer after the pass | Danger on its grid | What the borg does next |
| --- | --- | --- | --- |
| base `574b618c3` | forgotten: `# Forgetting a monster 'mind flayer' at (12,12)`, its grid and the south leg still marked `light=1` | **0** | `# Flowing toward dark-2` — a stroll west to light a dark corner (step key `4`). The borg has no idea the flayer is there. |
| fix `d8d31359e` | remembered: the record sits on the flayer's grid | **568** | `# Fleeing (failed to teleport)` → `# Leaving (failed to teleport)` → `# Reading Scroll of Word of Recall.` → `# Fleeing the level (recall)`. It tries to escape, then evacuates. |

That is the consequence in one turn: forgetting does not change the
borg's goal, it deletes the fear. The same borg, the same level, the
same stairs — one version strolls past a brain-eating caster because its
light map swears the dark corner is lit; the other remembers the flayer
and flees the level.

## Replay it

```sh
python3 demo/run-save-demo.py            # builds base and fix, writes both saves, replays
python3 demo/run-save-demo.py --skip-build   # reuse existing out/source-*/build-* trees
```

The runner asserts the kept/deleted matrix (retreat: base 0, fix 1; stay:
1 and 1, logs byte-identical) and writes `demo-result.json` with the
sha256 of every log and save. Heavy steps hold
`/tmp/angband-validation.lock`. This run:

- base `574b618c3bfec595f3503a58f8e488b1c4ae3a46`, fix
  `d8d31359ec2ef9c73c9d9aa4b92579dc73a3b5fb` (branch `task/lightmap-644`,
  PR #660).
- `lightmap644-retreat.sav` sha256
  `c1a6857a0d1663be`..., `lightmap644-stay.sav` sha256 `b3682099aa84b9d0`...
  (full digests in `demo-result.json`).

## Load a save in the real game (manual play test)

The games are built at `out/build-{base,fix}/game/angband` (X11 front
end, auto-enabled on non-Windows; `lib/` is staged next to the binary).
These builds keep their savefiles in **`<game dir>/lib/save/`** (the two
saves are staged there as `lm644-retreat` and `lm644-stay`) and read the
borg config from `out/test-user/Angband/` (staged with the same
`borg.txt` as the fixture). Always run from the `game/` directory:

```sh
cd /tmp/opencode/lightmap644-save/out/build-base/game
ANGBAND_X11_FONT=fixed ./angband -ulm644-retreat     # note: -u<name>, no space
# and the fixed build:
cd /tmp/opencode/lightmap644-save/out/build-fix/game
ANGBAND_X11_FONT=fixed ./angband -ulm644-retreat
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

What to watch in game:

- **`lightmap644-worm.sav` — the live forget.** The white worm mass is
  slow and moves at random, so it lingers. Walk east one grid (see it),
  walk back west (it goes dark), then step the borg. On the **base**
  build the message log shows `# There was a monster 'White worm mass'
  at (12,12)` and the record is expunged; on the **fixed** build the
  message never appears and the worm stays on the borg's map.
- **`lightmap644-lure.sav` — the live forget *and* the decision.** The
  mind flayer is a caster: it spends its turns casting from the dark
  ring instead of closing (monsters see without light), so it lingers
  exactly where the defect bites. Walk east one grid (see it), walk
  back west (it stays in the dark, blasting you), then step the borg:
  on the **base** build the message log shows the forget
  (`# There was a monster 'mind flayer' at (12,12)` then the expunge)
  and the borg keeps strolling — on the **fixed** build the message
  never appears and the borg's next notes turn to fleeing. The
  `saveplay` matrix freezes the world after the step-back and shows the
  same decision deterministically: stroll on the original, flee the
  level with the fix.
- **`lightmap644-retreat.sav` / `lightmap644-stay.sav`** — the ant pair;
  the ant is fast, so treat the retreat save the same way as any fast
  pursuer's (its divergence is the saveplay run).

For the deterministic before/after comparison, always use `saveplay.c`
(the runner above): on a bare arena with no stairs or items the borg
goes idle after its startup checks, so free-running play shows the
divergence only around the single step that processes the aged record.
