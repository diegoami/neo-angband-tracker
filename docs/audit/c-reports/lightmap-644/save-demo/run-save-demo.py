#!/usr/bin/env python3
"""lightmap-644 synthetic savefile demonstration (issue #644).

Builds the base (574b618c3) and the fix (task/lightmap-644), generates one
synthetic savefile with savegen.c (a walked dark dead-end corridor, a
torch-carrying Warrior, one awake soldier ant one grid east, visible),
loads it in every build through the production loader and the borg's real
observation path (saveplay.c), and compares the memory of the ant when it
retreats one grid east into the dark.

Expected:
  retreat  base: the record is deleted (the dark grid is lit by the
                borg's light map, the record is judged viewable and
                forgotten)      -> fix: the record follows the ant east
  stay     control: the record is kept by both, logs byte-identical

Heavy steps (configure/build) hold /tmp/angband-validation.lock.
"""

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

DEFAULT_REPO = "/home/diego/projects/angband-workspace/neo-angband-tracker"
BASE = "574b618c3bfec595f3503a58f8e488b1c4ae3a46"
FIX = "d8d31359ec2ef9c73c9d9aa4b92579dc73a3b5fb"
MODES = ["retreat", "worm", "lure", "stay"]
SAVES = {"retreat": "lightmap644-retreat.sav", "lure": "lightmap644-lure.sav",
         "worm": "lightmap644-worm.sav", "stay": "lightmap644-stay.sav"}
# kept expectation per build and mode
EXPECT = {
    "retreat": {"base": 0, "fix": 1},
    "worm": {"base": 0, "fix": 1},
    "lure": {"base": 0, "fix": 1},
    "stay": {"base": 1, "fix": 1},
}


def run(command, log, cwd=None, env=None, lock=False, check=True):
    if lock:
        command = ["flock", "/tmp/angband-validation.lock", *command]
    with open(log, "w") as out:
        result = subprocess.run(command, cwd=cwd, env=env,
                                stdout=out, stderr=subprocess.STDOUT)
    if check and result.returncode:
        print(f"FAILED ({result.returncode}): {' '.join(map(str, command))}")
        print(f"log: {log}")
        with open(log) as f:
            print(f.read()[-4000:])
        sys.exit(1)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default=DEFAULT_REPO)
    parser.add_argument("--out", default=None)
    parser.add_argument("--borg-txt", default=None,
                        help="profile borg.txt (default: the package's "
                             "../profile/borg.txt next to this script)")
    parser.add_argument("--skip-build", action="store_true",
                        help="reuse existing source-*/build-* directories")
    args = parser.parse_args()
    repo = Path(args.repo).resolve()
    demo = Path(__file__).resolve().parent
    out = Path(args.out).resolve() if args.out else demo.parent / "out"
    out.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)

    builds = {}
    for label, ref in (("base", BASE), ("fix", FIX)):
        source = out / f"source-{label}"
        build = out / f"build-{label}"
        if not args.skip_build or not source.exists():
            if source.exists():
                shutil.rmtree(source)
            run(["git", "worktree", "add", "--detach", str(source), ref],
                str(out / f"worktree-{label}.log"), cwd=repo)
            cmake = source / "CMakeLists.txt"
            cmake.write_text(cmake.read_text() + f"""
add_executable(LM644SG "{demo / 'savegen.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
add_executable(LM644SP "{demo / 'saveplay.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(LM644SG PRIVATE src src/tests)
target_include_directories(LM644SP PRIVATE src src/tests)
target_compile_definitions(LM644SG PRIVATE ALLOW_BORG)
target_compile_definitions(LM644SP PRIVATE ALLOW_BORG)
target_link_libraries(LM644SG PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
target_link_libraries(LM644SP PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{out / 'test-user'}")
""")
            run(["cmake", "-S", str(source), "-B", str(build), "-DSUPPORT_BORG=ON",
                 "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug"],
                str(out / f"configure-{label}.log"), lock=True)
        run(["cmake", "--build", str(build), "--target", "LM644SG", "LM644SP",
             "TransferLib", "--parallel", "2"],
            str(out / f"build-{label}.log"), lock=True)
        builds[label] = build

    profile = out / "profile"
    (profile / "save").mkdir(parents=True, exist_ok=True)
    borg_txt = profile / "borg.txt"
    if not borg_txt.exists():
        source_borg = (Path(args.borg_txt) if args.borg_txt
                       else demo.parent / "profile" / "borg.txt")
        if not source_borg.exists():
            source_borg = Path("/tmp/c-evidence-work/lightmap-644/profile/borg.txt")
        shutil.copy(source_borg, borg_txt)

    def find_binary(build, name):
        matches = [p for p in build.rglob(name)
                   if p.is_file() and os.access(p, os.X_OK)]
        if not matches:
            print(f"binary {name} not found under {build}")
            sys.exit(1)
        return matches[0]

    # One savefile per mode, written once with the base build.
    gen = find_binary(builds["base"], "LM644SG")
    for mode, savename in SAVES.items():
        run([str(gen), "-v"], str(out / f"savegen-{mode}.log"),
            env=dict(env, LM644S_PROFILE=str(profile), LM644S_MODE=mode,
                     LM644S_SAVE=savename),
            cwd=str(gen.parent))
        save = profile / "save" / savename
        if not save.exists():
            print(f"savefile not written: {save}")
            sys.exit(1)
        print(f"save: {save} ({save.stat().st_size} bytes, "
              f"sha256 {hashlib.sha256(save.read_bytes()).hexdigest()[:16]})")

    rows = {}
    for label in ("base", "fix"):
        binary = find_binary(builds[label], "LM644SP")
        for mode in MODES:
            log = out / f"{label}-{mode}.log"
            run([str(binary), "-v"], str(log),
                env=dict(env, LM644S_PROFILE=str(profile), LM644S_MODE=mode,
                         LM644S_SAVE=SAVES[mode]),
                cwd=str(binary.parent))
            text = log.read_text()
            kept = re.search(r"OUTCOME kept=(-?\d+)", text)
            record = re.search(
                r"STATE kills=(-?\d+) tracked_x=(-?\d+) monster_x=(-?\d+) flowed=(\d+)", text)
            decision = re.search(r"DECISION flowed=(\d+) keys=\[([^\]]*)\] step=(-?\d+)", text)
            rows[(label, mode)] = {
                "kept": int(kept.group(1)) if kept else None,
                "kills": int(record.group(1)) if record else None,
                "tracked_x": int(record.group(2)) if record else None,
                "monster_x": int(record.group(3)) if record else None,
                "flowed": int(record.group(4)) if record else 0,
                "keys": decision.group(2) if decision else None,
                "step_key": int(decision.group(3)) if decision else None,
                "passed": "native-c/lightmap-644-saveplay finished: 1/1 passed" in text,
            }
            print(label, mode, rows[(label, mode)])

    ok = True
    for mode in MODES:
        for label in ("base", "fix"):
            if rows[(label, mode)]["kept"] != EXPECT[mode][label]:
                print(f"UNEXPECTED {label}/{mode}: kept={rows[(label, mode)]['kept']}")
                ok = False

    # The lure is the acted-out consequence: after the two dances the
    # base borg has forgotten all three worm masses but kept the kobold
    # (the scary guy that sends it back east for the stairs), and its
    # first movement key is a step east (54 = '6') straight toward the
    # forgotten masses; the fixed borg keeps all four records, refuses
    # the east route (the flow through the corner is priced at some 100
    # against 12 HP), and steps away instead (50 = '2', south into the
    # corridor's wide row) - with no escape kit, what it does on foot
    # is keep its distance.
    if rows[("base", "lure")]["step_key"] != 54:
        print(f"UNEXPECTED base/lure step: {rows[('base', 'lure')]['step_key']} (want 54, '6' east)")
        ok = False
    if rows[("fix", "lure")]["step_key"] != 50:
        print(f"UNEXPECTED fix/lure step: {rows[('fix', 'lure')]['step_key']} (want 50, '2': it turns away from the remembered corner)")
        ok = False
    if rows[("base", "lure")]["kills"] != 1:
        print(f"UNEXPECTED base/lure kills: {rows[('base', 'lure')]['kills']} (want 1: the kobold, all three worms forgotten)")
        ok = False
    if rows[("fix", "lure")]["kills"] != 4:
        print(f"UNEXPECTED fix/lure kills: {rows[('fix', 'lure')]['kills']} (want 4: the kobold and all three worms)")
        ok = False

    if (out / "base-stay.log").read_bytes() != (out / "fix-stay.log").read_bytes():
        print("control logs are NOT byte-identical")
        ok = False
    else:
        print("control byte-identical: stay")

    digest = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(out.glob("*.log"))}
    for savename in SAVES.values():
        digest[savename] = hashlib.sha256((profile / "save" / savename).read_bytes()).hexdigest()
    (out / "demo-result.json").write_text(
        __import__("json").dumps({"base": BASE, "fix": FIX, "saves": SAVES,
                                  "modes": {m: {l: rows[(l, m)] for l in ("base", "fix")}
                                            for m in MODES},
                                  "sha256": digest}, indent=2) + "\n")
    print("PASS" if ok else "FAIL")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
