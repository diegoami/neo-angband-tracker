#!/usr/bin/env python3
"""Compare the first keypress after a Word of Recall level change between the original and the corrected source, in a normal and an AddressSanitizer build.

Run from a clone of diegoami/neo-angband-tracker after fetching
task/iswarded-198-evidence. The default fixtures reference FETCH_HEAD is the
revision fetched by the setup command; the fix reference is required. Use
--full-tests to also run both native unit and frontend regression gates for
the before and after builds.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

BASE = "e473720ed88a0f0c7ca66815cef3000eb4a38bc3"
DIRECTORY = "docs/audit/c-reports/iswarded-198/"
OBSERVER = 'ISW198Observer'
PHASES = ("before", "after")
# Each corrected file must equal the base file with exactly these hunks.
CHANGES = {
    'src/borg/borg-messages.c': [
        ('                        /* Check for glyphs under player -- Cheat*/\n                        if (square_iswarded(cave, borg.c)) {',
         '                        /* Check for glyphs under player -- Cheat.  The\n                         * position may still be on the previous level when\n                         * this is parsed just after a level change. */\n                        if (square_in_bounds(cave, borg.c)\n                            && square_iswarded(cave, borg.c)) {'),
        ('        /* Check for glyphs under player -- Cheat*/\n        if (square_iswarded(cave, borg.c)) {',
         '        /* Check for glyphs under player -- Cheat.  The position may still\n         * be on the previous level when this is parsed just after a level\n         * change. */\n        if (square_in_bounds(cave, borg.c)\n            && square_iswarded(cave, borg.c)) {'),
    ],
}
# AddressSanitizer variant: a second build dir with -DNDEBUG (a release-style
# build) so the debug assertion does not stop the run before the read.
ASAN_FLAGS = "-fsanitize=address -fno-omit-frame-pointer -g -DNDEBUG"
# The pristine base trips unrelated strcmp-interceptor reports in the Borg's
# own string sort (borg-util.c); they are not part of this defect.
ASAN_OPTIONS = "detect_leaks=0:intercept_strcmp=0"
# Out-of-level cases: the expected read site in the original.
ASAN_SITES = {"rune-town": "square_istrap <- borg_parse_aux borg-messages.c:1188",
              "hit-town": "square_istrap <- borg_parse_aux borg-messages.c:625"}
# Per mode: (expected pass before, expected pass after, byte-identical control).
EXPECT = {'rune-town': (False, True, False), 'hit-town': (False, True, False), 'rune-inside': (True, True, True), 'hit-inside': (True, True, True), 'hit-noglyph': (True, True, True)}


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

    root = Path(tempfile.mkdtemp(prefix="angband-iswarded-198-", dir="/tmp"))
    source, build, evidence = root / "source", root / "build", root / "evidence"
    build_asan = root / "build-asan"
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
    log, code = run("configure-asan", ["cmake", "-S", str(source), "-B", str(build_asan), "-DSUPPORT_BORG=ON",
                      "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug",
                      "-DCMAKE_C_FLAGS=" + ASAN_FLAGS, "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address",
                      *args.cmake_arg])
    if code:
        print(log.read_text()[-4000:], file=sys.stderr)
        raise RuntimeError("asan configure failed")
    rows = {}
    asan_rows = {}
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
        log, code = run("asan-" + phase + "-build", ["cmake", "--build", str(build_asan), "--parallel", "2"])
        if code:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"asan {phase} build failed")
        os.environ["ASAN_OPTIONS"] = ASAN_OPTIONS
        log, code = run("asan-" + phase + "-scenarios", [sys.executable, str(evidence / "run-cases.py"), "asan-" + phase,
                          str(build_asan / "game" / OBSERVER), str(evidence)])
        del os.environ["ASAN_OPTIONS"]
        if code:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"asan {phase} scenario runner failed")
        asan_rows[phase] = json.loads((evidence / f"asan-{phase}-results.json").read_text())
        if args.full_tests:
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
    for mode, (want_before, want_after, control) in EXPECT.items():
        before, after = rows["before"][mode], rows["after"][mode]
        if not want_before and not before["assert_abort"]:
            raise RuntimeError(f"{mode}: original did not stop on the square_istrap bounds assertion")
        if before["passed"] and before["assert_abort"]:
            raise RuntimeError(f"{mode}: unexpected assertion in a passing original case")
        if after["assert_abort"]:
            raise RuntimeError(f"{mode}: corrected run hit the bounds assertion")
        abefore, aafter = asan_rows["before"][mode], asan_rows["after"][mode]
        if mode in ASAN_SITES:
            if abefore["passed"] or not abefore["asan_read"] or abefore["asan_site"] != ASAN_SITES[mode]:
                raise RuntimeError(f"{mode}: expected AddressSanitizer read at {ASAN_SITES[mode]}, got {abefore.get('asan_site')}")
        elif not abefore["passed"] or abefore["asan"]:
            raise RuntimeError(f"{mode}: original AddressSanitizer control changed")
        if not aafter["passed"] or aafter["asan"]:
            raise RuntimeError(f"{mode}: corrected AddressSanitizer case failed")
        if control and (evidence / f"asan-before-{mode}.log").read_bytes() != (evidence / f"asan-after-{mode}.log").read_bytes():
            raise RuntimeError(f"{mode}: AddressSanitizer control logs are not byte-identical")
    for path, text in fixed.items():
        if (source / path).read_text() != text:
            raise RuntimeError("Disposable source not restored to the correction")
    summary = {"base": BASE, "fix": fix, "fixtures_ref": ref, "cases": rows,
               "identical_controls": controls, "full_tests": gates,
               "asan": {"flags": ASAN_FLAGS, "options": ASAN_OPTIONS, "cases": asan_rows,
                        "expected_sites": ASAN_SITES},
               "fixed_source_sha256": {p: hashlib.sha256((source / p).read_bytes()).hexdigest() for p in fixed}}
    (root / "result.json").write_text(json.dumps(summary, indent=2) + "\n")
    subprocess.run([sys.executable, str(evidence / "make_replay.py"), str(evidence)], check=True)
    print('VERIFIED: original aborts at both rune checks after recall (assertion; AddressSanitizer heap-buffer-overflow read with -DNDEBUG); corrected completes the first town keypress; three controls byte-identical.', flush=True)
    print(f"Logs and result.json: {root}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}", file=sys.stderr)
        sys.exit(1)
