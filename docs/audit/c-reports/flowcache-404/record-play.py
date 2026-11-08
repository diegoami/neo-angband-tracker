#!/usr/bin/env python3
"""Record the native frames behind play.html (flowcache-404).

Builds the pinned base twice (original, and with the correction), runs the
frame recorder (play-frames.c) for every scene against each, and writes
play-data.json and play.html.  The frames are printed by the real game code
(borg_caution, borg_flow_dark, borg_flow_spread, borg_flow_enqueue_grid,
borg_update); nothing in the page is simulated.

usage: record-play.py --fix-ref <commit> [--fixtures-ref <ref>] [--out <dir>]
"""
import argparse
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

BASE = "e473720ed88a0f0c7ca66815cef3000eb4a38bc3"
SOURCE = "src/borg/borg-flow.c"
DIRECTORY = "docs/audit/c-reports/flowcache-404/"
PAYLOAD = ("play-frames.c", "make_play.py", "borg.txt")
MODES = ("desperate", "lunal", "munchkin", "digging")
SCENES = ([f"walk-{s}" for s in ("visible", "remembered", "expired")]
          + [f"modes-{m}-{e}" for m in MODES for e in ("spread", "enqueue")])
ANSI = re.compile(r"\x1b\[[0-9;]*m")
FRAME = re.compile(r"FRAME (\{.*\})")


def collect(binary, profile, scenes=SCENES):
    """Run every scene against one build; return {scene: [frame, ...]}."""
    out = {}
    for scene in scenes:
        env = os.environ | {"FC404_PROFILE": str(profile), "FC404_CASE": scene}
        run = subprocess.run([str(binary), "-v"], env=env, cwd=Path(binary).parent, timeout=180,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        text = ANSI.sub("", run.stdout)
        frames = [json.loads(m) for m in FRAME.findall(text)]
        if run.returncode or "finished: 1/1 passed" not in text or not frames:
            raise RuntimeError(f"scene {scene} failed:\n{text[-1500:]}")
        out[scene] = frames
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--fixtures-ref", default="FETCH_HEAD")
    parser.add_argument("--fix-ref", required=True)
    parser.add_argument("--cmake-arg", action="append", default=[])
    parser.add_argument("--out", type=Path, help="existing directory to receive play-data.json and play.html")
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
    texts = {"before": git("show", f"{BASE}:{SOURCE}").decode(), "after": git("show", f"{fix}:{SOURCE}").decode()}
    payload = {name: git("show", f"{ref}:{DIRECTORY}{name}") for name in PAYLOAD}

    root = Path(tempfile.mkdtemp(prefix="angband-flowcache-404-play-", dir="/tmp"))
    source, build, evidence = root / "source", root / "build", root / "evidence"
    (evidence / "profile").mkdir(parents=True)
    for name, data in payload.items():
        (evidence / name).write_bytes(data)
    (evidence / "profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native recording directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    if (source / "src/borg/borg.txt").read_bytes() != payload["borg.txt"]:
        raise RuntimeError("fixture borg.txt differs from the base source default")
    cmake = source / "CMakeLists.txt"
    cmake.write_text(cmake.read_text() + f'''
add_executable(FC404Play "{evidence / 'play-frames.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(FC404Play PRIVATE src src/tests)
target_compile_definitions(FC404Play PRIVATE ALLOW_BORG)
target_link_libraries(FC404Play PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
target_link_options(FC404Play PRIVATE -Wl,--wrap=borg_danger -Wl,--wrap=borg_note)
add_compile_definitions(PRIVATE_USER_PATH="{root / 'test-user'}")
''')

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
    frames = {}
    for variant in ("before", "after"):
        (source / SOURCE).write_text(texts[variant])
        log, code = run(variant + "-build", ["cmake", "--build", str(build), "--parallel", "4"])
        if code:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"{variant} build failed")
        frames[variant] = collect(build / "game/FC404Play", evidence / "profile")
    data = {"base": BASE, "fix": fix, "fixtures_ref": ref, "frames": frames}
    (evidence / "play-data.json").write_text(json.dumps(data, separators=(",", ":")) + "\n")
    subprocess.run([sys.executable, str(evidence / "make_play.py"), str(evidence)], check=True)
    if args.out:
        for name in ("play-data.json", "play.html"):
            shutil.copy2(evidence / name, args.out / name)
    print(f"Page: {evidence / 'play.html'}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Recording failed: {error}", file=sys.stderr)
        sys.exit(1)
