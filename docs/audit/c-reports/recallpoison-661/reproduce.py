#!/usr/bin/env python3
"""Reproduce the recallpoison-661 native evidence: waiting for Recall must respect the pending poison damage.

Run from a clone of diegoami/neo-angband-tracker after fetching
recallpoison-661-evidence. The default fixtures reference FETCH_HEAD is the
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
DIRECTORY = "docs/audit/c-reports/recallpoison-661/"
OBSERVER = 'recallpoison661'
PHASES = ("before", "after")
# Each corrected file must equal the base file with exactly these hunks.
CHANGES = {
    'src/borg/borg-caution.c': [
        ('/**\n * \\file borg-caution.c\n * \\brief Try to be careful and not die when things are dangerous\n *\n * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n * Copyright (c) 2007-9 Andi Sidwell, Chris Carr, Ed Graham, Erik Osheim\n *\n * This work is free software; you can redistribute it and/or modify it\n * under the terms of either:\n *\n * a) the GNU General Public License as published by the Free Software\n *    Foundation, version 2, or\n *\n * b) the "Angband License":\n *    This software may be copied and distributed for educational, research,\n *    and not for profit purposes provided that this copyright and statement\n *    are included in all such copies.  Other copyrights may also apply.\n */\n\n#include "borg-caution.h"\n\n#ifdef ALLOW_BORG\n\n#include "../player-timed.h"\n#include "../ui-event.h"\n\n#include "borg-cave-util.h"\n#include "borg-cave-view.h"\n#include "borg-danger.h"\n#include "borg-escape.h"\n#include "borg-fight-defend.h"\n#include "borg-flow-glyph.h"\n#include "borg-flow-kill.h"\n#include "borg-flow-misc.h"\n#include "borg-flow-stairs.h"\n#include "borg-flow.h"\n#include "borg-inventory.h"\n#include "borg-io.h"\n#include "borg-item-activation.h"\n#include "borg-item-use.h"\n#include "borg-item-val.h"\n#include "borg-item-wear.h"\n#include "borg-light.h"\n#include "borg-magic.h"\n#include "borg-messages-react.h"\n#include "borg-prepared.h"\n#include "borg-store-sell.h"\n#include "borg-trait.h"\n#include "borg-think-dungeon-util.h"\n#include "borg.h"\n\n',
         '/**\n * \\file borg-caution.c\n * \\brief Try to be careful and not die when things are dangerous\n *\n * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n * Copyright (c) 2007-9 Andi Sidwell, Chris Carr, Ed Graham, Erik Osheim\n *\n * This work is free software; you can redistribute it and/or modify it\n * under the terms of either:\n *\n * a) the GNU General Public License as published by the Free Software\n *    Foundation, version 2, or\n *\n * b) the "Angband License":\n *    This software may be copied and distributed for educational, research,\n *    and not for profit purposes provided that this copyright and statement\n *    are included in all such copies.  Other copyrights may also apply.\n */\n\n#include "borg-caution.h"\n\n#ifdef ALLOW_BORG\n\n#include "../player-timed.h"\n#include "../ui-event.h"\n\n#include "borg-cave-util.h"\n#include "borg-cave-view.h"\n#include "borg-danger.h"\n#include "borg-escape.h"\n#include "borg-fight-defend.h"\n#include "borg-flow-glyph.h"\n#include "borg-flow-kill.h"\n#include "borg-flow-misc.h"\n#include "borg-flow-stairs.h"\n#include "borg-flow.h"\n#include "borg-inventory.h"\n#include "borg-io.h"\n#include "borg-item-activation.h"\n#include "borg-item-use.h"\n#include "borg-item-val.h"\n#include "borg-item-wear.h"\n#include "borg-light.h"\n#include "borg-magic.h"\n#include "borg-messages-react.h"\n#include "borg-prepared.h"\n#include "borg-store-sell.h"\n#include "borg-trait.h"\n#include "borg-think-dungeon-util.h"\n#include "borg.h"\n\n#include "../player-calcs.h"\n\n/*\n * #661: poison already in the system keeps dealing damage while the\n * Borg waits for Recall to lift off.  The engine deals one point of\n * poison damage per world turn (before damage reduction) and lowers\n * the poison timer by adj_con_fix(CON) + 1 per turn; Recall counts\n * down player->word_recall world turns and poison is processed before\n * the Recall countdown in each world tick.  Return the damage the\n * pending poison can still deal before lift-off, 0 if none.\n */\nstatic int borg_poison_damage_to_liftoff(void)\n{\n    int pois, decr, poison_turns;\n\n    if (!borg.trait[BI_ISPOISONED] || !player->word_recall)\n        return 0;\n\n    pois          = player->timed[TMD_POISONED];\n    decr          = adj_con_fix[player->state.stat_ind[STAT_CON]] + 1;\n    poison_turns  = (pois + decr - 1) / decr;\n\n    return (poison_turns < player->word_recall) ? poison_turns\n                                                : player->word_recall;\n}\n\n'),
        ('    /* If I am waiting for recall,  & safe, then stay put. */\n    if (borg.goal.recalling && borg_check_rest(borg.c.y, borg.c.x)\n        && borg.trait[BI_CDEPTH] && !borg.trait[BI_ISHUNGRY]) {\n        /* rest here until lift off */\n        borg_note("# Resting for Recall.");\n        borg_keypress(\'R\');\n        borg_keypress(\'5\');\n        borg_keypress(\'0\');\n        borg_keypress(\'0\');\n        borg_keypress(KC_ENTER);\n\n        /* I\'m not in a store */\n        borg.in_shop = false;\n\n        return true;\n    }\n\n',
         '    /* If I am waiting for recall,  & safe, then stay put.\n     * #661: not if the poison already in the system can kill the Borg\n     * before Recall lifts off - every resting turn is a poison tick,\n     * and the engine processes poison before the Recall countdown. */\n    if (borg.goal.recalling && borg_check_rest(borg.c.y, borg.c.x)\n        && borg.trait[BI_CDEPTH] && !borg.trait[BI_ISHUNGRY]\n        && borg_poison_damage_to_liftoff() < borg.trait[BI_CURHP]) {\n        /* rest here until lift off */\n        borg_note("# Resting for Recall.");\n        borg_keypress(\'R\');\n        borg_keypress(\'5\');\n        borg_keypress(\'0\');\n        borg_keypress(\'0\');\n        borg_keypress(KC_ENTER);\n\n        /* I\'m not in a store */\n        borg.in_shop = false;\n\n        return true;\n    }\n\n    /* #661: waiting for Recall while the pending poison can kill\n     * before lift-off: heal instead of waiting.  borg_heal() is the\n     * production cure cascade (it is skipped while fleeing, which is\n     * exactly the Recall-wait situation); with no cure left, the\n     * regular flows keep the Borg moving instead of resting into the\n     * poison. */\n    if (borg.goal.recalling && borg.trait[BI_CDEPTH]\n        && borg_poison_damage_to_liftoff() >= borg.trait[BI_CURHP]\n        && borg_heal(pos_danger))\n        return true;\n\n'),
    ],
}
# Per mode: (expected pass before, expected pass after, byte-identical control).
EXPECT = {'lethal_cure': (False, True, False), 'lethal_no_cure': (False, True, False), 'boundary': (False, True, False), 'survivable': (True, True, True), 'unpoisoned': (True, True, True)}


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

    root = Path(tempfile.mkdtemp(prefix="angband-recallpoison-661-", dir="/tmp"))
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
    print('VERIFIED: fail-before/pass-after on the lethal-with-cure, lethal-without-cure and exact-boundary cases, with the survivable-poison and unpoisoned controls byte-identical, gates 966/966 unit checks and 5/5 frontend tests on both variants, at base 574b618c3', flush=True)
    print(f"Logs and result.json: {root}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}", file=sys.stderr)
        sys.exit(1)
