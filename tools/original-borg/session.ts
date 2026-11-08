import * as core from "ENGINE/packages/core/dist/index.js";
import {
	createApi2Host,
	disposeApi2Host,
} from "ENGINE/packages/web/src/mod-api2-host.ts";
import { createIntentGate } from "ENGINE/packages/web/src/intent-gate.ts";
import {
	modPluginContext,
	setModComposedRecords,
	setModRegistries,
	setModDriverControl,
} from "ENGINE/packages/web/src/mod-context.ts";
import definition from "BORG/plugin.ts";
import { borgCfg, buffTimerSafetyNetEnabled } from "BORG/src/trait/config.ts";
import { loadPack } from "./pack.ts";
import { readFileSync, readdirSync } from "node:fs";
import { join } from "node:path";
import { createHash } from "node:crypto";
const canonical = (v: any): string =>
	v === null || typeof v !== "object"
		? JSON.stringify(v)
		: Array.isArray(v)
			? "[" + v.map(canonical).join(",") + "]"
			: "{" +
				Object.keys(v)
					.sort()
					.filter((k) => v[k] !== undefined)
					.map((k) => JSON.stringify(k) + ":" + canonical(v[k]))
					.join(",") +
				"}";
export async function run(o: any) {
	const pack = loadPack(o.packDir),
		game = core.startGame(pack, {
			seed: o.seed,
			depth: 0,
			...(o.race ? { raceName: o.race } : {}),
			...(o.class ? { className: o.class } : {}),
		});
	const state = game.state,
		p = state.actor.player;
	if (o.race && p.race.name !== o.race)
		throw new Error("Assigned race was not born: " + o.race);
	if (o.class && p.cls.name !== o.class)
		throw new Error("Assigned class was not born: " + o.class);
	state.actor.player.noscore |= core.NOSCORE.BORG;
	const flags = Object.fromEntries(
		o.manifest.rules.map((r: any) => [r.flag, r.default]),
	);
	// Same hidden-data grants as original Borg: perceived-monster truth only, never actual map/store stock.
	const grants = {
		has: (name: string) =>
			name === "engine:access" || o.manifest.capabilities.includes(name),
	};
	const records = Object.fromEntries(
		readdirSync(o.packDir)
			.filter((f) => f.endsWith(".json"))
			.flatMap((f) => {
				const v = JSON.parse(readFileSync(join(o.packDir, f), "utf8"));
				return Array.isArray(v.records)
					? [[f.slice(0, -5), v.records]]
					: [];
			}),
	);
	const deps = {
		...state.knownWorldDeps,
		resolver: new core.ContentIdResolver({
			objects: game.booted.registries.objects,
			monsters: game.booted.registries.monsters,
			playerRaces: game.players.races,
			playerClasses: game.players.classes,
		}),
		reg: game.booted.registries.objects,
		describe: (obj: any) =>
			core.describeObject(
				state,
				obj,
				core.ODESC.PREFIX | core.ODESC.FULL,
			),
		describeStore: (obj: any) =>
			core.describeObject(
				state,
				obj,
				core.ODESC.PREFIX | core.ODESC.FULL | core.ODESC.STORE,
			),
		ignored: (obj: any) => state.isIgnored?.(obj) ?? false,
		glyphs: new core.GlyphTable({
			features: game.booted.registries.features.allFeatures(),
			kinds: game.booted.registries.objects.kinds,
			races: game.booted.registries.monsters.races,
			traps: game.booted.registries.traps,
			flavors: game.booted.registries.objects.flavors,
		}).agentGlyphs(),
	};
	let queued: any = null,
		consumed: any = null,
		inputNeeded = false,
		store: number | null = null,
		released: string | null = null,
		callbacks = 0,
		decisions = 0,
		loopCalls = 0,
		maxDepth = 0,
		maxLevel = p.lev,
		commandEvents = 0;
	const messages: any[] = [],
		logs: string[] = [],
		tail: any[] = [],
		census: any = {},
		firstDepthTurns: any = { "0": state.turn },
		hash = createHash("sha256");
	const event = (row: any) => {
		const line = canonical(row) + "\n";
		hash.update(line);
		o.onCommand(line);
		tail.push(row);
		if (tail.length > 100) tail.shift();
		commandEvents++;
	};
	state.nextCommand = () => {
		if (queued) {
			const held = queued;
			queued = null;
			if (core.tokenIsCurrent(state, held.token)) {
				consumed = held.command;
				return consumed;
			}
		}
		inputNeeded = true;
		return null;
	};
	const oldMsg = state.msg;
	state.msg = (text: string) => {
		oldMsg?.(text);
		messages.push({ turn: state.turn, text });
		o.onMessage?.({ turn: state.turn, text });
		if (messages.length > 200) messages.shift();
	};
	const phase = () =>
		state.isDead ? "dead" : store === null ? "play" : "store";
	const source = {
		state: () => state,
		viewDeps: () => deps,
		phase,
		messagePending: () => false,
		prompt: () => null,
		frame: () => null,
		driver: () => ({ kind: "controller", owner: "borg" }),
		knownLevel: () =>
			core.createAgentView(state, undefined, deps, grants).knownLevel!(),
	};
	const originalGate = createIntentGate({
		state,
		registry: game.registry,
		snapshotSource: source,
		push: (command: any) => {
			queued = { command, token: core.inputToken(state) };
		},
		advance: () => {},
		storeCommand: (command: any) => {
			queued = { command, token: core.inputToken(state) };
			return true;
		},
	});
	const gate = {
		...originalGate,
		submit: (token: any, intent: any) => {
			const result = originalGate.submit(token, intent);
			event({
				type: "intent",
				turn: state.turn,
				intent,
				accepted: result.accepted,
				...(result.accepted
					? {}
					: { reason: result.reason ?? result.code }),
			});
			if (intent.kind === "command") {
				decisions++;
				census[intent.command.code] =
					(census[intent.command.code] ?? 0) + 1;
			}
			return result;
		},
	};
	let memory: any = null;
	setModRegistries(game.booted.registries);
	setModComposedRecords(records);
	setModDriverControl({
		current: () => ({ kind: "controller", owner: "borg" }),
		setStatus: () => {},
		release: (_id: string) => {},
	});
	const context: any = {
		...modPluginContext(
			"borg",
			flags,
			state,
			{},
			{
				newCharacter: true,
				capabilities: grants,
				prefs: { get: () => memory, set: (v: any) => (memory = v) },
				registries: game.booted.registries,
				snapshotSource: source,
			},
		),
		controllerArmed: true,
		intent: gate,
		log: (s: string) => logs.push(s),
		controller: {
			setStatus: () => {},
			markNondeterministic: () => {
				throw new Error("Unexpected nondeterministic controller");
			},
			release: (reason: string) => {
				released = reason;
				event({ type: "release", turn: state.turn, reason });
			},
		},
	};
	const mod = createApi2Host(context, {
		execution: "in-process",
		developerInProcess: true,
		grants,
		firstParty: { truth: true },
		viewDeps: deps,
		intent: gate,
		installStorage: { get: () => memory, set: (v: any) => (memory = v) },
	});
	const gear = () =>
		[...state.gear.store.entries()].map(([handle, obj]) => ({
			handle,
			location: p.equipment.includes(handle)
				? "equipped"
				: state.gear.quiver?.includes(handle)
					? "quiver"
					: "pack",
			slot: p.equipment.indexOf(handle),
			description: core.describeObject(
				state,
				obj,
				core.ODESC.PREFIX | core.ODESC.FULL,
			),
			tval: obj.tval,
			sval: obj.sval,
			number: obj.number,
			weight: obj.weight,
		}));

	const birth = {
		race: p.race.name,
		class: p.cls.name,
		method: "ordinary startGame roller; assigned race/class only",
		stats: [...p.statBirth],
		options: state.options?.snapshot().birth ?? {},
		inventory: gear(),
		gold: p.au,
		hp: p.chp,
		maxHp: p.mhp,
	};
	let lastPeriodic = 0,
		lastDepth = 0;
	const snapshot = (kind: string) => {
		maxDepth = Math.max(maxDepth, state.chunk.depth);
		maxLevel = Math.max(maxLevel, p.lev, p.maxLev);
		firstDepthTurns[state.chunk.depth] ??= state.turn;
		return {
			kind,
			turn: state.turn,
			decisions,
			callbacks,
			loopCalls,
			depth: state.chunk.depth,
			maxDepth,
			level: p.lev,
			maxLevel,
			hp: p.chp,
			maxHp: p.mhp,
			energy: state.actor.energy,
		};
	};
	const observe = () => {
		const s = snapshot("periodic");
		if (decisions >= lastPeriodic + 100) {
			o.onState(s);
			lastPeriodic = decisions;
		}
		if (s.depth !== lastDepth) {
			o.onState({ ...s, kind: "depth" });
			lastDepth = s.depth;
		}
	};
	let error: any = null,
		started = false;
	try {
		if (
			!definition.armed?.({
				id: "borg",
				flags,
				armed: true,
				engine: { core, state },
			})
		)
			throw new Error("Original Borg refused armed birth");
		await definition.start?.(mod);
		started = true;
		const failure = logs.find((s) =>
			s.startsWith("the Borg could not start:"),
		);
		if (failure) throw new Error(failure);
		o.onState(snapshot("birth"));
		while (
			loopCalls < o.decisions &&
			state.turn < o.turns &&
			!state.isDead &&
			released === null
		) {
			consumed = null;
			inputNeeded = false;
			const before = { ...state.actor.grid };
			const status = core.runGameLoop(state, game.registry);
			loopCalls++;
			if (status === core.LOOP_STATUS.LEVEL_CHANGE) {
				game.changeLevel(state.targetDepth ?? state.chunk.depth + 1);
				state.generateLevel = false;
			}
			if (consumed?.code === "shop-exit") store = null;
			else if (
				before.x !== state.actor.grid.x ||
				before.y !== state.actor.grid.y ||
				consumed?.code === "hold"
			) {
				const shop = state.chunk.feature(state.actor.grid).shopnum;
				store =
					shop > 0 && !core.playerIsShapechanged(state)
						? shop - 1
						: null;
			}
			if (store !== null && state.stores?.[store])
				core.observeKnownStore(state, state.stores[store]!, deps);
			observe();
			if (state.itemRequest)
				throw new Error(
					"Original Core requested item-choice UI unsupported by this measurement host",
				);
			if (
				state.isDead ||
				status === core.LOOP_STATUS.DEAD ||
				status === core.LOOP_STATUS.DEATH_CONFIRM
			)
				break;
			if (status === core.LOOP_STATUS.STOPPED)
				throw new Error("Engine stopped without controller release");
			if (state.turn >= o.turns || loopCalls >= o.decisions) break;
			if (inputNeeded) {
				callbacks++;
				const input = await mod.view.input();
				if (!input.ok) throw new Error(input.message);
				await definition.controller?.(mod, input.value);
			}
		}
	} catch (e) {
		error = String((e as Error).stack ?? e);
	} finally {
		if (started) await definition.stop?.();
		disposeApi2Host(mod);
	}
	const finalKnownWorld = core.buildKnownWorld(state, deps, grants);
	const stop = snapshot("stop");
	o.onState(stop);
	const stall =
		released &&
		(released === "The Borg had no command to give." ||
			released === "The game kept refusing the Borg's commands." ||
			/^The Borg's .+ times running.$/.test(released))
			? { reason: released, turn: state.turn, decisions }
			: null;
	const outcome = error
		? "harness_failure"
		: state.isDead
			? "death"
			: stall
				? "stalled"
				: released
					? "borg_stopped"
					: state.turn >= o.turns
						? "turn_cap"
						: "decision_cap";
	return {
		outcome,
		error,
		seed: o.seed,
		birth,
		settings: flags,
		cfg: { ...borgCfg() },
		crossModFlags: { "bugfix.borgFixes": buffTimerSafetyNetEnabled() },
		...stop,
		causeOfDeath: state.isDead ? p.diedFrom : null,
		stall,
		stopped: stall ? null : released,
		firstDepthTurns,
		commands: census,
		playSha256: hash.digest("hex"),
		commandEvents,
		lastCommands: tail,
		messages,
		logs,
		finalKnownWorld,
		finalInventory: gear(),
		turnCap: {
			requested: o.turns,
			actual: state.turn,
			overshoot: Math.max(0, state.turn - o.turns),
			unit: "core_game_tick",
			normalActionTicks: 10,
			exact: state.turn === o.turns,
			guaranteedExact: false,
		},
		profile: {
			name: "original-source-developer-api2",
			host: "original createApi2Host/createIntentGate/buildInputSnapshot",
			store: "Core transaction commands; adapter modal lifecycle",
			unsupported:
				"item-choice UI, frontend rendering and browser modal flows",
			grants: [...o.manifest.capabilities, "engine:access"],
			truth: "original perceived monster/race truth only; no actual map/store stock",
		},
	};
}
