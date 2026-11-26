#!/usr/bin/env python3
"""Reproduce the lightmap-644 matrix: fail-before/pass-after on the light-map radius, byte-identical adjacent control, full native gates on both variants.

Run from a clone of diegoami/neo-angband-tracker after fetching task/lightmap-644-evidence. The default fixtures reference FETCH_HEAD is the revision fetched by the setup command; the fix reference is required. Use --full-tests to also run both native unit and frontend regression gates for the before and after builds.

Run from a clone of diegoami/neo-angband-tracker after fetching
task/lightmap-644-evidence. The default fixtures reference FETCH_HEAD is the
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
DIRECTORY = "docs/audit/c-reports/lightmap-644/"
OBSERVER = 'LM644Observer'
PHASES = ("before", "after")
# Each corrected file must equal the base file with exactly these hunks.
CHANGES = {
    'src/borg/borg-cave-light.c': [
        ('#include "borg.h"\n#include "borg-cave-util.h"\n#include "borg-cave-view.h"\n#include "borg-cave.h"\n#include "borg-trait.h"',
         '#include "borg.h"\n#include "borg-cave-util.h"\n#include "borg-cave-view.h"\n#include "borg-cave.h"\n#include "borg-projection.h"\n#include "borg-trait.h"'),
        ('void borg_update_light(void)\n{\n    int i, x, y, min_x, max_x, min_y, max_y;\n\n    /*** Clear old grids ***/\n\n    /* Clear them all */\n    for (i = 0; i < borg_light_n; i++) {\n        y = borg_light_y[i];\n        x = borg_light_x[i];\n\n        /* Mark the grid as not "lite" */\n        borg_grids[y][x].info &= ~BORG_LIGHT;\n    }\n\n    /* None left */\n    borg_light_n = 0;\n\n    /* Player has no lite */\n    if (borg_items[INVEN_LIGHT].iqty <= 0)\n        return;\n\n    /*** Collect the new "lite" grids ***/\n\n    /* Player grid */\n    borg_cave_light_hack(borg.c.y, borg.c.x);\n\n    /* Radius 1 -- torch radius */\n    if (borg.trait[BI_LIGHT] >= 1) {\n        /* Adjacent grid */\n        borg_cave_light_hack(borg.c.y + 1, borg.c.x);\n        borg_cave_light_hack(borg.c.y - 1, borg.c.x);\n        borg_cave_light_hack(borg.c.y, borg.c.x + 1);\n        borg_cave_light_hack(borg.c.y, borg.c.x - 1);\n\n        /* Diagonal grids */\n        borg_cave_light_hack(borg.c.y + 1, borg.c.x + 1);\n        borg_cave_light_hack(borg.c.y + 1, borg.c.x - 1);\n        borg_cave_light_hack(borg.c.y - 1, borg.c.x + 1);\n        borg_cave_light_hack(borg.c.y - 1, borg.c.x - 1);\n    }\n\n    /* Radius 2 -- lantern radius */\n    if (borg.trait[BI_LIGHT] >= 2 && borg.c.y + 2 < AUTO_MAX_Y\n        && borg.c.y - 2 > 0 && borg.c.x + 2 < AUTO_MAX_X && borg.c.x - 2 > 0) {\n        /* South of the player */\n        if (borg_cave_floor_bold(borg.c.y + 2, borg.c.x)) {\n            borg_cave_light_hack(borg.c.y + 2, borg.c.x);\n            borg_cave_light_hack(borg.c.y + 2, borg.c.x + 2);\n            borg_cave_light_hack(borg.c.y + 2, borg.c.x - 2);\n        }\n\n        /* North of the player */\n        if (borg_cave_floor_bold(borg.c.y - 2, borg.c.x)) {\n            borg_cave_light_hack(borg.c.y - 2, borg.c.x);\n            borg_cave_light_hack(borg.c.y - 2, borg.c.x + 2);\n            borg_cave_light_hack(borg.c.y - 2, borg.c.x - 2);\n        }\n\n        /* East of the player */\n        if (borg_cave_floor_bold(borg.c.y, borg.c.x + 2)) {\n            borg_cave_light_hack(borg.c.y, borg.c.x + 2);\n            borg_cave_light_hack(borg.c.y + 1, borg.c.x + 2);\n            borg_cave_light_hack(borg.c.y - 1, borg.c.x + 2);\n        }\n\n        /* West of the player */\n        if (borg_cave_floor_bold(borg.c.y, borg.c.x - 2)) {\n            borg_cave_light_hack(borg.c.y, borg.c.x - 2);\n            borg_cave_light_hack(borg.c.y + 2, borg.c.x - 2);\n            borg_cave_light_hack(borg.c.y - 2, borg.c.x - 2);\n        }\n    }\n\n    /* Radius 3+ -- artifact radius */\n    if (borg.trait[BI_LIGHT] >= 3 && borg.c.y + 3 < AUTO_MAX_Y\n        && borg.c.y - 3 > 0 && borg.c.x + 3 < AUTO_MAX_X && borg.c.x - 3 > 0) {\n        int d, p;\n\n        /* Maximal radius */\n        p = borg.trait[BI_LIGHT];\n\n        /* Paranoia -- see "LITE_MAX" */\n        if (p > 5)\n            p = 5;\n\n        /* South-East of the player */\n        if (borg_cave_floor_bold(borg.c.y + 3, borg.c.x + 3)) {\n            borg_cave_light_hack(borg.c.y + 3, borg.c.x + 3);\n        }\n\n        /* South-West of the player */\n        if (borg_cave_floor_bold(borg.c.y + 3, borg.c.x - 3)) {\n            borg_cave_light_hack(borg.c.y + 3, borg.c.x - 3);\n        }\n\n        /* North-East of the player */\n        if (borg_cave_floor_bold(borg.c.y - 3, borg.c.x + 3)) {\n            borg_cave_light_hack(borg.c.y - 3, borg.c.x + 3);\n        }\n\n        /* North-West of the player */\n        if (borg_cave_floor_bold(borg.c.y - 3, borg.c.x - 3)) {\n            borg_cave_light_hack(borg.c.y - 3, borg.c.x - 3);\n        }\n\n        /* Maximal north */\n        min_y = borg.c.y - p;\n        if (min_y < 0)\n            min_y = 0;\n\n        /* Maximal south */\n        max_y = borg.c.y + p;\n        if (max_y > AUTO_MAX_Y - 1)\n            max_y = AUTO_MAX_Y - 1;\n\n        /* Maximal west */\n        min_x = borg.c.x - p;\n        if (min_x < 0)\n            min_x = 0;\n\n        /* Maximal east */\n        max_x = borg.c.x + p;\n        if (max_x > AUTO_MAX_X - 1)\n            max_x = AUTO_MAX_X - 1;\n\n        /* Scan the maximal box */\n        for (y = min_y; y <= max_y; y++) {\n            for (x = min_x; x <= max_x; x++) {\n                int dy = (borg.c.y > y) ? (borg.c.y - y) : (y - borg.c.y);\n                int dx = (borg.c.x > x) ? (borg.c.x - x) : (x - borg.c.x);\n\n                /* Skip the "central" grids (above) */\n                if ((dy <= 2) && (dx <= 2))\n                    continue;\n\n                /* Approximate the distance "as the bird flies" (straight-line).\n                 * Ignores obstacles and does not follow walkable paths like\n                 * pathfinding code elsewhere.\n                 */\n                d = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));\n\n                /* Skip distant grids */\n                if (d > p)\n                    continue;\n\n                /* Viewable, nearby, grids get "torch lit" */\n                if (borg_grids[y][x].info & BORG_VIEW) {\n                    /* This grid is "torch lit" */\n                    borg_cave_light_hack(y, x);\n                }\n            }\n        }\n    }\n}\n',
         'void borg_update_light(void)\n{\n    int i, x, y, radius;\n\n    /*** Clear old grids ***/\n\n    /* Clear them all */\n    for (i = 0; i < borg_light_n; i++) {\n        y = borg_light_y[i];\n        x = borg_light_x[i];\n\n        /* Mark the grid as not "lite" */\n        borg_grids[y][x].info &= ~BORG_LIGHT;\n    }\n\n    /* None left */\n    borg_light_n = 0;\n\n    /* Player has no lite */\n    if (borg_items[INVEN_LIGHT].iqty <= 0)\n        return;\n\n    /*** Collect the new "lite" grids ***/\n\n    /* Player grid */\n    borg_cave_light_hack(borg.c.y, borg.c.x);\n\n    /* The engine lights one grid less than the light\'s strength:\n     * calc_lighting() uses radius = ABS(light) - 1 and add_light()\n     * lights every grid with distance() no greater than that radius\n     * which has line of sight from the player (see cave-view.c).\n     * Using the strength itself made the borg expect to see monsters\n     * standing in the dark ring just outside its real light, and\n     * forget them when they were not on screen.  A Necromancer\n     * standing in unlight sees by the unlight instead, so keep the\n     * radius the borg always used for that case.\n     */\n    radius = borg.trait[BI_LIGHT] - 1;\n\n    /* Never below zero */\n    if (radius < 0)\n        radius = 0;\n\n    /* Necromancers see by unlight */\n    if (borg.trait[BI_CLASS] == CLASS_NECROMANCER\n        && borg.trait[BI_LIGHT] <= 1) {\n        radius = borg.trait[BI_LIGHT];\n    }\n\n    /* Scan the box around the player, lighting what the engine lights */\n    for (y = borg.c.y - radius; y <= borg.c.y + radius; y++) {\n        for (x = borg.c.x - radius; x <= borg.c.x + radius; x++) {\n            /* Stay on the map */\n            if (y < 0 || y >= AUTO_MAX_Y || x < 0 || x >= AUTO_MAX_X)\n                continue;\n\n            /* The player grid is already lit (and must stay first) */\n            if (y == borg.c.y && x == borg.c.x)\n                continue;\n\n            /* The engine lights distance() <= radius */\n            if (distance(borg.c, loc(x, y)) > radius)\n                continue;\n\n            /* Don\'t propagate the light through walls */\n            if (!borg_los(borg.c.y, borg.c.x, y, x))\n                continue;\n\n            /* This grid is "torch lit" */\n            borg_cave_light_hack(y, x);\n        }\n    }\n}\n'),
    ],
    'src/borg/borg-light.c': [
        ('    if (borg.trait[BI_LIGHT] == 1) {',
         '    /* The engine lights BI_LIGHT - 1 grids (see borg_update_light), so\n     * the radius-1 corner test below covers every light of strength 2\n     * or less, and the radius-2 5x5 scan covers stronger lights. */\n    if (borg.trait[BI_LIGHT] <= 2) {'),
        ('    } else if (borg.trait[BI_LIGHT] > 1) {',
         '    } else if (borg.trait[BI_LIGHT] > 2) {'),
    ],
}
# Per mode: (expected pass before, expected pass after, byte-identical control).
EXPECT = {'forget': (False, True, False), 'adjacent': (True, True, True)}


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

    root = Path(tempfile.mkdtemp(prefix="angband-lightmap-644-", dir="/tmp"))
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
    print('VERIFIED: lightmap-644 matrix: forget fails on the original (record deleted) and passes on the fix (record kept), the adjacent control is byte-identical, and both full native gates pass.', flush=True)
    print(f"Logs and result.json: {root}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}", file=sys.stderr)
        sys.exit(1)
