#!/usr/bin/env python3
"""Run every flowcache-404 case against one native build and record the results.

usage: run-cases.py <variant> <FC404Observer binary> <evidence directory>
"""
import json
import os
import re
import subprocess
import sys
from pathlib import Path

# case -> role.  contract: the correct behaviour is asserted by the case itself
# (it fails on the original).  limit: both builds must agree (documents a limit
# of the correction).  cost: counts danger evaluations.  control: behaviour must
# not change.
MODES = ("desperate", "lunal", "munchkin", "digging")
CASES = {}
for mode in MODES:
    CASES[f"cache-{mode}-spread"] = "contract"
    CASES[f"cache-{mode}-enqueue"] = "contract"
CASES["caution"] = "contract"
CASES["prod-visible"] = "contract"
CASES["prod-remembered"] = "contract"
CASES["prod-expired"] = "limit"
for mode in MODES:
    CASES[f"safe-{mode}-spread"] = "cost"
for mode in MODES:
    CASES[f"cost-{mode}"] = "cost"
for mode in MODES:
    CASES[f"costmixed-{mode}"] = "cost"
for name in ("safe", "dangerous", "blocked", "retreat", "reuse"):
    CASES[f"control-{name}"] = "control"
CASES["retreat-after-normal"] = "control"

ANSI = re.compile(r"\x1b\[[0-9;]*m")


def parse(raw):
    row = {}
    for tag, key in (("STATE ", "state"), ("OUTCOME ", "outcome"), ("RATING ", "rating")):
        for match in re.findall(r"^" + re.escape(tag) + r"(.*)$", raw, re.M):
            row.setdefault(key, {}).update({k: int(v) for k, v in re.findall(r"(\w+)=(-?\d+)", match)})
    row["notes"] = re.findall(r"^NOTE (.*)$", raw, re.M)
    row["update_notes"] = re.findall(r"^UPDATENOTE (.*)$", raw, re.M)
    row["calls"] = re.findall(r"^CALLS (.*)$", raw, re.M)
    row["geometry"] = re.findall(r"^GEOMETRY (.*)$", raw, re.M)
    return row


def main():
    variant, binary, directory = sys.argv[1:]
    root = Path(directory)
    binary = Path(binary).resolve()
    rows, log = {}, []
    for case, role in CASES.items():
        env = os.environ | {"FC404_PROFILE": str(root / "profile"), "FC404_CASE": case}
        result = subprocess.run([str(binary), "-v"], env=env, cwd=binary.parent, timeout=180,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        raw = ANSI.sub("", result.stdout)
        log.append(f"===== {variant} {case} (exit {result.returncode}) =====\n{raw}")
        row = parse(raw)
        row.update(role=role, exit=result.returncode,
                   passed="native-c/flowcache-404 finished: 1/1 passed" in raw)
        rows[case] = row
    (root / f"{variant}-cases.log").write_text("\n".join(log))
    (root / f"{variant}-results.json").write_text(json.dumps(rows, indent=2, sort_keys=True) + "\n")
    failed = sorted(c for c, r in rows.items() if not r["passed"])
    print(f"{variant}: {len(rows) - len(failed)}/{len(rows)} cases passed; failed: {', '.join(failed) or 'none'}")


if __name__ == "__main__":
    main()
