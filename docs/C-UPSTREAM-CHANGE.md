# Upstream change: the update inventory

When anything upstream-side moves — we adopt an upstream review shape,
upstream merges or reshapes a PR, upstream pushes their own fix, or an
evidence head gets re-pinned — the change is not done when the source
branch is pushed. Every reference to the old shape is now wrong, and
they are spread across lanes that do not update each other. Work this
list top to bottom; each item has been a real trap in this repo.

## 1. Source lane

- Reshape commit on the task branch, push, note the new pinned digest
  (`git show <head>:<file> | sha256sum`).
- Hosted checks run per head: re-check the rollup before quoting CI
  status anywhere.

## 2. Evidence lane (re-pin)

- `reproduce.py`: embedded FIX literal **and** `FIXED_SOURCE_SHA256`.
- Cumulative patch file under `patches/upstream-c/` (regenerate with
  `git diff <base> <new-head> -- <file>`).
- `SOURCE-UPDATE.md`: new head, new digest, equivalence statement,
  verification `git diff` commands.
- Expanded README sections that quote the diff or name the head.
- **Replay generator narration (`make_replay.py`)**: byte-identical
  measured data hides the change — the replay page looks exactly as
  before unless the page NAMES the accepted shape. Update the narration
  and panel labels, regenerate `replay.html` from the retained run
  directory, render-check it headless.
- Re-run the matrix; commit the receipts.
- **Commit policy**: if any already-pushed doc links a pinned evidence
  SHA (`blob/<sha>/...`), prefer a NEW commit over amend/force-push —
  amending breaks those links.

## 3. Owner-facing duplicates (they are copies, not shared)

The source issue body, the source PR body, and the evidence PR body
often carry the SAME sections: the alternative-comparison table, the
"current proposal head" line, the CI snapshot, verification commands,
patch/doc links. Update **each copy separately**; fixing one does not
fix the others.

## 4. Pinned blob links

Every `blob/<sha>/docs/...` link freezes content at that sha. When the
evidence moves, re-derive every such link. Sweep for each superseded
SHA across: issue bodies, PR bodies, posted comments (issue, PR, and
review comments), README/SOURCE-UPDATE cross-links, and **drafted
upstream replies** (see 6).

## 5. CI snapshot lines

"At evidence head X, CI ... completed successfully" — refresh the head,
the status, and keep the "supersedes earlier snapshots" sentence.

## 6. Drafted upstream replies (high priority)

Replies drafted for the owner to post on upstream PRs (`/tmp/opencode/
prXXXX-reply.md`) cite our evidence links so reviewers can follow them.
If the underlying comparison/evidence is re-pinned, the draft must be
re-checked and re-pinned **before** the owner posts it — a stale link
shows the pre-change comparison as the accepted result. Re-verify:
every link target exists at the cited sha, and the text describes the
currently accepted shape.

## 7. Posted comments

- Comments posted by this session in the same cycle that claim to be
  "current status" may be edited or superseded with a correction.
- Dated historical comments are records, not state: do not rewrite
  them; supersede with a new comment or fix the body instead.

## 8. Verification gate

Before declaring the cycle done, for each superseded pin:

```sh
gh issue view <n> -R diegoami/neo-angband-tracker --json body --jq .body | grep -c <sha>
gh pr view <n>   -R diegoami/neo-angband-tracker --json body --jq .body | grep -c <sha>
```

Every count must be 0, except references explicitly marked historical.
Grep **both** forms of each SHA — bodies mix 10-char short pins and
40-char full pins of the same commit, and a full-form-only grep misses
half of them.
