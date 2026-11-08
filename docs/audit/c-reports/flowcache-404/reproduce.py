#!/usr/bin/env python3
"""Reproduce the flow danger-cache fault (flowcache-404) and check its correction.

Builds the native C game five ways from the pinned C base, runs the same
30 native cases against each, and asserts the expected outcome matrix:

  before            pristine base                                  (fault)
  after             the proposed correction (--fix-ref)            (fixed)
  neg-spread-only   correction in borg_flow_spread() only          (negative control)
  neg-enqueue-only  correction in borg_flow_enqueue_grid() only    (negative control)
  alt-wipe          alternative: bypass flows force a cache wipe   (comparison, not proposed)

The neg-* and alt-* sources are derived from the base by the exact text
edits below; their digests are recorded in result.json.
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
SOURCE = "src/borg/borg-flow.c"
DIRECTORY = "docs/audit/c-reports/flowcache-404/"
PAYLOAD = ("scenario.c", "run-cases.py", "make_replay.py", "borg.txt")
HELPER_START = "/*\n * Do the flows of the current mode skip the danger check?"
CLEAR_DOC = '/*\n * Clear the "flow" information\n */'
VARIANTS = ("before", "after", "neg-spread-only", "neg-enqueue-only", "alt-wipe")

MODES = ("desperate", "lunal", "munchkin", "digging")
CONTRACT = ({f"cache-{m}-{e}" for m in MODES for e in ("spread", "enqueue")}
            | {"caution", "prod-visible", "prod-remembered"})
EXPECTED_FAIL = {
    "before": CONTRACT,
    "after": set(),
    "neg-spread-only": {f"cache-{m}-enqueue" for m in MODES},
    "neg-enqueue-only": {f"cache-{m}-spread" for m in MODES} | {"caution", "prod-visible", "prod-remembered"},
    "alt-wipe": set(),
}


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError(f"expected exactly one occurrence of: {old[:60]!r}")
    return text.replace(old, new)


def derive(base, fixed):
    """Return {variant: source text} for the five variants."""
    start = fixed.index(HELPER_START)
    helper = fixed[start:fixed.index(CLEAR_DOC)]
    with_helper = replace_once(base, CLEAR_DOC, helper + CLEAR_DOC)

    spread_old = ('                /* Mark as known */\n'
                  '                borg_data_know->data[y][x] = true;\n\n'
                  '                if (!borg.status.desperate && !borg.lunal_mode && !borg.munchkin_mode\n'
                  '                    && !borg.status.digging) {\n'
                  '                    /* Get the danger */\n')
    spread_new = ('                if (!borg_flow_ignores_danger()) {\n'
                  '                    /* Mark as known */\n'
                  '                    borg_data_know->data[y][x] = true;\n\n'
                  '                    /* Get the danger */\n')
    enqueue_old = ('    /* Unknown */\n    if (!borg_data_know->data[y][x]) {\n        /* Mark as known */')
    enqueue_new = ('    /* Unknown */\n    if (!borg_data_know->data[y][x] && !borg_flow_ignores_danger()) {\n'
                   '        /* Mark as known */')
    gate_old = ('        if ((p > fear) && !borg.status.desperate && !borg.lunal_mode\n'
                '            && !borg.munchkin_mode && !borg.status.digging) {')
    gate_new = '        if (p > fear) {'

    spread_only = replace_once(with_helper, spread_old, spread_new)
    enqueue_only = replace_once(replace_once(with_helper, enqueue_old, enqueue_new), gate_old, gate_new)

    bypass = ('borg.status.desperate || borg.lunal_mode || borg.munchkin_mode\n'
              '                    || borg.status.digging')
    alt = replace_once(base, '                /* Mark as known */\n                borg_data_know->data[y][x] = true;\n\n'
                             '                if (!borg.status.desperate',
                       '                /* Mark as known */\n                borg_data_know->data[y][x] = true;\n\n'
                       f'                if ({bypass})\n                    borg_danger_wipe = true;\n\n'
                       '                if (!borg.status.desperate')
    alt = replace_once(alt, '        /* Dangerous grid */\n        if ((p > fear) && !borg.status.desperate',
                       f'        if ({bypass.replace(chr(10) + "                    ", chr(10) + "            ")})\n'
                       '            borg_danger_wipe = true;\n\n'
                       '        /* Dangerous grid */\n        if ((p > fear) && !borg.status.desperate')
    return {"before": base, "after": fixed, "neg-spread-only": spread_only,
            "neg-enqueue-only": enqueue_only, "alt-wipe": alt}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--fixtures-ref", default="FETCH_HEAD")
    parser.add_argument("--fix-ref", required=True)
    parser.add_argument("--full-tests", action="store_true",
                        help="also run both full native regression gates (before and after)")
    parser.add_argument("--cmake-arg", action="append", default=[])
    args = parser.parse_args()
    for command in ("git", "cmake", "flock", "cc", "make"):
        if not shutil.which(command):
            raise RuntimeError(f"Required command not found: {command}")
    repo = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
    git = lambda *a: subprocess.check_output(["git", *a], cwd=repo)
    ref = git("rev-parse", args.fixtures_ref).decode().strip()
    fix = git("rev-parse", args.fix_ref).decode().strip()
    for pin in (BASE, fix):
        if subprocess.run(["git", "cat-file", "-e", pin + "^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)
    if git("diff", "--name-only", BASE, fix).decode().split() != [SOURCE]:
        raise RuntimeError("Source scope changed: the correction must touch only " + SOURCE)
    texts = derive(git("show", f"{BASE}:{SOURCE}").decode(), git("show", f"{fix}:{SOURCE}").decode())
    payload = {name: git("show", f"{ref}:{DIRECTORY}{name}") for name in PAYLOAD}

    root = Path(tempfile.mkdtemp(prefix="angband-flowcache-404-", dir="/tmp"))
    source, build, evidence = root / "source", root / "build", root / "evidence"
    (evidence / "profile").mkdir(parents=True)
    for name, data in payload.items():
        (evidence / name).write_bytes(data)
    (evidence / "profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    if (source / "src/borg/borg.txt").read_bytes() != payload["borg.txt"]:
        raise RuntimeError("fixture borg.txt differs from the base source default")
    cmake = source / "CMakeLists.txt"
    cmake.write_text(cmake.read_text() + f'''
add_executable(FC404Observer "{evidence / 'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(FC404Observer PRIVATE src src/tests)
target_compile_definitions(FC404Observer PRIVATE ALLOW_BORG)
target_link_libraries(FC404Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
target_link_options(FC404Observer PRIVATE -Wl,--wrap=borg_danger -Wl,--wrap=borg_note)
add_compile_definitions(PRIVATE_USER_PATH="{root / 'test-user'}")
''')
    script = source / "tests/run-test"
    script.write_text(script.read_text().replace("~/.angband/Test", str(root / "test-user/Test")))

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
    for variant in VARIANTS:
        (source / SOURCE).write_text(texts[variant])
        log, code = run(variant + "-build", ["cmake", "--build", str(build), "--parallel", "2"])
        if code:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"{variant} build failed")
        log, code = run(variant + "-cases", [sys.executable, str(evidence / "run-cases.py"), variant,
                                             str(build / "game/FC404Observer"), str(evidence)])
        if code:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"{variant} case runner failed")
        rows[variant] = json.loads((evidence / f"{variant}-results.json").read_text())
        if args.full_tests and variant in ("before", "after"):
            log, code = run(variant + "-alltests", ["cmake", "--build", str(build), "--target", "alltests",
                                                   "--parallel", "2"])
            text = re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals (965/965 units, 5/5 frontends) missing")
            gates[variant] = {"units": "965/965", "frontends": "5/5"}

    # 1. Pass/fail matrix exactly as expected, per variant.
    for variant in VARIANTS:
        failed = {c for c, r in rows[variant].items() if not r["passed"]}
        if failed != EXPECTED_FAIL[variant]:
            raise RuntimeError(f"{variant}: failing cases {sorted(failed)} != expected {sorted(EXPECTED_FAIL[variant])}")
    # 2. Controls and the documented limit: identical behaviour before and after
    #    (danger-evaluation counts excluded; they are the measured cost).
    def behaviour(row):
        keep = lambda d: {k: v for k, v in d.items() if "calls" not in k}
        return {"state": keep(row.get("state", {})), "outcome": keep(row.get("outcome", {})),
                "passed": row["passed"]}
    for case, row in rows["before"].items():
        if row["role"] in ("control", "limit", "cost"):
            if behaviour(row) != behaviour(rows["after"][case]) and row["role"] != "cost":
                raise RuntimeError(f"{case}: behaviour changed with the correction")
    # 3. The measured danger-evaluation cost must never exceed what a fresh
    #    cache needs, and honest entries must not be recomputed.
    for mode in MODES:
        mixed = lambda v: rows[v][f"costmixed-{mode}"]["state"]["second_normal_calls"]
        if mixed("before") != 0 or mixed("after") != 0:
            raise RuntimeError(f"costmixed-{mode}: honest cache entries were recomputed")
        if mixed("alt-wipe") <= 0:
            raise RuntimeError(f"costmixed-{mode}: the wipe alternative no longer costs more")
    for mode in ("desperate", "digging"):
        fresh = rows["after"]["control-safe"]["state"]["danger_calls"]
        extra = rows["after"][f"safe-{mode}-spread"]["state"]["p2_danger_calls"]
        if extra > fresh + 1:
            raise RuntimeError(f"safe-{mode}-spread: more evaluations ({extra}) than a fresh flow ({fresh})")
    for variant in VARIANTS:
        for case, row in rows[variant].items():
            if row.get("state", {}).get("rng_touched", 0):
                raise RuntimeError(f"{variant}/{case}: borg_danger() changed the RNG state")

    summary = {"base": BASE, "fix": fix, "fixtures_ref": ref, "variants": VARIANTS,
               "variant_sources_sha256": {v: sha(texts[v]) for v in VARIANTS},
               "failed_cases": {v: sorted(c for c, r in rows[v].items() if not r["passed"]) for v in VARIANTS},
               "cases": rows, "full_tests": gates}
    (root / "result.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
    subprocess.run([sys.executable, str(evidence / "make_replay.py"), str(evidence)], check=True)
    print("VERIFIED: the original reuses danger-unchecked cache entries; the correction does not; "
          "controls are unchanged; negative controls fail where predicted.", flush=True)
    print(f"Logs, result.json and replay page: {root}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}", file=sys.stderr)
        sys.exit(1)
