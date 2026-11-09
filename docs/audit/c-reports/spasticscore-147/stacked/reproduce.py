#!/usr/bin/env python3
"""Stacked run: which grid the bored Borg's secret-door search walks to, with
the #146 correction alone and with the #146 and #147 corrections together.

Run from a clone of diegoami/neo-angband-tracker after fetching
task/spastic-146, task/spasticscore-147 and task/spasticscore-147-evidence.
Builds three variants of the native C game in one disposable worktree:
the pristine base (reference), base + #146, and base + #146 + #147. Each
source branch must be exactly its expected hunk against the base. Use
--full-tests to also run the native unit and frontend gates for each build.
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
DIRECTORY = "docs/audit/c-reports/spasticscore-147/stacked/"
OBSERVER = 'SP147StackedObserver'
SOURCE = 'src/borg/borg-flow-misc.c'
# Exact hunk of each source branch against the base (the only file it changes).
HUNK_146 = ("    /* Reverse flow */\n    borg_flow_reverse(250, true, false, false, -1, false);\n\n    /* Scan the entire map */\n    for (y = 1;",
            "    /* Reverse flow, not optimized: it starts at the borg's own grid (cost 0)\n     * and an optimized spread stops before reaching any other grid. */\n    borg_flow_reverse(250, false, false, false, -1, false);\n\n    /* Scan the entire map */\n    for (y = 1;")
HUNK_147 = ('            v = (supp * 500) + (diag * 100) - (ag->xtra * 40) - (cost * 2)\n                - borg_timer(borg.time.level);',
            '            v = (supp * 500) + (diag * 100) - (borg_grids[y][x].xtra * 40)\n                - (cost * 2)\n                - borg_timer(borg.time.level);')
PHASES = ("base", "with146", "stacked")
MODES = ("wall", "approach")
# Exact OUTCOME values and scenario pass state per phase and mode.
NONE = dict(first_pick_x=-1, first_pick_y=-1, first_pick_count=-1)
EXPECT = {
    "base": {
        "wall": (False, dict(searches=3, repeat_searches=2, moves=0, end_x=35, end_y=2, **NONE)),
        "approach": (False, dict(searches=0, repeat_searches=0, moves=0, end_x=35, end_y=5, **NONE)),
    },
    "with146": {
        "wall": (False, dict(searches=6, repeat_searches=5, moves=5, first_pick_x=34, first_pick_y=2,
                             first_pick_count=5, end_x=30, end_y=2)),
        "approach": (True, dict(searches=1, repeat_searches=0, moves=3, end_x=38, end_y=2, **NONE)),
    },
    "stacked": {
        "wall": (True, dict(searches=4, repeat_searches=0, moves=7, first_pick_x=37, first_pick_y=2,
                            first_pick_count=0, end_x=42, end_y=2)),
        "approach": (True, dict(searches=1, repeat_searches=0, moves=3, end_x=38, end_y=2, **NONE)),
    },
}
# Byte-identical between #146 only and #146 + #147.
CONTROLS = ("approach",)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures-ref", default="FETCH_HEAD")
    parser.add_argument("--fix146-ref", required=True)
    parser.add_argument("--fix147-ref", required=True)
    parser.add_argument("--full-tests", action="store_true")
    parser.add_argument("--cmake-arg", action="append", default=[])
    args = parser.parse_args()
    for command in ("git", "cmake", "flock", "cc", "make"):
        if not shutil.which(command):
            raise RuntimeError(f"Required command not found: {command}")
    repo = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())

    def rev(name):
        return subprocess.check_output(["git", "rev-parse", name], cwd=repo, text=True).strip()

    ref, fix146, fix147 = rev(args.fixtures_ref), rev(args.fix146_ref), rev(args.fix147_ref)

    def read(path):
        return subprocess.check_output(["git", "show", ref + ":" + path], cwd=repo)

    payload = {name: read(DIRECTORY + name) for name in ("scenario.c", "run-cases.py", "make_replay.py", "borg.txt")}
    for pin in (BASE, fix146, fix147):
        if subprocess.run(["git", "cat-file", "-e", pin + "^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)

    def blob(commit):
        return subprocess.check_output(["git", "show", commit + ":" + SOURCE], cwd=repo).decode()

    def apply(text, hunk):
        old, new = hunk
        if text.count(old) != 1:
            raise RuntimeError("Expected hunk not found exactly once")
        return text.replace(old, new, 1)

    original = blob(BASE)
    for name, commit, hunk in (("#146", fix146, HUNK_146), ("#147", fix147, HUNK_147)):
        if apply(original, hunk) != blob(commit):
            raise RuntimeError(f"{name} source branch is not exactly its expected change")
        if subprocess.check_output(["git", "diff", "--name-only", BASE, commit], cwd=repo).decode().split() != [SOURCE]:
            raise RuntimeError(f"{name} source scope changed")
    variants = {"base": original, "with146": apply(original, HUNK_146),
                "stacked": apply(apply(original, HUNK_146), HUNK_147)}

    root = Path(tempfile.mkdtemp(prefix="angband-spasticscore-147-stacked-", dir="/tmp"))
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
    rows, gates = {}, {}
    for phase in PHASES:
        (source / SOURCE).write_text(variants[phase])
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
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text:
                raise RuntimeError(f"{phase}: expected native regression totals missing or frontend failure")
            gates[phase] = {"units": "965/965", "frontends": "5/5"}

    for phase in PHASES:
        for mode in MODES:
            want_pass, want = EXPECT[phase][mode]
            row = rows[phase][mode]
            if row["passed"] != want_pass:
                raise RuntimeError(f"{phase}/{mode}: expected passed={want_pass}, got {row['passed']}")
            if row.get("outcome") != want:
                raise RuntimeError(f"{phase}/{mode}: expected {want}, got {row.get('outcome')}")
    for mode in CONTROLS:
        if (evidence / f"with146-{mode}.log").read_bytes() != (evidence / f"stacked-{mode}.log").read_bytes():
            raise RuntimeError(f"{mode}: control logs are not byte-identical")
    if (source / SOURCE).read_text() != variants["stacked"]:
        raise RuntimeError("Disposable source not left at the stacked variant")
    summary = {"base": BASE, "fix146": fix146, "fix147": fix147, "fixtures_ref": ref, "cases": rows,
               "identical_controls": list(CONTROLS), "full_tests": gates,
               "variant_sha256": {p: hashlib.sha256(t.encode()).hexdigest() for p, t in variants.items()}}
    (root / "result.json").write_text(json.dumps(summary, indent=2) + "\n")
    subprocess.run([sys.executable, str(evidence / "make_replay.py"), str(evidence)], check=True)
    print('VERIFIED: with #146 alone the bored Borg walks onto a grid its first search already covered and searches '
          'five covered grids again; with #146 and #147 it walks to an uncovered grid and repeats none; the control '
          'is byte-identical.', flush=True)
    print(f"Logs and result.json: {root}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}", file=sys.stderr)
        sys.exit(1)
