#!/usr/bin/env python3
"""Render replay.html (offline, no scripts) from the recorded flowcache-404 results."""
import html
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
VARIANTS = ("before", "after", "neg-spread-only", "neg-enqueue-only", "alt-wipe")
LABEL = {"before": "Original", "after": "Corrected", "neg-spread-only": "Only spread fixed",
         "neg-enqueue-only": "Only enqueue fixed", "alt-wipe": "Wipe alternative"}
rows = {v: json.loads((root / f"{v}-results.json").read_text()) for v in VARIANTS}
key = lambda k: {52: "west (4)", 54: "east (6)", 0: "no step"}.get(k, str(k))


def card(variant):
    r = rows[variant]["caution"]
    s, o = r.get("state", {}), r.get("outcome", {})
    east = o.get("stepped_east", 0)
    return (f'<div class="card {"bad" if east else "good"}"><h3>{LABEL[variant]}</h3>'
            f'<p>Retreat chosen by <code>borg_caution()</code>: <b>{key(s.get("caution_key", 0))}</b>'
            f' ({html.escape("; ".join(n for n in r.get("notes", []) if "Desperate" in n))})</p>'
            f'<p>Cache entry for the grid beside the mold: know={s.get("know_G", "?")}</p>'
            f'<p>Next normal exploration step: <b>{key(s.get("explore_key", 0))}</b>'
            f' {"(back into the mold" + chr(39) + "s reach)" if east else "(blocked: danger is assessed)"}</p></div>')


def cell(variant, case):
    r = rows[variant][case]
    return f'<td class="{"ok" if r["passed"] else "no"}">{"pass" if r["passed"] else "FAIL"}</td>'


cases = list(rows["before"])
matrix = "".join(
    f'<tr><td>{html.escape(c)}</td><td>{rows["before"][c]["role"]}</td>'
    + "".join(cell(v, c) for v in VARIANTS) + "</tr>" for c in cases)

mixed = "".join(
    f'<tr><td>{m}</td>' + "".join(
        f'<td>{rows[v][f"costmixed-{m}"]["state"]["second_normal_calls"]}</td>' for v in VARIANTS) + "</tr>"
    for m in ("desperate", "lunal", "munchkin", "digging"))
safe = "".join(
    f'<tr><td>{m}</td>' + "".join(
        f'<td>{rows[v][f"safe-{m}-spread"]["state"]["p2_danger_calls"]}</td>' for v in VARIANTS) + "</tr>"
    for m in ("desperate", "lunal", "munchkin", "digging"))
head = "".join(f"<th>{LABEL[v]}</th>" for v in VARIANTS)
states = ""
for st in ("visible", "remembered", "expired"):
    b, a = rows["before"][f"prod-{st}"], rows["after"][f"prod-{st}"]
    states += (f'<tr><td>{st}</td><td>{b["state"].get("mold_remembered")}</td>'
               f'<td>{key(b["state"].get("explore_key", 0))}</td><td>{key(a["state"].get("explore_key", 0))}</td></tr>')

page = f'''<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Flow danger cache (flowcache-404)</title>
<style>
:root{{--bg:#fff;--fg:#1b1f24;--mut:#59636e;--ok:#1a7f37;--bad:#cf222e;--line:#d0d7de}}
@media (prefers-color-scheme:dark){{:root{{--bg:#0d1117;--fg:#e6edf3;--mut:#9198a1;--ok:#3fb950;--bad:#f85149;--line:#30363d}}}}
body{{background:var(--bg);color:var(--fg);font:16px/1.5 system-ui,sans-serif;margin:0 auto;max-width:60rem;padding:1rem}}
table{{border-collapse:collapse;width:100%;margin:.5rem 0 1.5rem;font-size:.9rem}}
td,th{{border:1px solid var(--line);padding:.25rem .5rem;text-align:left}}
.ok{{color:var(--ok)}}.no{{color:var(--bad);font-weight:600}}
.cards{{display:flex;flex-wrap:wrap;gap:1rem}}.card{{flex:1 1 15rem;border:1px solid var(--line);border-left-width:6px;padding:.5rem 1rem}}
.good{{border-left-color:var(--ok)}}.bad{{border-left-color:var(--bad)}}.mut{{color:var(--mut)}}code{{font-size:.9em}}
</style></head><body>
<h1>A retreat left a dangerous grid marked "checked"</h1>
<p>The Borg stands next to a grey mold, retreats onto the up staircase with the real <code>borg_caution()</code>,
then explores again. Native C, captured output (not a live run).</p>
<div class="cards">{"".join(card(v) for v in ("before", "after"))}</div>
<h2>What the Borg still knows about the mold</h2>
<table><tr><th>Mold state at the second step</th><th>Mold remembered</th><th>Original</th><th>Corrected</th></tr>{states}</table>
<p class="mut">Visible and remembered: the correction makes the exploring flow assess the grid again. Expired: the
Borg has forgotten the mold, its danger is now 0, and both builds step east. The correction cannot recover a forgotten monster.</p>
<h2>All cases, all builds</h2>
<table><tr><th>Case</th><th>Role</th>{head}</tr>{matrix}</table>
<p class="mut">contract = asserts the correct behaviour; limit = both builds must agree; control = behaviour unchanged;
cost = counts danger evaluations. "Only spread/enqueue fixed" are negative controls; the wipe alternative is a comparison, not the proposal.</p>
<h2>Cost: danger evaluations (borg_danger calls)</h2>
<p>Honest verdicts cached by a normal flow, then an emergency flow over new ground, then the normal flow again (evaluations in that last flow):</p>
<table><tr><th>Mode</th>{head}</tr>{mixed}</table>
<p>Normal flow right after an emergency flow over a safe corridor (evaluations in that flow; the original reuses unchecked entries):</p>
<table><tr><th>Mode</th>{head}</tr>{safe}</table>
</body></html>'''
(root / "replay.html").write_text(page)
(root / "replay-data.json").write_text(json.dumps({v: rows[v] for v in VARIANTS}, indent=1, sort_keys=True) + "\n")
print(f"wrote {root / 'replay.html'}")
