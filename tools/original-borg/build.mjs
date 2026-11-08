import { readFileSync, writeFileSync, mkdirSync, cpSync } from "node:fs";
import { resolve, join, dirname } from "node:path";
import { createRequire } from "node:module";
import { execFileSync } from "node:child_process";
import { createHash } from "node:crypto";
import { fileURLToPath } from "node:url";
const args = Object.fromEntries(
	process.argv
		.slice(2)
		.reduce(
			(a, v, i, all) =>
				i % 2 ? a : (a.push([v.slice(2), all[i + 1]]), a),
			[],
		),
);
for (const name of ["engine-dir", "borg-dir", "out"])
	if (!args[name]) throw new Error("Missing --" + name);
const engine = resolve(args["engine-dir"]),
	borg = resolve(args["borg-dir"]),
	out = resolve(args.out);
const git = (dir, ...a) =>
	execFileSync("git", ["-C", dir, ...a], { encoding: "utf8" }).trim();
for (const [dir, pin] of [
	[engine, "5a9cab20621278b74f8376996eba1f6fda14c523"],
	[borg, "014c810466ec7ac0b9b7a83416e075238c72edd0"],
]) {
	if (git(dir, "rev-parse", "HEAD") !== pin)
		throw new Error("Unexpected upstream source pin: " + dir);
	if (
		git(dir, "diff", "HEAD", "--numstat")
			.split("\n")
			.filter(Boolean)
			.some((row) => !/^0\t0\t/.test(row))
	)
		throw new Error("Tracked source contents changed: " + dir);
	const modes = git(dir, "diff", "HEAD", "--summary");
	if (
		modes &&
		(dir !== engine ||
			modes
				.split("\n")
				.some(
					(row) =>
						!/^mode change 100644 => 100755 packages\/mod-sdk\/bin\/neo-angband-mod-(build|check)\.mjs$/.test(
							row.trim(),
						),
				))
	)
		throw new Error("Unapproved tracked mode changes");
}
mkdirSync(out, { recursive: false });
const tool = dirname(fileURLToPath(import.meta.url));
const require = createRequire(join(engine, "packages/web/package.json"));
const { build } = await import(require.resolve("vite"));
const source = readFileSync(join(tool, "session.ts"), "utf8")
	.replaceAll('"ENGINE/', JSON.stringify(engine + "/").slice(0, -1))
	.replaceAll('"BORG/', JSON.stringify(borg + "/").slice(0, -1));
writeFileSync(join(out, "entry.ts"), source);
cpSync(join(tool, "pack.ts"), join(out, "pack.ts"));
await build({
	configFile: false,
	root: join(engine, "packages/web"),
	logLevel: "warn",
	define: {
		"import.meta.env.DEV": "false",
		"import.meta.env.PROD": "true",
		"import.meta.env.MODE": '"production"',
	},
	resolve: {
		alias: [
			{
				find: "@rpgm-tools/neo-angband-core/log",
				replacement: join(engine, "packages/core/dist/log.js"),
			},
			{
				find: "@rpgm-tools/neo-angband-core",
				replacement: join(engine, "packages/core/dist/index.js"),
			},
			{
				find: "@rpgm-tools/neo-angband-mod-sdk",
				replacement: join(engine, "packages/mod-sdk/dist/index.js"),
			},
		],
	},
	ssr: { noExternal: true },
	build: {
		ssr: join(out, "entry.ts"),
		outDir: out,
		emptyOutDir: false,
		target: "node22",
		minify: false,
		rollupOptions: {
			output: {
				entryFileNames: "bundle.mjs",
				inlineDynamicImports: true,
			},
		},
	},
});

cpSync(join(engine, "packages/content/pack"), join(out, "pack"), {
	recursive: true,
});
const sha = (p) => createHash("sha256").update(readFileSync(p)).digest("hex");
writeFileSync(
	join(out, "build.json"),
	JSON.stringify(
		{
			schema: "neo-original-build@1",
			versions: Object.fromEntries(
				["core", "content", "mod-sdk"].map((n) => [
					n,
					JSON.parse(
						readFileSync(
							join(engine, "packages", n, "package.json"),
						),
					).version,
				]),
			),
			appliedPatches: [],
			lockSha256: sha(join(engine, "pnpm-lock.yaml")),
			engine: {
				repository: "neostryder/neo-angband",
				commit: git(engine, "rev-parse", "HEAD"),
				dirty: git(engine, "status", "--porcelain") !== "",
				version: JSON.parse(readFileSync(join(engine, "package.json")))
					.version,
			},
			borg: {
				repository: "neostryder/neo-angband-mod-borg",
				commit: git(borg, "rev-parse", "HEAD"),
				dirty: git(borg, "status", "--porcelain") !== "",
				manifest: JSON.parse(readFileSync(join(borg, "manifest.json"))),
			},
			bundleSha256: sha(join(out, "bundle.mjs")),
			packSha256: createHash("sha256")
				.update(
					execFileSync(
						"git",
						[
							"-C",
							engine,
							"ls-tree",
							"-r",
							"HEAD",
							"packages/content/pack",
						],
						{ encoding: "utf8" },
					),
				)
				.digest("hex"),
			officialHostModules: Object.fromEntries(
				[
					"mod-api2-host.ts",
					"intent-gate.ts",
					"mod-context.ts",
					"input-snapshot.ts",
				].map((f) => [f, sha(join(engine, "packages/web/src", f))]),
			),
			contentFiles: Object.fromEntries(
				(await import("node:fs"))
					.readdirSync(join(out, "pack"))
					.filter((f) => f.endsWith(".json"))
					.sort()
					.map((f) => [f, sha(join(out, "pack", f))]),
			),
			toolSha256: Object.fromEntries(
				["session.ts", "pack.ts", "build.mjs", "run.mjs"].map((f) => [
					f,
					sha(join(tool, f)),
				]),
			),
		},
		null,
		2,
	) + "\n",
);
