#!/usr/bin/env python3
"""Compare the screen-text conversion: the original, the earlier character-count bound, and the buffer-size bound."""
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
# Earlier fix shape (ABS(n) * MB_CUR_MAX), measured only to show the overflow it allowed.
INTERIM = "3e7fe8a8399352c4fe67f5de2d8d48b5e0bd04c1"
FIX = "e7f670fa4d4f0672dc346637fc7be20c814a4b62"
DIRECTORY = "docs/audit/c-reports/text-287/"
IO = "src/borg/borg-io.c"
SOURCES = (IO, "src/borg/borg-io.h", "src/borg/borg-messages.c", "src/borg/borg-think.c")
PHASES = ("before", "interim", "after")
GATED = ("before", "after")
CONTROLS = ("control",)

# The exact source change, file by file (original text -> corrected text).
HUNKS = {
 IO: [
  ("""errr borg_what_text(int x, int y, int n, uint8_t *a, char *s)
{
    int     i;
""", """errr borg_what_text(int x, int y, int n, uint8_t *a, char *s, size_t size)
{
    int     i;
    size_t  len;
"""),
  (""" * Note that "a" points to a single "attr", and "s" to an array
 * of "chars", into which the attribute and text at the given
 * location are stored.
""", """ * Note that "a" points to a single "attr", and "s" to an array
 * of "size" "chars", into which the attribute and text at the given
 * location are stored.  The text is truncated to fit and always
 * terminated.
"""),
  ("""    /* Convert back to a char string */
    wcstombs(s, screen_str, ABS(n) + 1);
""", """    /* Convert back to a char string that fits the buffer; wcstombs()
     * does not terminate a result it had to truncate */
    len = wcstombs(s, screen_str, size - 1);
    s[(len == (size_t)-1) ? 0 : len] = '\\0';
""")],
 "src/borg/borg-io.h": [
  ("extern errr borg_what_text(int x, int y, int n, uint8_t *a, char *s);\n",
   "extern errr borg_what_text(\n    int x, int y, int n, uint8_t *a, char *s, size_t size);\n")],
 "src/borg/borg-messages.c": [
  ("    borg_what_text(0, 0, ((Term->wid - 1) / (tile_width)), &t_a, buffer);\n",
   "    borg_what_text(\n        0, 0, ((Term->wid - 1) / (tile_width)), &t_a, buffer, sizeof(buffer));\n"),
  ("        && (0 == borg_what_text(cursor.x - 7, cursor.y, 7, &t_a, buffer))\n",
   "        && (0 == borg_what_text(\n            cursor.x - 7, cursor.y, 7, &t_a, buffer, sizeof(buffer)))\n"),
  ("        if (0 == borg_what_text(0, 0, cursor.x - 7, &t_a, buffer)) {\n",
   "        if (0 == borg_what_text(\n                0, 0, cursor.x - 7, &t_a, buffer, sizeof(buffer))) {\n"),
  ("                0, 0, ((Term->wid - 1) / (tile_width)), &t_a, buffer)) {\n",
   "                0, 0, ((Term->wid - 1) / (tile_width)), &t_a, buffer,\n                sizeof(buffer))) {\n")],
 "src/borg/borg-think.c": [
  ("    if ((0 == borg_what_text(1, 3, 4, &t_a, buf))\n",
   "    if ((0 == borg_what_text(1, 3, 4, &t_a, buf, sizeof(buf)))\n")],
}
INTERIM_HUNK = ("""    /* Convert back to a char string */
    wcstombs(s, screen_str, ABS(n) + 1);""", """    /* Convert back to a char string.  Each wide character can become
     * several bytes, so a byte limit of one per wide character can
     * truncate exactly at the limit and leave the destination
     * unterminated; bound the conversion by the widest encoding
     * instead, and fall back to an empty string on a conversion
     * error. */
    if (wcstombs(s, screen_str, ABS(n) * MB_CUR_MAX) == (size_t)-1)
        s[0] = '\\0';""")

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
    for pin in (BASE, INTERIM, FIX):
        if subprocess.run(["git", "cat-file", "-e", pin+"^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)
    def show(pin, path): return subprocess.check_output(["git", "show", pin+":"+path], cwd=repo).decode()
    def names(a, b): return sorted(subprocess.check_output(["git", "diff", "--name-only", a, b], cwd=repo).decode().split())
    variants = {phase: {} for phase in PHASES}
    for path in SOURCES:
        original = show(BASE, path); expected = original
        for old, new in HUNKS[path]:
            if expected.count(old) != 1: raise RuntimeError(f"Expected original text missing in {path}: {old!r}")
            expected = expected.replace(old, new, 1)
        if expected != show(FIX, path):
            raise RuntimeError(f"Source correction in {path} is not exactly the buffer-size bound")
        variants["before"][path] = original; variants["after"][path] = expected
        variants["interim"][path] = show(INTERIM, path)
    if names(BASE, FIX) != sorted(SOURCES): raise RuntimeError("Source scope changed")
    if names(BASE, INTERIM) != [IO] or show(BASE, IO).replace(*INTERIM_HUNK, 1) != show(INTERIM, IO):
        raise RuntimeError("Earlier fix shape is not exactly the character-count bound")
    sized = {"before": 0, "interim": 0, "after": 1}
    root = Path(tempfile.mkdtemp(prefix="angband-text-287-", dir="/tmp"))
    source, build, build_asan, evidence = root/"source", root/"build", root/"build-asan", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(T287Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(T287Observer PRIVATE src src/tests)
target_compile_definitions(T287Observer PRIVATE ALLOW_BORG)
target_link_libraries(T287Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
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
    def check(log, code, what):
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(what+" failed")
    check(*run("configure", ["cmake", "-S", str(source), "-B", str(build), "-DSUPPORT_BORG=ON",
                      "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug", *args.cmake_arg]), "configure")
    check(*run("configure-asan", ["cmake", "-S", str(source), "-B", str(build_asan),
                      "-DSUPPORT_BORG=ON", "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug",
                      "-DCMAKE_C_FLAGS=-fsanitize=address -g",
                      "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address", *args.cmake_arg]), "asan configure")
    rows={}; gates={}
    for phase in PHASES:
        for path, text in variants[phase].items(): (source/path).write_text(text)
        (evidence/"t287-variant.h").write_text(f"#define T287_SIZED {sized[phase]}\n")
        check(*run(phase+"-build", ["cmake", "--build", str(build), "--parallel", "2"]), phase+" build")
        check(*run(phase+"-build-asan", ["cmake", "--build", str(build_asan), "--target", "T287Observer",
                                         "--parallel", "2"]), phase+" asan build")
        check(*run(phase+"-scenarios", [sys.executable, str(evidence/"run-cases.py"), phase,
                   str(build/"game/T287Observer"), str(build_asan/"game/T287Observer"), str(evidence)]),
              phase+" scenario runner")
        rows[phase]=json.loads((evidence/f"{phase}-results.json").read_text())
        if args.full_tests and phase in GATED:
            log, code = run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    def expect(phase, mode, passed, rating, asan=None):
        row=rows[phase][mode]
        if rating is not None and (row["passed"]!=passed or row.get("rating")!=rating):
            raise RuntimeError(f"{phase} {mode}: expected passed={passed} '{rating}', got {row}")
        a=rows[phase]["asan"][mode]
        if asan is None:
            if a["sanitizer_report"] or a["passed"]!=passed or a.get("rating")!=rating:
                raise RuntimeError(f"{phase} asan {mode}: expected no report and the same outcome, got {a}")
        elif (a["sanitizer_report"], a["access"])!=asan or "wcstombs" not in a["frames"] or "borg_what_text" not in a["frames"]:
            raise RuntimeError(f"{phase} asan {mode}: expected {asan} in wcstombs from borg_what_text, got {a}")
    expect("before", "multibyte", False, "terminated=0 strnlen=1024")
    expect("interim", "multibyte", True, "terminated=1 strnlen=81")
    expect("after", "multibyte", True, "terminated=1 strnlen=81")
    for phase in PHASES: expect(phase, "control", True, "terminated=1 strnlen=80")
    expect("before", "wide", False, "terminated=0 strnlen=1024 guard_overwritten=0")
    if rows["interim"]["wide"]["passed"] or rows["interim"]["wide"].get("rating")!="terminated=0 strnlen=1024 guard_overwritten=177":
        raise RuntimeError(f"Earlier fix shape does not show the overflow: {rows['interim']['wide']}")
    expect("interim", "wide", False, None, ("stack-buffer-overflow", "WRITE of size 1201"))
    expect("after", "wide", True, "terminated=1 strnlen=1020 guard_overwritten=0")
    for mode in CONTROLS:
        if (evidence/f"before-{mode}.log").read_bytes()!=(evidence/f"after-{mode}.log").read_bytes():
            raise RuntimeError("Changed control log: "+mode)
    for path in SOURCES:
        if (source/path).read_text()!=variants["after"][path]:
            raise RuntimeError("Disposable source not restored to the correction")
    summary={"base":BASE,"interim":INTERIM,"fix":FIX,"fixtures_ref":ref,"cases":rows,
             "identical_full_logs":list(CONTROLS),"full_tests":gates,
             "fixed_source_sha256":{p:hashlib.sha256((source/p).read_bytes()).hexdigest() for p in SOURCES}}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: multibyte - the original leaves the text unterminated (1024 stale bytes), the correction terminates at 81; "
          "wide (301 columns, 4-byte text) - the original is unterminated without overflow, the earlier character-count bound "
          "writes 177 bytes past the 1024-byte buffer (AddressSanitizer: stack-buffer-overflow, WRITE of size 1201), "
          "the buffer-size bound terminates at 1020 with nothing past the buffer; control identical; no sanitizer report "
          "for the original or the correction.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduce failed: {error}",file=sys.stderr);sys.exit(1)
