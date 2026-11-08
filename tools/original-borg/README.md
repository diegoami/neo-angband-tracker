# Original Neo Borg headless measurement

This tool measures original upstream Neo Angband and its original Borg, without Mandos, our port repairs or C corrections. It is a developer Core/API2 host, not the desktop application. The controller and the API2 host implementation are imported unchanged from the frozen original repositories. The adapter owns birth, a command buffer, store modal lifecycle, stopping and observation.

Frozen runtime: engine `neostryder/neo-angband@5a9cab20621278b74f8376996eba1f6fda14c523`; Borg `neostryder/neo-angband-mod-borg@014c810466ec7ac0b9b7a83416e075238c72edd0`. Engine package strings remain 1.23.0-beta.3, while this original Borg source identifies as 1.4.0-beta.5. Exact commits take precedence over version strings.

## Build once, run separately

Use Node 22 or newer. In the original engine checkout, `pnpm install --frozen-lockfile && pnpm build`. Then:

```sh
node tools/original-borg/build.mjs --engine-dir /path/to/original-engine --borg-dir /path/to/original-borg --out /fresh/built
node tools/original-borg/run.mjs --bundle /fresh/built/bundle.mjs --seed 42 --race Human --class Warrior --turns 100000 --out /fresh/run-42
```

The output directories must not exist. The built bundle, pack and manifest are portable and need no source checkout or npm dependencies at run time. The engine's own Vite build transforms its original host's import.meta.glob content imports. The tool rejects different source commits and tracked content edits; pnpm's two executable-mode-only changes to SDK bin scripts are allowed and recorded as dirty. No source text is rewritten.

`--decisions` is an optional safety bound on Core loop calls (default 10000000), independent of controller callbacks and issued command counts. `--turns` counts Core game ticks from ordinary birth, not Borg decisions. At normal speed, ten ticks fund a normal 100-energy action. The cap is observed at the first Core loop boundary at/after the requested counter and can overshoot. The report records actual ticks, units and overshoot. The C game's numeric seed names a different game: comparisons are distributional and never matched worlds.

## Evidence

`run.json` (neo-original/run@1) records outcome, birth race/class/stats/options/inventory, final counters/HP/energy, cause of death, final known-world snapshot, recent messages and commands, first tick at each depth, effective Borg settings, source/package/content/bundle/tool hashes and wall time. `states.jsonl`, `commands.jsonl` and `messages.jsonl` stream evidence while the run progresses. `playSha256` hashes the ordered command event stream as UTF-8 canonical JSON plus LF: keys sorted recursively, array order retained, undefined object fields omitted. Command events include tick, exact submitted intent and acceptance result.

Outcomes are death, turn_cap, stalled, borg_stopped, decision_cap and harness_failure. Death, stall and ordinary stops exit zero; harness failure exits one. A process timeout is classified separately by the calling suite and retains streamed evidence. No timeout, stall or UI failure counts as a death. Repeated controls are not additional independent lives.

## Host authority and limits

The original `createApi2Host`, `createIntentGate`, `modPluginContext` and `buildInputSnapshot` implementations are bundled directly from the engine checkout. The original plugin's start/controller/stop, RNG, views and loadout logic run unchanged. Original manifest grants are used plus the original host's supported `developerInProcess` engine:access grant. This is explicit developer execution of the actual Borg source, not a claim of stock desktop digest/origin verification. The truth namespace permits only the original Borg's perceived monster/race reads. Hidden map and omniscient store stock are not granted.

Stores use the original Core registered buy/sell/exit transactions and observe stock only while the adapter's store modal is open. The adapter mirrors the frontend's distinction between store modal state and shop terrain; it does not repair the original Borg's shop knowledge. Frontend rendering, message pauses and the browser's interactive cast/item/modal flows are outside this profile. An actual Core item-choice request immediately becomes harness_failure; the tool never chooses an item or synthesizes a policy answer. This suite can establish behavior in the named headless profile, not that every difference from C originates in Borg policy.

The stock `pnpm mod-runner` is unsuitable for a life suite: it starts at depth1 and runs a short fixed conformance script with no life cap/birth-class API. This independent entry point does not change that command or production source.
