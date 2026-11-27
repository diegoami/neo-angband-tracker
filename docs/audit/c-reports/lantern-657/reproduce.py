#!/usr/bin/env python3
"""Reproduce the lantern-657 native evidence: with an empty light slot the maintenance decision must wield a light source that actually produces light.

Run from a clone of diegoami/neo-angband-tracker after fetching
lantern-657-evidence. The default fixtures reference FETCH_HEAD is the
revision fetched by the setup command; the fix reference is required. Use
--full-tests to also run both native unit and frontend regression gates for
the before and after builds.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

BASE = "574b618c3bfec595f3503a58f8e488b1c4ae3a46"
DIRECTORY = "docs/audit/c-reports/lantern-657/"
OBSERVER = 'lantern657'
PHASES = ("before", "after")
# Each corrected file must equal the base file with exactly these hunks.
CHANGES = {
    'src/borg/borg-light.c': [
        ("    } else {\n        i = borg_slot(TV_LIGHT, sv_light_lantern);\n        if (i < 0) {\n            i = borg_slot(TV_LIGHT, sv_light_torch);\n        }\n\n        if (i < 0) {\n            return BORG_UNMET_NEED;\n        } else {\n            borg_keypress('w');\n            borg_keypress(all_letters_nohjkl[i]);\n            return BORG_MET_NEED;\n        }\n    }\n",
         "    } else {\n        /* Prefer a light source that is actually producing light.\n         * A spent lantern gives no light, and once it is worn the\n         * refuel branch above has no fuel left to offer, which\n         * locks out a perfectly good torch. */\n        for (i = 0; i < z_info->pack_size; i++) {\n            borg_item *item = &borg_items[i];\n\n            if (!item->iqty || !item->aware)\n                continue;\n            if (item->tval != TV_LIGHT\n                || item->sval != sv_light_lantern)\n                continue;\n            if (of_has(item->flags, OF_NO_FUEL) || item->timeout > 0)\n                break;\n        }\n        if (i >= z_info->pack_size) {\n            /* No producing lantern: torches never arrive spent. */\n            i = borg_slot(TV_LIGHT, sv_light_torch);\n            if (i < 0) {\n                /* A spent lantern is only worth wearing if oil can\n                 * light it.  A fueled lantern would already have\n                 * been taken above. */\n                i = borg_slot(TV_LIGHT, sv_light_lantern);\n                if (i >= 0 && borg_slot(TV_FLASK, sv_flask_oil) < 0)\n                    i = -1;\n            }\n        }\n\n        if (i < 0) {\n            return BORG_UNMET_NEED;\n        } else {\n            borg_keypress('w');\n            borg_keypress(all_letters_nohjkl[i]);\n            return BORG_MET_NEED;\n        }\n    }\n"),
    ],
}
# Per mode: (expected pass before, expected pass after, byte-identical control).
EXPECT = {'torchpref': (False, True, False), 'no_false_need': (False, True, False), 'fueledlantern': (True, True, True), 'spentlantern_oil': (True, True, True), 'torchonly': (True, True, True)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures-ref", default="FETCH_HEAD")
    parser.add_argument("--fix-ref", required=True)
    parser.add_argument("--full-tests", action="store_true")
    parser.add_argument("--cmake-arg", action="append", default=[])
    args = parser.parse_args()
    for command in ("git", "cmake", "flock", "cc", "make"):
        if not shutil.which(command):
            raise RuntimeError(f"Required command not found: {command}")
    repo = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
    ref = subprocess.check_output(["git", "rev-parse", args.fixtures_ref], cwd=repo, text=True).strip()
    fix = subprocess.check_output(["git", "rev-parse", args.fix_ref], cwd=repo, text=True).strip()

    def read(path):
        return subprocess.check_output(["git", "show", ref + ":" + path], cwd=repo)

    payload = {name: read(DIRECTORY + name) for name in ("scenario.c", "run-cases.py", "make_replay.py", "borg.txt")}
    for pin in (BASE, fix):
        if subprocess.run(["git", "cat-file", "-e", pin + "^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)
    fixed = {}
    for source, hunks in CHANGES.items():
        original = subprocess.check_output(["git", "show", BASE + ":" + source], cwd=repo).decode()
        fixed[source] = subprocess.check_output(["git", "show", fix + ":" + source], cwd=repo).decode()
        candidate = original
        for old, new in hunks:
            if candidate.count(old) != 1:
                raise RuntimeError(f"Expected hunk not found exactly once in {source}")
            candidate = candidate.replace(old, new, 1)
        if candidate != fixed[source]:
            raise RuntimeError(f"Source correction in {source} is not exactly the expected change")
    if sorted(subprocess.check_output(["git", "diff", "--name-only", BASE, fix], cwd=repo).decode().split()) != sorted(CHANGES):
        raise RuntimeError("Source scope changed")

    root = Path(tempfile.mkdtemp(prefix="angband-lantern-657-", dir="/tmp"))
    source, build, evidence = root / "source", root / "build", root / "evidence"
    (evidence / "profile").mkdir(parents=True)
    for name, data in payload.items():
        (evidence / name).write_bytes(data)
    (evidence / "profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source / "CMakeLists.txt"
    cmake.write_text(cmake.read_text() + f"""
add_executable({OBSERVER} "{evidence / 'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories({OBSERVER} PRIVATE src src/tests)
target_compile_definitions({OBSERVER} PRIVATE ALLOW_BORG)
target_link_libraries({OBSERVER} PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{root / 'test-user'}")
""")
    test_script = source / "tests/run-test"
    test_script.write_text(test_script.read_text().replace("~/.angband/Test", str(root / "test-user/Test")))

    def run(label, command):
        log = root / (label + ".log")
        print(f"{label}: shared validation lock; log {log}", flush=True)
        with log.open("w") as output:
            result = subprocess.run(["flock", "/tmp/angband-validation.lock", *command], cwd=repo,
                                    stdout=output, stderr=subprocess.STDOUT)
        return log, result.returncode

    log, code = run("configure", ["cmake", "-S", str(source), "-B", str(build), "-DSUPPORT_BORG=ON",
                      "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug", *args.cmake_arg])
    if code:
        print(log.read_text()[-4000:], file=sys.stderr)
        raise RuntimeError("configure failed")
    rows = {}
    gates = {}
    for phase in PHASES:
        if phase == "after":
            for path, text in fixed.items():
                (source / path).write_text(text)
        log, code = run(phase + "-build", ["cmake", "--build", str(build), "--parallel", "2"])
        if code:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"{phase} build failed")
        log, code = run(phase + "-scenarios", [sys.executable, str(evidence / "run-cases.py"), phase,
                          str(build / "game" / OBSERVER), str(evidence)])
        if code:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"{phase} scenario runner failed")
        rows[phase] = json.loads((evidence / f"{phase}-results.json").read_text())
        if args.full_tests:
            log, code = run(phase + "-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text = re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 966/966 passed (100.0%)" not in text or "Total: 5/5" not in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase] = {"units": "966/966", "frontends": "5/5"}

    controls = []
    for mode, (want_before, want_after, control) in EXPECT.items():
        before, after = rows["before"][mode], rows["after"][mode]
        if before["passed"] != want_before:
            raise RuntimeError(f"{mode}: original expected passed={want_before}, got {before['passed']}")
        if after["passed"] != want_after:
            raise RuntimeError(f"{mode}: corrected expected passed={want_after}, got {after['passed']}")
        if control:
            if (evidence / f"before-{mode}.log").read_bytes() != (evidence / f"after-{mode}.log").read_bytes():
                raise RuntimeError(f"{mode}: control logs are not byte-identical")
            controls.append(mode)
    for path, text in fixed.items():
        if (source / path).read_text() != text:
            raise RuntimeError("Disposable source not restored to the correction")
    summary = {"base": BASE, "fix": fix, "fixtures_ref": ref, "cases": rows,
               "identical_controls": controls, "full_tests": gates,
               "fixed_source_sha256": {p: hashlib.sha256((source / p).read_bytes()).hexdigest() for p in fixed}}
    (root / "result.json").write_text(json.dumps(summary, indent=2) + "\n")
    subprocess.run([sys.executable, str(evidence / "make_replay.py"), str(evidence)], check=True)
    print('VERIFIED: fail-before/pass-after on the spent-lantern-over-torch case and the no-false-need case, with the fueled-lantern, spent-lantern-with-oil and torch-only controls byte-identical, gates 966/966 unit checks and 5/5 frontend tests on both variants, at base 574b618c3', flush=True)
    print(f"Logs and result.json: {root}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}", file=sys.stderr)
        sys.exit(1)
