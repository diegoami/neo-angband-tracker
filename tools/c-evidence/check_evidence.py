#!/usr/bin/env python3
"""Check a native C fix evidence package (and optionally its GitHub side)
against the C-FIX-POSTING standard.

Usage:
  check_evidence.py --evidence-pr N            # find dir, receipt, source PR, issue
  check_evidence.py --all                      # every open evidence PR on master
  check_evidence.py --ref REF --dir DIR        # package checks only

Package checks read committed bytes with `git show REF:PATH` (no checkout).
GitHub checks use `gh api` GET calls only. Exit status 1 if any FAIL.
"""
import argparse, hashlib, json, re, subprocess, sys

REPO = "diegoami/neo-angband-tracker"
GENERATORS = ("scenario.c", "run-cases.py", "make_replay.py", "reproduce.py", "borg.txt")
ARTIFACTS = ("README.md", "replay.html", "replay-data.json", "receipt.json")
PLACEHOLDERS = ("EVID_SHA", "pull/SOURCE", "#…", "<SOURCE>", "<EVIDENCE>")


def git(*args, check=True):
    r = subprocess.run(["git", *args], capture_output=True)
    if check and r.returncode:
        raise RuntimeError(r.stderr.decode().strip())
    return r


def show(ref, path):
    r = git("show", f"{ref}:{path}", check=False)
    return r.stdout if r.returncode == 0 else None


def gh(path):
    r = subprocess.run(["gh", "api", f"repos/{REPO}/{path}"], capture_output=True, text=True)
    if r.returncode:
        return None
    return json.loads(r.stdout)


def sha_exists(sha):
    if not sha:
        return False
    if git("cat-file", "-e", f"{sha}^{{commit}}", check=False).returncode == 0:
        return True
    git("fetch", "-q", "--no-tags", "origin", sha, check=False)
    return git("cat-file", "-e", f"{sha}^{{commit}}", check=False).returncode == 0


class Report:
    def __init__(self, title):
        self.title, self.rows = title, []

    def check(self, ok, what, warn=False):
        self.rows.append(("PASS" if ok else ("WARN" if warn else "FAIL"), what))
        return ok

    @property
    def failed(self):
        return any(s == "FAIL" for s, _ in self.rows)

    def print(self):
        print(f"== {self.title}: {'FAIL' if self.failed else 'PASS'}")
        for s, w in self.rows:
            if s != "PASS":
                print(f"   {s}: {w}")


def check_package(ref, d, rep):
    d = d.rstrip("/") + "/"
    tree = git("ls-tree", "-r", "--name-only", ref, d, check=False).stdout.decode().split()
    names = {p[len(d):] for p in tree}
    for f in GENERATORS:
        rep.check(f in names, f"generator {f} committed")
    for f in ARTIFACTS:
        rep.check(f in names, f"artifact {f} committed")
    logs = sorted(n for n in names if n.endswith(".log"))
    rep.check(any(n.startswith("before-") for n in logs) and any(n.startswith("after-") for n in logs),
              "before/after scenario logs committed")
    rep.check(any("alltests" in n for n in logs), "full-gate (alltests) logs committed")
    raw = show(ref, d + "receipt.json")
    if not raw:
        return None
    try:
        receipt = json.loads(raw)
    except ValueError:
        rep.check(False, "receipt.json parses")
        return None
    hashes = receipt.get("sha256") or {}
    if rep.check(bool(hashes), "receipt has a sha256 map"):
        missing = [k for k in hashes if k not in names]
        rep.check(not missing, f"every hashed file committed (missing {len(missing)}: {', '.join(missing[:4])}{'...' if len(missing) > 4 else ''})")
        bad = []
        for k, v in hashes.items():
            if k in names:
                data = show(ref, d + k)
                if data is not None and hashlib.sha256(data).hexdigest() != v:
                    bad.append(k)
        rep.check(not bad, f"committed bytes match receipt hashes (mismatch: {', '.join(bad[:4])})")
    for key in ("base", "fix", "fixtures_ref"):
        sha = receipt.get(key)
        rep.check(sha_exists(sha), f"receipt {key} {str(sha)[:12]} exists")
    controls = receipt.get("identical_controls") or receipt.get("identical_full_logs") or []
    if rep.check(bool(controls), "at least one byte-identical control declared"):
        for c in controls:
            b, a = show(ref, f"{d}before-{c}.log"), show(ref, f"{d}after-{c}.log")
            if b is not None and a is not None:
                rep.check(a == b, f"control '{c}' logs byte-identical")
    gates = json.dumps(receipt.get("full_tests", {}))
    rep.check(gates.count("965/965") >= 2 and gates.count("5/5") >= 2, "full gates 965/965 + 5/5 recorded for both phases")
    render = json.dumps({k: receipt.get(k) for k in ("render_check", "browser_verification")})
    rep.check("planned" not in render.lower() and render != '{"render_check": null, "browser_verification": null}',
              "replay render check recorded (not 'planned')")
    text = (show(ref, d + "README.md") or b"").decode(errors="replace") + raw.decode(errors="replace")
    hits = [p for p in PLACEHOLDERS if p in text]
    rep.check(not hits, f"no placeholders in README/receipt ({', '.join(hits)})")
    readme = (show(ref, d + "README.md") or b"").decode(errors="replace")
    rep.check("--fixtures-ref" in readme or re.search(r"git show [0-9a-f]{40}:", readme) is not None,
              "README repeat commands pin the fixtures by sha", warn=True)
    if "asan" in json.dumps(receipt).lower():
        rep.check(any("asan" in n for n in logs), "asan logs committed for an asan receipt")
    return receipt


def check_github(issue_no, src_no, ev_no, rep):
    issue = gh(f"issues/{issue_no}") if issue_no else None
    if not rep.check(issue is not None, f"issue #{issue_no} readable"):
        return
    body = issue.get("body") or ""
    labels = [l["name"] for l in issue.get("labels", [])]
    head = body[:2500]
    rep.check("compare/master...diegoami:neo-angband-tracker:" in body, "issue has the upstream compare link")
    rep.check("compare/master...diegoami:neo-angband-tracker:" in head, "filing kit near the top of the issue", warn=True)
    rep.check(re.search(r"#+\s*Verification", body) is not None, "issue has a Verification section")
    rep.check("not been checked in the native" not in body and "not yet its own native reproduction" not in body
              and "next action: reproduce" not in body.lower(), "issue body not stale (no 'not checked natively' text)")
    pr = [l for l in labels if l.startswith("priority:")]
    rep.check(len(pr) == 1, f"exactly one priority label ({', '.join(pr) or 'none'})")
    rep.check("evidence:c-game-reproduced" in labels, "label evidence:c-game-reproduced")
    rep.check("evidence:reported-lead" not in labels, "no stale evidence:reported-lead label")
    rep.check("status:in-review" in labels, "label status:in-review")
    hits = [p for p in PLACEHOLDERS if p in body]
    rep.check(not hits, f"no placeholders in issue body ({', '.join(hits)})")
    if src_no:
        src = gh(f"pulls/{src_no}")
        sb = (src or {}).get("body") or ""
        rep.check(f"#{issue_no}" in sb, f"source PR #{src_no} links issue #{issue_no}")
        rep.check(f"#{ev_no}" in sb, f"source PR #{src_no} links evidence PR #{ev_no}")
        rep.check(re.search(r"#+\s*Verification", sb) is not None, f"source PR #{src_no} has a Verification section")
        rep.check(not [p for p in PLACEHOLDERS if p in sb], f"no placeholders in source PR #{src_no}")
    else:
        rep.check(False, "source PR found for this evidence PR")
    ev = gh(f"pulls/{ev_no}")
    eb = (ev or {}).get("body") or ""
    if src_no:
        rep.check(f"#{src_no}" in eb, f"evidence PR #{ev_no} links source PR #{src_no}")


def from_evidence_pr(ev_no):
    pr = gh(f"pulls/{ev_no}")
    files = subprocess.run(["gh", "api", f"repos/{REPO}/pulls/{ev_no}/files", "--paginate", "--jq", ".[].filename"],
                           capture_output=True, text=True).stdout.split()
    dirs = sorted({"/".join(f.split("/")[:4]) for f in files if f.startswith("docs/audit/c-reports/") and f.count("/") >= 4})
    head = pr["head"]["sha"]
    git("fetch", "-q", "--no-tags", "origin", pr["head"]["ref"], check=False)
    return pr, dirs, head


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--evidence-pr", type=int)
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--ref")
    ap.add_argument("--dir")
    ap.add_argument("--no-github", action="store_true")
    ap.add_argument("--json")
    args = ap.parse_args()
    reports, summary = [], []
    if args.ref and args.dir:
        rep = Report(f"{args.ref}:{args.dir}")
        check_package(args.ref, args.dir, rep)
        reports.append(rep)
    else:
        if args.all:
            prs = []
            page = 1
            while True:
                batch = gh(f"pulls?state=open&base=master&per_page=100&page={page}") or []
                prs += batch
                if len(batch) < 100:
                    break
                page += 1
            ev_list = [p["number"] for p in prs if p["head"]["ref"].endswith("-evidence")]
        else:
            ev_list = [args.evidence_pr]
        open_src = {}
        page = 1
        while True:
            batch = gh(f"pulls?state=open&base=c-borg-reference&per_page=100&page={page}") or []
            for p in batch:
                open_src[p["head"]["ref"]] = p["number"]
                open_src[p["head"]["sha"]] = p["number"]
            if len(batch) < 100:
                break
            page += 1
        for ev in sorted(ev_list):
            pr, dirs, head = from_evidence_pr(ev)
            for d in dirs or [None]:
                rep = Report(f"evidence #{ev} {d}")
                receipt = check_package(head, d, rep) if d else None
                if not d:
                    rep.check(False, "evidence PR adds a docs/audit/c-reports/<slug>/ directory")
                issue_no = None
                if receipt:
                    m = re.search(r"#(\d+)", str(receipt.get("issue", "")))
                    issue_no = int(m.group(1)) if m else None
                src_no = None
                if receipt:
                    src_no = open_src.get(receipt.get("fix_branch")) or open_src.get(receipt.get("fix"))
                if not src_no:
                    m = re.search(r"source PR #(\d+)", pr.get("body") or "", re.I) or re.search(r"diegoami/neo-angband-tracker#(\d+) \(source", pr.get("body") or "")
                    src_no = int(m.group(1)) if m and int(m.group(1)) in open_src.values() else None
                if not issue_no:
                    body = pr.get("body") or ""
                    m = (re.search(r"neo-angband-tracker#(\d+)", body)
                         or re.search(r"\bissue\s*#?(\d+)", body, re.I)
                         or (None if re.search(r"CBUG-\d+$", (d or "").rstrip("/"))
                             else re.search(r"-(\d+)$", (d or "").rstrip("/"))))
                    issue_no = int(m.group(1)) if m else None
                if not args.no_github:
                    check_github(issue_no, src_no, ev, rep)
                reports.append(rep)
                summary.append({"evidence_pr": ev, "dir": d, "issue": issue_no, "source_pr": src_no,
                                "failed": [w for s, w in rep.rows if s == "FAIL"],
                                "warn": [w for s, w in rep.rows if s == "WARN"]})
    for r in reports:
        r.print()
    if args.json:
        with open(args.json, "w") as fh:
            json.dump(summary, fh, indent=1)
    sys.exit(1 if any(r.failed for r in reports) else 0)


if __name__ == "__main__":
    main()
