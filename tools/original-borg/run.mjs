import { createHash } from "node:crypto";
import {
	readFileSync,
	writeFileSync,
	mkdirSync,
	createWriteStream,
} from "node:fs";
import { resolve, dirname, join } from "node:path";
import { pathToFileURL } from "node:url";
const a = Object.fromEntries(
	process.argv
		.slice(2)
		.reduce(
			(r, v, i, all) =>
				i % 2 ? r : (r.push([v.slice(2), all[i + 1]]), r),
			[],
		),
);
for (const k of ["bundle", "seed", "turns", "out"])
	if (!a[k]) throw new Error("Missing --" + k);
for (const k of ["seed", "turns", "decisions"])
	if (
		a[k] !== undefined &&
		(!Number.isSafeInteger(Number(a[k])) ||
			(k !== "seed" && Number(a[k]) < 1))
	)
		throw new Error("Invalid --" + k);
const bundle = resolve(a.bundle),
	out = resolve(a.out);
mkdirSync(out, { recursive: false });
const build = JSON.parse(readFileSync(join(dirname(bundle), "build.json")));
const states = createWriteStream(join(out, "states.jsonl")),
	commands = createWriteStream(join(out, "commands.jsonl"));
const messageFile = createWriteStream(join(out, "messages.jsonl"));
const input = {
	seed: Number(a.seed),
	race: a.race,
	class: a.class,
	turns: Number(a.turns ?? 100000),
	decisions: Number(a.decisions ?? 10000000),
};
const started = performance.now();
let record;
try {
	const sha = (p) =>
		createHash("sha256").update(readFileSync(p)).digest("hex");
	if (sha(bundle) !== build.bundleSha256)
		throw new Error("Bundle hash mismatch");
	for (const [file, expected] of Object.entries(build.contentFiles))
		if (sha(join(dirname(bundle), "pack", file)) !== expected)
			throw new Error("Content hash mismatch: " + file);
	const m = await import(pathToFileURL(bundle));
	record = await m.run({
		...input,
		packDir: join(dirname(bundle), "pack"),
		manifest: build.borg.manifest.data,
		onState: (r) => states.write(JSON.stringify(r) + "\n"),
		onCommand: (r) => commands.write(r),
		onMessage: (r) => messageFile.write(JSON.stringify(r) + "\n"),
	});
} catch (e) {
	record = { outcome: "harness_failure", error: String(e?.stack ?? e) };
}
record = {
	schema: "neo-original/run@1",
	...record,
	inputs: input,
	provenance: build,
	runtime: {
		node: process.version,
		platform: process.platform,
		arch: process.arch,
	},
	wallSeconds: (performance.now() - started) / 1000,
};
writeFileSync(join(out, "run.json"), JSON.stringify(record, null, 2) + "\n");
await Promise.all([
	new Promise((r) => states.end(r)),
	new Promise((r) => commands.end(r)),
	new Promise((r) => messageFile.end(r)),
]);
console.log(
	JSON.stringify({
		out: join(out, "run.json"),
		outcome: record.outcome,
		turn: record.turn,
		causeOfDeath: record.causeOfDeath,
		wallSeconds: record.wallSeconds,
	}),
);
process.exitCode = record.outcome === "harness_failure" ? 1 : 0;
