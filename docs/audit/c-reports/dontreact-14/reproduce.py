#!/usr/bin/env python3
"""Compare the knockout reaction gating between the original static flag and the rename to borg.dont_react."""
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
FIX = None  # set below after reading the fix commit from the task branch ref
DIRECTORY = "docs/audit/c-reports/dontreact-14/"
SOURCES = (
    "src/borg/borg-messages-react.c",
    "src/borg/borg-messages-react.h",
    "src/borg/borg-messages.c",
    "src/borg/borg-caution.c",
)
PHASES = ("before", "after")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures-ref", default="FETCH_HEAD")
    parser.add_argument("--fix-ref", required=True)
    parser.add_argument("--full-tests", action="store_true")
    parser.add_argument("--cmake-arg", action="append", default=[])
    args = parser.parse_args()
    for command in ("git", "cmake", "flock", "cc", "make"):
        if not shutil.which(command): raise RuntimeError(f"Required command not found: {command}")
    repo = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
    ref = subprocess.check_output(["git", "rev-parse", args.fixtures_ref], cwd=repo, text=True).strip()
    fix = subprocess.check_output(["git", "rev-parse", args.fix_ref], cwd=repo, text=True).strip()
    def read(path): return subprocess.check_output(["git", "show", ref + ":" + path], cwd=repo)
    payload = {name: read(DIRECTORY + name) for name in ("scenario.c", "run-cases.py", "make_replay.py", "borg.txt")}
    for pin in (BASE, fix):
        if subprocess.run(["git", "cat-file", "-e", pin+"^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)
    variants = {}
    for source in SOURCES:
        original = subprocess.check_output(["git", "show", BASE+":"+source], cwd=repo).decode()
        fixed = subprocess.check_output(["git", "show", fix+":"+source], cwd=repo).decode()
        variants.setdefault("before", {})[source] = original
        variants.setdefault("after", {})[source] = fixed
    # Verify each file's delta is exactly the expected rename.
    renames = (
        ("src/borg/borg-messages-react.c",
         [("bool borg_dont_react = false;\n", ""),
          ("if (borg_dont_react || borg.trait[BI_ISPARALYZED]) {", "if (borg.dont_react || borg.trait[BI_ISPARALYZED]) {")]),
        ("src/borg/borg-messages-react.h", [("extern bool borg_dont_react;\n", "")]),
        ("src/borg/borg-messages.c",
         [("borg_dont_react = true;", "borg.dont_react = true;")]),
        ("src/borg/borg-caution.c", [("if (borg_dont_react) {", "if (borg.dont_react) {")]),
    )
    for source, edits in renames:
        original = subprocess.check_output(["git", "show", BASE+":"+source], cwd=repo).decode()
        candidate = original
        for old, new in edits:
            if candidate.count(old) < 1: raise RuntimeError(f"Expected edit text missing in {source}")
            candidate = candidate.replace(old, new)
        fixed_file = subprocess.check_output(["git", "show", fix+":"+source], cwd=repo).decode()
        if candidate != fixed_file:
            raise RuntimeError(f"Source correction for {source} is not exactly the rename")
    if subprocess.check_output(["git","diff","--name-only",BASE,fix],cwd=repo).decode().split() != sorted(SOURCES):
        raise RuntimeError("Source scope changed")
    root = Path(tempfile.mkdtemp(prefix="angband-dontreact-14-", dir="/tmp"))
    source, build, evidence = root/"source", root/"build", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(KO14Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(KO14Observer PRIVATE src src/tests)
target_compile_definitions(KO14Observer PRIVATE ALLOW_BORG)
target_link_libraries(KO14Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{root/'test-user'}")
''')
    test_script=source/"tests/run-test"
    test_script.write_text(test_script.read_text().replace("~/.angband/Test", str(root/"test-user/Test")))
    def run(label, command):
        log=root/(label+".log"); print(f"{label}: shared validation lock; log {log}", flush=True)
        with log.open("w") as output:
            result=subprocess.run(["flock", "/tmp/angband-validation.lock", *command], cwd=repo,
                                  stdout=output, stderr=subprocess.STDOUT)
        return log, result.returncode
    log, code = run("configure", ["cmake", "-S", str(source), "-B", str(build), "-DSUPPORT_BORG=ON",
                      "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug", *args.cmake_arg])
    if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError("configure failed")
    rows={}; gates={}
    for phase in PHASES:
        for source_path, content in variants[phase].items():
            (source/source_path).write_text(content)
        log, code = run(phase+"-build", ["cmake", "--build", str(build), "--parallel", "2"])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} build failed")
        log, code = run(phase+"-scenarios", [sys.executable, str(evidence/"run-cases.py"), phase,
                               str(build/"game/KO14Observer"), str(evidence)])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} scenario runner failed")
        rows[phase]=json.loads((evidence/f"{phase}-results.json").read_text())
        if args.full_tests:
            log, code = run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    if rows["before"]["main"]["passed"]:
        raise RuntimeError("Original knockout case unexpectedly passed: the fixture no longer demonstrates the permanent deafness")
    if rows["after"]["main"]["state"]["after_recovery"] <= rows["after"]["main"]["state"]["after_ko"]:
        raise RuntimeError("Corrected variant does not resume reacting after the pump")
    for source_path, content in variants["after"].items():
        if (source/source_path).read_text() != content:
            raise RuntimeError("Disposable source not restored to the correction")
    summary={"base":BASE,"fix":fix,"fixtures_ref":ref,"cases":rows,
             "identical_full_logs":[],"full_tests":gates,
             "fixed_sources_sha256":{s: hashlib.sha256((source/s).read_bytes()).hexdigest() for s in SOURCES}}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: the original stays deaf after knockout recovery, the corrected borg resumes reacting.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
