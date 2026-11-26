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
| `lightmap644-retreat.sav` | The ant holds the corridor's last floor, two grids east of the player, in the dark outside the torch's real light (the corridor was walked, so the level's memory covers that floor). |
| `lightmap644-stay.sav` | Control: the ant stands one grid east, inside the real light, visible. |

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

| Build | The ant's grid after the pass | The record |
| --- | --- | --- |
| base `574b618c3` | `light=1` — the borg's light map still marks the dark grid lit from two grids away | deleted as one that "did not really move": `KILLS kills=0`, the map keeps only `@` and `a` |
| fix `d8d31359e` (`task/lightmap-644`) | `light=0` — the grid is dark | kept at (12,12): `?` on the map, right at the ant |

The control (`lightmap644-stay.sav`) keeps the record in both builds with
byte-identical logs.

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

What to watch on the retreat save: walk east one grid yourself (the
torch lights the ant at the corridor's end), walk back west (the ant
goes dark), then step the borg. On the **base** build the message log
shows `# There was a monster 'Soldier ant' at (12,12)` — the borg judged
the dark grid viewable and then expunged the record ("did not really
move"). On the **fixed** build no such message appears: the grid is
dark, the record is not viewable, and the ant stays in the borg's
memory. The `stay` save loads identically in both. For the deterministic
before/after comparison, use `saveplay.c` (the runner above): on a bare
arena with no stairs or items the borg goes idle after its startup
checks, so free-running play shows the divergence only around the
single step that processes the aged record.
