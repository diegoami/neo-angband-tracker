#!/usr/bin/env python3
"""Compare the clock-overflow handling between the original and the corrected source.

Run from the repository after fetching task/clockreset-246-evidence. The default
fixtures reference FETCH_HEAD is the revision fetched by the setup command; the
fix reference is required. Use --full-tests to also run both native unit and
frontend regression gates for the before and after builds.
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
DIRECTORY = "docs/audit/c-reports/clockreset-246/"
SOURCE = "src/borg/borg-think-dungeon.c"
CONTROLS = ('below-threshold',)  # unchanged-situation controls: complete logs must be byte-identical
DIAGNOSTICS = ()  # recorded, not asserted
PHASES = ("before", "after")

ABORT_OLD = '''    /* Prevent clock wrapping Step 2 */
    if (borg.time.now >= BORG_TIME_MAX - 3000) {
        /* Panic */
        borg_oops("clock overflow");

        /* Oops */
        return true;
    }'''
ABORT_NEW = '''    /* Prevent clock wrapping Step 2: reset the clock gracefully and keep
     * playing.  This must run before any panic: the borg_think() overflow
     * check remains as a failsafe in case the reset was somehow missed.
     * The previous version aborted the borg here, which fired about 2900
     * turns earlier than the reset threshold and made the graceful reset
     * unreachable. */
    if (borg.time.now >= BORG_TIME_MAX - 3000) {
        borg.time.now = 1;
        borg_warning("*****WARNING***** Borg Clock Overflow");
    }'''


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
    original = subprocess.check_output(["git", "show", BASE + ":" + SOURCE], cwd=repo).decode()
    fixed = subprocess.check_output(["git", "show", fix + ":" + SOURCE], cwd=repo).decode()
    candidate = original.replace(ABORT_OLD, ABORT_NEW, 1)
    if candidate != fixed:
        raise RuntimeError("Source correction is not exactly the graceful clock reset")
    if subprocess.check_output(["git", "diff", "--name-only", BASE, fix], cwd=repo).decode().split() != [SOURCE]:
        raise RuntimeError("Source scope changed")

    root = Path(tempfile.mkdtemp(prefix="angband-clockreset-246-", dir="/tmp"))
    source, build, evidence = root / "source", root / "build", root / "evidence"
    (evidence / "profile").mkdir(parents=True)
    for name, data in payload.items():
        (evidence / name).write_bytes(data)
    (evidence / "profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source / "CMakeLists.txt"
    cmake.write_text(cmake.read_text() + f'''
add_executable(CR246Observer "{evidence / 'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(CR246Observer PRIVATE src src/tests)
target_compile_definitions(CR246Observer PRIVATE ALLOW_BORG)
target_link_libraries(CR246Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{root / 'test-user'}")
''')
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
            (source / SOURCE).write_text(fixed)
        log, code = run(phase + "-build", ["cmake", "--build", str(build), "--parallel", "2"])
        if code:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"{phase} build failed")
        log, code = run(phase + "-scenarios", [sys.executable, str(evidence / "run-cases.py"), phase,
                          str(build / "game/CR246Observer"), str(evidence)])
        if code:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"{phase} scenario runner failed")
        rows[phase] = json.loads((evidence / f"{phase}-results.json").read_text())
        if args.full_tests:
            log, code = run(phase + "-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text = re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase] = {"units": "965/965", "frontends": "5/5"}

    before, after = rows["before"]["main"], rows["after"]["main"]
    if before["passed"] or before["state"].get("active") != 0 or before["state"].get("time_after") != 2147480647:
        raise RuntimeError("Original did not abort at the threshold: the fixture no longer demonstrates the defect")
    if not after["passed"]:
        raise RuntimeError("Corrected variant did not pass")
    if after["state"].get("active") != 1 or after["state"].get("time_after") != 1:
        raise RuntimeError("Corrected variant must reset the clock and keep the borg active")
    if (source / SOURCE).read_text() != fixed:
        raise RuntimeError("Disposable source not restored to the correction")
    for control in CONTROLS:
        if not (rows["before"][control]["passed"] and rows["after"][control]["passed"]):
            raise RuntimeError(f"Control {control} did not pass in both versions")
        if (evidence / f"before-{control}.log").read_bytes() != (evidence / f"after-{control}.log").read_bytes():
            raise RuntimeError(f"Control {control} logs differ between the original and the correction")
    summary = {"base": BASE, "fix": fix, "fixtures_ref": ref, "cases": rows, "identical_full_logs": list(CONTROLS), "diagnostics": list(DIAGNOSTICS),
               "full_tests": gates,
               "fixed_source_sha256": {SOURCE: hashlib.sha256((source / SOURCE).read_bytes()).hexdigest()}}
    (root / "result.json").write_text(json.dumps(summary, indent=2) + "\n")
    subprocess.run([sys.executable, str(evidence / "make_replay.py"), str(evidence)], check=True)
    print("VERIFIED: at the overflow threshold the original aborts the borg with a frozen clock; the correction resets the clock and keeps playing.", flush=True)
    print(f"Logs and result.json: {root}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}", file=sys.stderr)
        sys.exit(1)
