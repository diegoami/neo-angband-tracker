#!/usr/bin/env python3
"""Behaviour change (owner decision): compare which of two carried items the Borg sells first before and after restoring the modifier credits in its item value guess, with the guess floored at zero (a stat ring, a speed item, a control with no modifiers, an amulet with a speed penalty and an artifact with a stealth penalty).

The runner also builds the earlier fix head (credits without the floor) to
record the guessed values the floor replaces; that build runs no gates.

Run from a clone of diegoami/neo-angband-tracker after fetching
task/modcredit-70-evidence. The default fixtures reference FETCH_HEAD is the
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

BASE = "e473720ed88a0f0c7ca66815cef3000eb4a38bc3"
DIRECTORY = "docs/audit/c-reports/modcredit-70/"
OBSERVER = 'MC70Observer'
PHASES = ("before", "prior", "after")
SOURCES = ("prior", "after")
# Each corrected file must equal the base file with exactly these hunks.
# The earlier fix head (credits without the floor), built only to record the
# values the floor replaces.
PRIOR = "46c6188eccb54bd0571e0a527145f7fd7ed756c3"
PRIOR_CHANGES = {
    'src/borg/borg-item-analyze.c': [
        ('        /* No pval */\n        if (!item->pval)\n            break;\n\n        /* Give credit for stat bonuses */\n        if (of_has(item->flags, STAT_STR))\n            value += (item->pval * 200L);\n        if (of_has(item->flags, STAT_INT))\n            value += (item->pval * 200L);\n        if (of_has(item->flags, STAT_WIS))\n            value += (item->pval * 200L);\n        if (of_has(item->flags, STAT_DEX))\n            value += (item->pval * 200L);\n        if (of_has(item->flags, STAT_CON))\n            value += (item->pval * 200L);\n',
         '        /* Give credit for stat bonuses */\n        if (item->modifiers[OBJ_MOD_STR] > 0)\n            value += (item->modifiers[OBJ_MOD_STR] * 200L);\n        if (item->modifiers[OBJ_MOD_INT] > 0)\n            value += (item->modifiers[OBJ_MOD_INT] * 200L);\n        if (item->modifiers[OBJ_MOD_WIS] > 0)\n            value += (item->modifiers[OBJ_MOD_WIS] * 200L);\n        if (item->modifiers[OBJ_MOD_DEX] > 0)\n            value += (item->modifiers[OBJ_MOD_DEX] * 200L);\n        if (item->modifiers[OBJ_MOD_CON] > 0)\n            value += (item->modifiers[OBJ_MOD_CON] * 200L);\n'),
    ],
}
CHANGES = {
    'src/borg/borg-item-analyze.c': [
        ('        /* Negative "pval" is always bad */\n        if (item->pval < 0)\n            return (0L);\n\n        /* No pval */\n        if (!item->pval)\n            break;\n\n        /* Give credit for stat bonuses */\n        if (of_has(item->flags, STAT_STR))\n            value += (item->pval * 200L);\n        if (of_has(item->flags, STAT_INT))\n            value += (item->pval * 200L);\n        if (of_has(item->flags, STAT_WIS))\n            value += (item->pval * 200L);\n        if (of_has(item->flags, STAT_DEX))\n            value += (item->pval * 200L);\n        if (of_has(item->flags, STAT_CON))\n            value += (item->pval * 200L);\n',
         '        /* Give credit for stat bonuses */\n        if (item->modifiers[OBJ_MOD_STR] > 0)\n            value += (item->modifiers[OBJ_MOD_STR] * 200L);\n        if (item->modifiers[OBJ_MOD_INT] > 0)\n            value += (item->modifiers[OBJ_MOD_INT] * 200L);\n        if (item->modifiers[OBJ_MOD_WIS] > 0)\n            value += (item->modifiers[OBJ_MOD_WIS] * 200L);\n        if (item->modifiers[OBJ_MOD_DEX] > 0)\n            value += (item->modifiers[OBJ_MOD_DEX] * 200L);\n        if (item->modifiers[OBJ_MOD_CON] > 0)\n            value += (item->modifiers[OBJ_MOD_CON] * 200L);\n'),
        ('        value += (item->modifiers[OBJ_MOD_DAM_RED] * 200L);\n\n        break;\n',
         '        value += (item->modifiers[OBJ_MOD_DAM_RED] * 200L);\n\n        /* Penalties never make an item worth less than nothing */\n        if (value < 0)\n            return (0L);\n\n        break;\n'),
    ],
}
# Per mode: (expected pass before, expected pass after, byte-identical control).
EXPECT = {'statring': (False, True, False), 'speed': (False, True, False), 'nomods': (True, True, True),
          'inertia': (False, True, False), 'thror': (True, True, False)}


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
    for pin in (BASE, PRIOR, fix):
        if subprocess.run(["git", "cat-file", "-e", pin + "^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)
    def checked(pin, changes):
        texts = {}
        for source, hunks in changes.items():
            original = subprocess.check_output(["git", "show", BASE + ":" + source], cwd=repo).decode()
            texts[source] = subprocess.check_output(["git", "show", pin + ":" + source], cwd=repo).decode()
            candidate = original
            for old, new in hunks:
                if candidate.count(old) != 1:
                    raise RuntimeError(f"Expected hunk not found exactly once in {source}")
                candidate = candidate.replace(old, new, 1)
            if candidate != texts[source]:
                raise RuntimeError(f"Source correction in {source} at {pin} is not exactly the expected change")
        if sorted(subprocess.check_output(["git", "diff", "--name-only", BASE, pin], cwd=repo).decode().split()) != sorted(changes):
            raise RuntimeError(f"Source scope changed at {pin}")
        return texts

    fixed = checked(fix, CHANGES)
    prior = checked(PRIOR, PRIOR_CHANGES)

    root = Path(tempfile.mkdtemp(prefix="angband-modcredit-70-", dir="/tmp"))
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
        if phase in SOURCES:
            for path, text in (prior if phase == "prior" else fixed).items():
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
        if args.full_tests and phase != "prior":
            log, code = run(phase + "-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text = re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase] = {"units": "965/965", "frontends": "5/5"}

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
    for mode, row in rows["after"].items():
        if row["state"]["value_a"] < 0 or row["state"]["value_b"] < 0:
            raise RuntimeError(f"{mode}: corrected guess is below zero")
        if row["state"] != rows["prior"][mode]["state"] and mode != "inertia":
            raise RuntimeError(f"{mode}: the floor changed a value that was not below zero")
    if rows["prior"]["inertia"]["state"]["value_a"] >= 0 or rows["after"]["inertia"]["state"]["value_a"] != 0:
        raise RuntimeError("inertia: expected a negative guess without the floor and 0 with it")
    if not rows["prior"]["inertia"]["passed"] or not rows["prior"]["thror"]["passed"]:
        raise RuntimeError("earlier fix head: penalty cases did not sell the expected item")
    if rows["after"]["thror"]["state"]["value_a"] <= rows["before"]["thror"]["state"]["value_a"]:
        raise RuntimeError("thror: the stealth penalty cancelled the bonus credit")
    for path, text in fixed.items():
        if (source / path).read_text() != text:
            raise RuntimeError("Disposable source not restored to the correction")
    summary = {"base": BASE, "prior_fix": PRIOR, "fix": fix, "fixtures_ref": ref, "cases": rows,
               "identical_controls": controls, "full_tests": gates,
               "fixed_source_sha256": {p: hashlib.sha256((source / p).read_bytes()).hexdigest() for p in fixed}}
    (root / "result.json").write_text(json.dumps(summary, indent=2) + "\n")
    subprocess.run([sys.executable, str(evidence / "make_replay.py"), str(evidence)], check=True)
    print('VERIFIED: the stat ring and the speed item are now valued with their bonuses and the Borg sells the plain item first; the no-modifier control is byte-identical; the Amulet of Inertia is floored at 0 (negative without the floor) and kept out of the sale; the Boots of Thror keep their credit; no corrected guess is below zero.', flush=True)
    print(f"Logs and result.json: {root}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}", file=sys.stderr)
        sys.exit(1)
