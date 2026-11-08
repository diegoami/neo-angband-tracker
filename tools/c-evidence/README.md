# tools/c-evidence: native C fix evidence packages

Committed generators and checks for the C-FIX-POSTING standard
(docs/C-FIX-POSTING.md). Paths: `CEV_WORK` (scratch work directory, default
`/tmp/c-evidence-work`), `CEV_REPO` (tracker clone, default this checkout),
`CEV_DEVSRC` (a scratch C source worktree for fixture iteration, never a
pristine reference), `CEV_BUILD` (its build directory), `CEV_BORG_TXT`
(profile borg.txt; take it from an existing evidence package).

| Tool | Use |
| --- | --- |
| `check_evidence.py` | **Run before opening or updating any pair.** `--evidence-pr N` (package + issue + both PRs) or `--all`; `--ref REF --dir DIR` for a package only. Fails on missing generators/artifacts/logs, receipt hash mismatches, nonexistent pins, no byte-identical control, missing gates or render check, placeholders, missing filing kit or Verification, missing cross-links, inconsistent labels. |
| `gen.py SPEC OUT` | Writes `run-cases.py`, `make_replay.py`, `reproduce.py` from a spec JSON (slug, issue, prefix, observer, suite, changes = exact old/new hunks per file, modes with expected before/after pass and `control` flags, replay texts, claim, limits). |
| `devbuild.sh` / `devrun.sh` | Iterate a fixture against `CEV_DEVSRC` before publishing. |
| `mkev.sh SLUG MSGFILE` | Creates `task/SLUG-evidence` from master with the generators, commits, pushes, extracts the pinned reproduce.py. |
| `finish.sh SLUG FIXTURES_SHA FIX_SHA` | After a green `reproduce.py --full-tests` run: README (`mkreadme.py`), receipt with sha256 of every file + headless render check (`package.py`, `render-check.js`), commits **with `git add -f`** (logs), pushes. |

Definition of done for a pair: `check_evidence.py --evidence-pr N` passes,
the issue opens in plain language with the owner filing kit near the top and
a Verification section, both PRs cross-link, and the #207 receipt is posted.
