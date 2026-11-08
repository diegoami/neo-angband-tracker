import { strict as assert } from "node:assert";
import { readFileSync, writeFileSync, mkdirSync } from "node:fs";
import { spawnSync } from "node:child_process";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { createHash } from "node:crypto";
const bundle = resolve(process.argv[2]),
	out = resolve(process.argv[3]);
mkdirSync(out, { recursive: false });
const tool = dirname(fileURLToPath(import.meta.url));
const cases = [
	["cap-a", 42, "Human", "Warrior", 2000],
	["cap-b", 42, "Human", "Warrior", 2000],
	["death-a", 2, "Human", "Druid", 20000],
	["death-b", 2, "Human", "Druid", 20000],
	["old-shop-control", 132877853, "Half-Elf", "Mage", 12000],
	["100000-tick-limit", 42, "Human", "Warrior", 100000],
	["old-item-choice-control", 807683971, "Human", "Ranger", 100000],
	["unsupported-birth", 42, "NoSuchRace", "Warrior", 2000],
];
const rows = [];
for (const [label, seed, race, cls, turns] of cases) {
	const dir = join(out, label);
	const result = spawnSync(
		process.execPath,
		[
			join(tool, "run.mjs"),
			"--bundle",
			bundle,
			"--seed",
			String(seed),
			"--race",
			race,
			"--class",
			cls,
			"--turns",
			String(turns),
			"--out",
			dir,
		],
		{ encoding: "utf8", timeout: 180000 },
	);
	writeFileSync(
		join(out, label + ".process.json"),
		JSON.stringify(
			{
				status: result.status,
				signal: result.signal,
				error: result.error?.message,
				stdout: result.stdout,
				stderr: result.stderr,
			},
			null,
			2,
		),
	);
	if (result.error) throw result.error;
	const record = JSON.parse(readFileSync(join(dir, "run.json")));
	rows.push({ label, ...record });
	console.log(
		JSON.stringify({
			label,
			outcome: record.outcome,
			turn: record.turn,
			causeOfDeath: record.causeOfDeath,
			wallSeconds: record.wallSeconds,
		}),
	);
	if (label !== "unsupported-birth" && record.outcome !== "harness_failure") {
		assert.equal(record.birth.race, race);
		assert.equal(record.birth.class, cls);
		assert.equal(record.birth.stats.length, 5);
		assert.equal(typeof record.settings["borg.playsRisky"], "boolean");
		assert.ok(record.finalKnownWorld);
		if (record.outcome === "turn_cap") assert.ok(record.turn >= turns);
	}
}
for (const [a, b] of [
	["cap-a", "cap-b"],
	["death-a", "death-b"],
]) {
	for (const file of ["commands.jsonl", "states.jsonl", "messages.jsonl"])
		assert.equal(
			readFileSync(join(out, a, file), "utf8"),
			readFileSync(join(out, b, file), "utf8"),
		);
	const left = { ...rows.find((r) => r.label === a) },
		right = { ...rows.find((r) => r.label === b) };
	delete left.wallSeconds;
	delete right.wallSeconds;
	delete left.label;
	delete right.label;
	assert.deepEqual(left, right);
}
const death = rows.find((r) => r.label === "death-a");
assert.equal(death.outcome, "death");
assert.ok(death.causeOfDeath);
assert.ok(death.messages.length);
assert.ok(death.lastCommands.length);
assert.ok(death.hp < 0);
assert.equal(
	rows.find((r) => r.label === "unsupported-birth").outcome,
	"harness_failure",
);
const commands = readFileSync(join(out, "cap-a", "commands.jsonl"));
const hash = (x) => createHash("sha256").update(x).digest("hex");
assert.equal(hash(commands), rows[0].playSha256);
assert.notEqual(
	hash(Buffer.concat([commands, Buffer.from('{"different":true}\n')])),
	rows[0].playSha256,
);
writeFileSync(
	join(out, "pilot-summary.json"),
	JSON.stringify(
		{
			schema: "neo-original-pilot@1",
			deterministicPairs: 2,
			rows: rows.map((r) => ({
				label: r.label,
				outcome: r.outcome,
				turn: r.turn,
				causeOfDeath: r.causeOfDeath,
				wallSeconds: r.wallSeconds,
				commands: r.commands,
				error: r.error,
			})),
		},
		null,
		2,
	) + "\n",
);
