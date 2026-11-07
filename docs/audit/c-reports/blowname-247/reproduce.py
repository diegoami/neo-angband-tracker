#!/usr/bin/env python3
"""Compare the blow-effect table lookup between the original and the sentinel-safe correction."""
import argparse
import hashlib
import os
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

BASE = "e473720ed88a0f0c7ca66815cef3000eb4a38bc3"
FIX = "541e03b1e6e0bed42e0b71ec520cb6ced8f29d65"
DIRECTORY = "docs/audit/c-reports/blowname-247/"
SOURCE = "src/borg/borg-fight-attack.c"
PHASES = ("before", "after")
OLD_LOOP = (
    "    unsigned long i;\n"
    "    for (i = 0; i < sizeof(monblow) / sizeof(monblow[0]); ++i)\n"
    "        if (!strcmp(name, monblow[i].name))\n"
    "            return monblow[i].val;\n"
    "    return MONBLOW_NONE;"
)
NEW_LOOP = (
    "    unsigned long i;\n"
    "    for (i = 0; i + 1 < sizeof(monblow) / sizeof(monblow[0]); ++i)\n"
    "        if (!strcmp(name, monblow[i].name))\n"
    "            return monblow[i].val;\n"
    "    return MONBLOW_NONE;"
)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures-ref", default="FETCH_HEAD")
    parser.add_argument("--full-tests", action="store_true")
    parser.add_argument("--cmake-arg", action="append", default=[])
    args = parser.parse_args()
    for command in ("git", "cmake", "flock", "cc", "make"):
        if not shutil.which(command): raise RuntimeError(f"Missing prerequisite: {command}")
    repo = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
    ref = subprocess.check_output(["git", "rev-parse", args.fixtures_ref], cwd=repo, text=True).strip()
    def read(path): return subprocess.check_output(["git", "show", ref + ":" + path], cwd=repo)
    payload = {name: read(DIRECTORY + name) for name in ("scenario.c", "run-cases.py", "make_replay.py", "borg.txt")}
    for pin in (BASE, FIX):
        if subprocess.run(["git", "cat-file", "-e", pin+"^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)
    original = subprocess.check_output(["git", "show", BASE+":"+SOURCE], cwd=repo).decode()
    fixed = subprocess.check_output(["git", "show", FIX+":"+SOURCE], cwd=repo).decode()
    if original.count(OLD_LOOP) != 1:
        raise RuntimeError("Expected original sentinel-visiting lookup loop missing")
    expected = original.replace(OLD_LOOP, NEW_LOOP, 1)
    if expected != fixed:
        raise RuntimeError("Source correction is not exactly the sentinel-safe loop bound")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source scope changed")
    variants={"before":original,"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-blowname-247-", dir="/tmp"))
    source, build, build_asan, evidence = root/"source", root/"build", root/"build-asan", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(BW247Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(BW247Observer PRIVATE src src/tests)
target_compile_definitions(BW247Observer PRIVATE ALLOW_BORG)
target_link_libraries(BW247Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{root/'test-user'}")
''')
    test_script=source/"tests/run-test"
    test_script.write_text(test_script.read_text().replace("~/.angband/Test", str(root/"test-user/Test")))
    def run(label, command):
        log=root/(label+".log"); print(f"{label}: shared validation lock; log {log}", flush=True)
        env=None
        if "build-asan" in " ".join(map(str, command)):
            env=dict(os.environ, ASAN_OPTIONS="halt_on_error=0:detect_leaks=0")
        with log.open("w") as output:
            result=subprocess.run(["flock", "/tmp/angband-validation.lock", *command], cwd=repo,
                                  stdout=output, stderr=subprocess.STDOUT, env=env)
        return log, result.returncode
    log, code = run("configure", ["cmake", "-S", str(source), "-B", str(build), "-DSUPPORT_BORG=ON",
                      "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug", *args.cmake_arg])
    if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError("configure failed")
    log, code = run("configure-asan", ["cmake", "-S", str(source), "-B", str(build_asan),
                      "-DSUPPORT_BORG=ON", "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug",
                      "-DCMAKE_C_FLAGS=-fsanitize=address -fsanitize-recover=address -g",
                      "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address", *args.cmake_arg])
    if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError("asan configure failed")
    rows={}; gates={}
    for phase in PHASES:
        (source/SOURCE).write_text(variants[phase])
        log, code = run(phase+"-build", ["cmake", "--build", str(build), "--parallel", "2"])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} build failed")
        log, code = run(phase+"-build-asan", ["cmake", "--build", str(build_asan), "--parallel", "2"])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} asan build failed")
        env_for_asan = dict(os.environ, ASAN_OPTIONS="halt_on_error=0:detect_leaks=0")
        log, code = run(phase+"-scenarios", [sys.executable, str(evidence/"run-cases.py"), phase,
                               str(build/"game/BW247Observer"), str(build_asan/"game/BW247Observer"),
                               str(evidence)])
        os.environ.clear(); os.environ.update(env_for_asan)
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} scenario runner failed")
        rows[phase]=json.loads((evidence/f"{phase}-results.json").read_text())
        if args.full_tests:
            log, code = run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    if rows["before"]["main"]["passed"] or rows["before"]["main"]["signal"]!="segfault":
        raise RuntimeError("Original does not segfault on the unknown blow name")
    if "AddressSanitizer" not in (evidence/"before-asan-main.log").read_text():
        raise RuntimeError("Original asan run lacks the sanitizer crash report")
    if not rows["after"]["main"]["passed"] or rows["after"]["main"]["signal"]!="clean":
        raise RuntimeError("Corrected variant still crashes on the unknown blow name")
    if not rows["after"]["asan"]["passed"]:
        raise RuntimeError("Corrected asan variant failed")
    if "borg_sort_comp_hook" not in (evidence/"before-asan-main.log").read_text():
        raise RuntimeError("Expected the documented unrelated init-time sort-hook report in the before asan log")
    if (source/SOURCE).read_text()!=fixed:
        raise RuntimeError("Disposable source not restored to the correction")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,"cases":rows,
             "identical_full_logs":[],"full_tests":gates,
             "fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: the original segfaults on an unknown blow name (strcmp against the NULL end entry), the correction returns NONE; asan report kept for the original.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
