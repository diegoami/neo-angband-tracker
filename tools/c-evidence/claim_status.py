#!/usr/bin/env python3
"""Read-only claim (lease) status for issues worked by parallel sessions.

Protocol (writes are done by the session with the GitHub MCP tools):
  claim    : add label `status:in-progress` and post a comment whose first line is
             `CLAIM by <session> at <UTC ISO time>, expires <UTC ISO time>`
  renew    : post `HEARTBEAT by <session> at <UTC ISO time>, expires <UTC ISO time>`
             (at least every few hours while working; a new expiry each time)
  release  : post `RELEASE by <session> at <UTC ISO time>: <outcome>` and remove
             the `status:in-progress` label
  takeover : allowed only when the latest CLAIM/HEARTBEAT has expired; post
             `CLAIM by <new session> ... (taking over expired claim of <old>)`

A claim decays on its own: once its newest CLAIM/HEARTBEAT is past its expiry
(default lease 6 hours when no expiry is written), the issue is free again even if
the label is still present.  An issue with an open PR that names it is never free.

Usage:
  claim_status.py 197 198              # status of issues
  claim_status.py --candidates         # free candidate leads (upstream-c + reported-lead)
  claim_status.py --stale              # expired claims whose label should be cleared
Exit status: 0 if every named issue is FREE, 1 otherwise.
"""
import argparse, datetime as dt, json, re, subprocess, sys

REPO = "diegoami/neo-angband-tracker"
LABEL = "status:in-progress"
LEASE_HOURS = 6
LINE = re.compile(r"^(CLAIM|HEARTBEAT|RELEASE) by (?P<who>.+?) at (?P<at>\S+?)(?:,? expires (?P<exp>\S+?))?(?:[:\s].*)?$")


def gh(path, pages=True):
    """GET with manual paging (the session proxy rejects --paginate's links)."""
    items, page = [], 1
    while True:
        sep = "&" if "?" in path else "?"
        url = f"repos/{REPO}/{path}{sep}per_page=100&page={page}" if pages else f"repos/{REPO}/{path}"
        r = subprocess.run(["gh", "api", url], capture_output=True, text=True)
        if r.returncode:
            raise SystemExit(f"gh api failed for {path}: {r.stderr.strip()}")
        data = json.loads(r.stdout or "null")
        if not pages or not isinstance(data, list):
            return data
        items += data
        if len(data) < 100:
            return items
        page += 1


def parse_time(s):
    try:
        return dt.datetime.fromisoformat(s.rstrip(".,").replace("Z", "+00:00"))
    except ValueError:
        return None


def status(issue_no, now, open_prs):
    issue = gh(f"issues/{issue_no}", pages=False)
    labels = [l["name"] for l in issue.get("labels", [])]
    if issue.get("state") != "open":
        return "CLOSED", ""
    linked = [p["number"] for p in open_prs if re.search(rf"#{issue_no}\b", (p.get("body") or "") + " " + p["title"])]
    if linked:
        return "HAS-PR", f"open PR(s) {linked}"
    last = None
    for c in gh(f"issues/{issue_no}/comments"):
        first = (c.get("body") or "").strip().splitlines()[0] if c.get("body") else ""
        m = LINE.match(first)
        legacy = re.match(r"^\**Claimed by (?P<who>[^*]+?)\**(?: for| \()", first)
        if legacy and not m:
            # older free-form claims: lease runs from the comment time
            last = ("CLAIM", legacy.group("who").strip(), parse_time(c["created_at"]), None, c["created_at"])
            continue
        if re.match(r"^[#*\s]*(Triage|Native triage|Native consequence triage|Owner decision|Owner-intent)", first, re.I) and last and last[0] != "RELEASE":
            # a triage/decision comment by the claimant ends the work
            last = ("RELEASE", last[1], parse_time(c["created_at"]), None, c["created_at"])
            continue
        if m:
            last = (m.group(1), m.group("who"), parse_time(m.group("at")) or parse_time(c["created_at"]),
                    parse_time(m.group("exp") or "") if m.group("exp") else None, c["created_at"])
    if not last or last[0] == "RELEASE":
        note = f"label {LABEL} still set" if LABEL in labels else ""
        return "FREE", note
    kind, who, at, exp, _ = last
    exp = exp or (at + dt.timedelta(hours=LEASE_HOURS))
    if exp <= now:
        return "EXPIRED", f"last {kind} by {who} expired {exp.isoformat()}" + (f"; label {LABEL} still set" if LABEL in labels else "")
    return "CLAIMED", f"by {who} until {exp.isoformat()}"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("issues", nargs="*", type=int)
    ap.add_argument("--candidates", action="store_true")
    ap.add_argument("--stale", action="store_true")
    args = ap.parse_args()
    now = dt.datetime.now(dt.timezone.utc)
    open_prs = gh("pulls?state=open")
    targets = list(args.issues)
    if args.candidates:
        targets += [i["number"] for i in gh("issues?state=open&labels=upstream-c,evidence:reported-lead")
                    if "pull_request" not in i]
    if args.stale:
        targets += [i["number"] for i in gh(f"issues?state=open&labels={LABEL}") if "pull_request" not in i]
    ok = True
    for n in sorted(set(targets)):
        st, why = status(n, now, open_prs)
        if args.candidates and st != "FREE":
            continue
        if args.stale and st not in ("EXPIRED", "FREE", "HAS-PR", "CLOSED"):
            continue
        print(f"#{n}: {st}{' — ' + why if why else ''}")
        ok = ok and st == "FREE"
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
