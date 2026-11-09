# Browse recorded Borg lives locally

Python 3.10 or newer and a normal web browser are sufficient. Node, npm, a game installation and a desktop-app build are not needed. GitHub CLI (`gh`, authenticated) is needed only when explicitly downloading artifacts.

From a fresh checkout of this proposal:

```sh
git clone --depth 1 --branch task/original-borg-headless https://github.com/diegoami/neo-angband-tracker.git neo-borg-tools
cd neo-borg-tools
python3 tools/original-borg/browse.py --github-run 42534228784
```

That command downloads **compact catalogues only**, binds a local server to `127.0.0.1` on a free port and opens your browser. Selecting a life does not download anything. Use **Download raw shard** to retrieve its evidence; the shard also contains other lives. Use **Check GitHub for new catalogues** as an ongoing run publishes more completed shards. There is no unattended polling.

To browse an existing cache without network access:

```sh
python3 tools/original-borg/browse.py --cache /path/to/artifacts
```

Standard `gh run download ... --pattern 'cells-*' --dir /path/to/artifacts` and `--pattern 'raw-*'` directory layouts work. Custom download receipts are optional; without them the browser reports that completion/run identity is unattested. A partially written or malformed artifact is shown as an error/pending observation, not a death.

`--cache` may be repeated to inspect multiple evidence roots. Different recorded attempts remain distinct even when seed/specimen names match; only byte-identical compact records are deduplicated. Raw evidence belongs to that row's original cache root. A separate download-cache fallback requires matching workflow-run receipts; unrelated runs cannot substitute logs. Unknown-run local caches can still be browsed entirely offline.

`--no-open` prints the loopback URL without launching a browser. `--port 8879` chooses a port. Stop with Ctrl-C. `--repository owner/repo` selects a different GitHub artifact source. Downloaded artifacts go to `~/.cache/neo-borg-log-browser/<run>` by default; `--download-cache` chooses a separate append-only destination. It must not be a read-only input root. Existing artifact folders are preserved; choose a fresh download cache if an earlier incomplete/corrupt download occupies the name.

## What it shows

Filter by original engine/profile, outcome, class, race, seed or cause. Select a life to inspect its recorded outcome, death/stall/error, birth, settings, counters and full compact provenance. Commands, messages and state tables support pages of100 rows and **Recent / last100**. Raw files remain available, including native final `.map` reports and Neo final inventory/known-world observations. ASCII maps preserve their original columns with horizontal scrolling.

Neo commands are the recorded controller intents and their accept/reject reason, including target coordinates and command arguments. Native C's minimal observer records Borg notes, messages and state snapshots, **not a generated keypress stream**; the browser labels that distinction and does not invent acceptance results. Native `STATE.energy` is cumulative `player.total_energy`; Neo `energy` is current `state.actor.energy`. The two energy columns are deliberately named differently.

Summary now uses a compact terminal-style character sheet. Neo HP/SP, EXP, gold, AC, speed and STR/INT/WIS/DEX/CON come only from the **current recorded final KnownWorld**, never birth values. Native final HP, level, depth and outcome come from the final compact/result observations; EXP/gold and equipment/quiver/pack sections come from the recorded native map dump. Stale `CharOutput.txt` and pre-death message HP are not used. An alive cap remains alive even if the legacy dump header says “Killed … (alive and well)”. Missing fields say **not recorded**; missing/corrupt raw artifacts still leave the compact final sheet available.

Neo inventory uses recorded final inventory/equipment/quiver arrays and slot names. Letter labels in this view are **viewer row labels**, not recorded Neo menu keys; original item handles remain visible. Native dump item letters (including skipped letters) and descriptions remain verbatim. Only recorded item `nameColor` labels select a viewer display color; no terrain or monster colors are invented.

Neo maps are a **documented terrain-symbol projection** of recorded sparse cells: walls `#`, floor `.`, stairs `< >`, doors/stores `+`, lava `~`, rubble `:`, recorded traps `^`, recorded items `*`, unmapped recorded terrain `?`. Unknown/unrecorded cells stay blank. Only recorded visible monster glyphs and the recorded player position `@` are plotted. Original terrain glyphs/colors were not captured, so this is not a pixel-faithful reconstruction. Native ASCII frames remain exact, including internal all-wall rows. The viewport centers the recorded player without changing/cropping the retained grid. Original JSON/provenance is folded into secondary details or available under Raw files.

C and Neo numerical seeds describe different games. Every item is labeled **unassessed recorded facts**. Browsing, hashes and cause strings are not a diagnosis, replay, new independent life or proof of a gameplay bug.

## Integrity and evidence preservation

Existing caches are read-only. Raw `.tgz` / `.tar.gz` archives are read directly; no raw evidence is extracted or rewritten. Outer GitHub artifact ZIP containers are unpacked only into a new browser download-cache folder. ZIP digests are checked against GitHub's digest when available. Path traversal, links and duplicate tar members are rejected or excluded; previews are bounded and paginated.

**Verify recorded hashes** explicitly hashes the selected raw archive against its `.sha256` sidecar, and selected-life files against the compact evidence manifest. Results distinguish match, mismatch, missing/error and absent expected hashes. Hashes are not claimed verified merely because a file was displayed. Binary saves get metadata/hash inspection rather than decoded text. Verification checks integrity, not source authenticity or gameplay correctness.

## Stable local API for agents

All responses are JSON. No game or replay command is available.

- `GET /api/lives?source_profile=neo-original&outcome=stalled&seed=1261438670&offset=0&limit=50`: filtered catalogue, facets, cache/pending/error status. Native API profile is `better-borg-c` because that observer captured the original C specimen; the UI calls it **Native C**.
- `GET /api/life?id=ID`: original compact record, evidence files, source-root/run association, counter meanings and raw availability.
- `GET /api/file?id=ID&file=run/commands.jsonl&offset=0&limit=100`: bounded original lines plus parsed JSONL values. Native `file=trace.txt&mode=states`, `mode=messages` and `mode=commands` filter recorded trace categories. `tail=1` returns the last bounded page. Maximum page500 rows/2MiB; exceptional oversized lines are bounded fragments, not reconstructed complete lines.
- `GET /api/presentation?id=ID`: conservative character-sheet, item groups and known-map projection with explicit source/legend/unknown-field notices. This is presentation of recorded facts, not game-state inference.
- `GET /api/observation?id=ID&field=finalInventory`: original Neo `finalInventory`, `finalKnownWorld`, `birth` or `provenance` field; unavailable native structured fields are explicitly reported.
- `GET /api/verify?id=ID`: actual archive/file hash comparison; potentially reads a full selected shard, with bounded memory.
- `POST /api/catalogues` and `POST /api/download?id=ID`: explicit GitHub downloads only when launched with `--github-run`. Both require the loopback UI's `X-Browser-Token` header. They never run automatically on a read request.

The server accepts loopback Host headers only, serves no arbitrary filesystem paths and renders recorded text as text. There is no remote-host deployment mode.

## Verification

```sh
python3 -m unittest discover -s tools/original-borg/browser -p '*_test.py' -v
python3 tools/original-borg/browser/real_smoke.py --cache /path/to/the/originals-artifacts
```

Offline tests additionally cover current-vs-birth stats, stale native dump caveats, alive legacy headers, original item letters, complete ASCII frames, sparse known maps/visible monster glyphs, unknown fields and missing/corrupt raw fallback. They also cover both native and Neo formats, rejected shop intents, native death state/map, raw/missing/hash status, receiptless standard downloads, tail/page bounds, long-line progress, archive traversal/links, distinct attempts across caches, foreign-run fallback rejection, receipt arrival and read-only HTTP/no-network behavior. The optional real-evidence smoke reads the recorded Neo shop stall1261438670, cast stall1460096338 and a native death if those artifacts are cached. It creates no lives and changes no evidence.

This browser is additive tooling on PR542. The ongoing original-source benchmark remains pinned to measurement toolkit `2668adc5ab3e2b6e194c2f1561e248b88850ea98`, engine5a9, original Borg014 and original C e473. New browser commits do not change that experiment's runner or source pins.
