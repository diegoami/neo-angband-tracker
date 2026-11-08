#!/usr/bin/env python3
"""Render play.html: a self-contained, offline page that steps through frames
recorded from the real game code (see record-play.py).  Nothing on the page
runs the game or simulates it; it replays recorded state.

usage: make_play.py <directory containing play-data.json>
"""
import json
import sys
from pathlib import Path

root = Path(sys.argv[1])
data = json.loads((root / "play-data.json").read_text())
payload = json.dumps(data["frames"], separators=(",", ":")).replace("</", "<\\/")
meta = {"base": data["base"], "fix": data["fix"], "fixtures_ref": data["fixtures_ref"]}

PAGE = r'''<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Play the flow-cache fault</title>
<style>
:root{--bg:#fff;--fg:#1b1f24;--mut:#59636e;--line:#d0d7de;--card:#f6f8fa;--wall:#57606a;--unk:#e6e9ee;--floor:#fff;
--ok:#1a7f37;--bad:#cf222e;--acc:#0969da;--warn:#bc4c00}
@media (prefers-color-scheme:dark){:root{--bg:#0d1117;--fg:#e6edf3;--mut:#9198a1;--line:#30363d;--card:#161b22;--wall:#3b434c;--unk:#1c2128;--floor:#0d1117;
--ok:#3fb950;--bad:#f85149;--acc:#58a6ff;--warn:#d29922}}
*{box-sizing:border-box}
body{background:var(--bg);color:var(--fg);font:16px/1.5 system-ui,-apple-system,Segoe UI,sans-serif;margin:0;padding:0 16px 48px}
main{max-width:64rem;margin:0 auto}
h1{font-size:1.5rem;margin:1.25rem 0 .25rem}
h2{font-size:1.05rem;margin:1.25rem 0 .5rem}
p{margin:.4rem 0}
.mut{color:var(--mut)}
.box{background:var(--card);border:1px solid var(--line);border-radius:8px;padding:.75rem 1rem;margin:.75rem 0}
fieldset{border:0;margin:0 0 .5rem;padding:0}
legend{font-weight:600;padding:0;margin-bottom:.25rem}
.seg{display:flex;flex-wrap:wrap;gap:.4rem}
.seg label{border:1px solid var(--line);border-radius:6px;padding:.3rem .7rem;cursor:pointer;background:var(--bg)}
.seg input{position:absolute;opacity:0;pointer-events:none}
.seg input:checked+span{font-weight:700;color:var(--acc)}
.seg label:has(input:checked){border-color:var(--acc);box-shadow:0 0 0 1px var(--acc)}
.seg label:has(input:focus-visible){outline:2px solid var(--acc);outline-offset:2px}
button{font:inherit;border:1px solid var(--line);background:var(--bg);color:var(--fg);border-radius:6px;padding:.35rem .9rem;cursor:pointer}
button:focus-visible{outline:2px solid var(--acc);outline-offset:2px}
button:disabled{opacity:.45;cursor:default}
.bar{display:flex;flex-wrap:wrap;gap:.5rem;align-items:center;margin:.75rem 0}
.boards{display:grid;grid-template-columns:1fr 1fr;gap:1rem}
@media (max-width:720px){.boards{grid-template-columns:1fr}}
.pane{border:1px solid var(--line);border-radius:8px;padding:.6rem .75rem;background:var(--card)}
.pane h3{margin:0 0 .3rem;font-size:1rem}
.pane.orig h3{color:var(--bad)}.pane.fix h3{color:var(--ok)}
svg{width:100%;height:auto;display:block;background:var(--bg);border:1px solid var(--line);border-radius:6px}
.cap{min-height:6.5em;margin:.5rem 0 0;font-size:.95rem}
.verdict{font-weight:600}
.legend{display:flex;flex-wrap:wrap;gap:.35rem 1.1rem;font-size:.85rem;color:var(--mut)}
.legend b{color:var(--fg)}
code{font-size:.9em}
a{color:var(--acc)}
.step{font-weight:600}
</style></head><body><main>
<h1>Play the flow-cache fault</h1>
<p>The Borg remembers which squares it has already danger-checked, so it does not repeat the work. In its emergency modes it
<b>skips</b> the check, but the original game still wrote "checked". Step through what the game actually did, in the original
and with the one-file correction. The grey mold is stationary; it hurts anything standing next to it.</p>
<p class="mut">Every frame was captured from the real game code (the real <code>borg_caution()</code>, <code>borg_flow_dark()</code>,
<code>borg_flow_spread()</code>, <code>borg_flow_enqueue_grid()</code> and <code>borg_update()</code>). This page replays the
recorded state; it does not run the game and nothing here is simulated. This is one self-contained file: download it and open it in any browser, no network needed. Synthetic parts: the corridor, the mold, the Borg's 6 HP and
the unexplored end of the corridor were placed by the test fixture.</p>

<div class="box">
<fieldset><legend>Scene</legend><div class="seg" id="scene">
<label><input type="radio" name="scene" value="walk" checked><span>Retreat, then explore again</span></label>
<label><input type="radio" name="scene" value="modes"><span>Four emergency modes</span></label></div></fieldset>
<fieldset id="walkopts"><legend>What does the Borg remember about the mold when it explores again?</legend><div class="seg">
<label><input type="radio" name="state" value="visible" checked><span>It can see it</span></label>
<label><input type="radio" name="state" value="remembered"><span>Not seen for 60 turns, still remembered</span></label>
<label><input type="radio" name="state" value="expired"><span>Forgotten (not seen for 2500 turns)</span></label></div></fieldset>
<fieldset id="modeopts" hidden><legend>Emergency mode, and how the square gets into the flow</legend><div class="seg">
<label><input type="radio" name="mode" value="desperate" checked><span>desperate (retreat)</span></label>
<label><input type="radio" name="mode" value="lunal"><span>lunal</span></label>
<label><input type="radio" name="mode" value="munchkin"><span>munchkin</span></label>
<label><input type="radio" name="mode" value="digging"><span>digging</span></label></div>
<div class="seg" style="margin-top:.4rem">
<label><input type="radio" name="entry" value="spread" checked><span>flow spreads over the squares</span></label>
<label><input type="radio" name="entry" value="enqueue"><span>square is a flow's starting point</span></label></div></fieldset>
</div>

<div class="bar" role="group" aria-label="Playback">
<button id="prev">&#9664; Previous</button><button id="next">Next &#9654;</button>
<button id="play">Play</button><button id="reset">Start over</button>
<span class="step" id="stepno" aria-live="polite"></span>
</div>
<h2 id="title"></h2>
<div class="boards">
<section class="pane orig" aria-labelledby="ot"><h3 id="ot">Original game</h3><div id="bo"></div><p class="cap" id="co" aria-live="polite"></p></section>
<section class="pane fix" aria-labelledby="ft"><h3 id="ft">With the correction</h3><div id="bf"></div><p class="cap" id="cf" aria-live="polite"></p></section>
</div>
<p class="verdict" id="verdict"></p>

<div class="box legend" aria-label="Legend">
<span><b>@</b> the Borg</span><span><b>&lt;</b> up staircase</span><span><b>m</b> grey mold (dashed: forgotten)</span>
<span><b>&#10003;</b> noted as "already checked"</span><span><b>&#10007;</b> refused: too dangerous</span>
<span><b style="color:var(--bad)">red ring</b> noted "checked" but dangerous and never checked</span>
<span><b style="color:var(--acc)">&#9679;</b> danger really computed this step</span>
<span>bottom right: danger (<b id="lim">red = over the limit</b>)</span><span>bottom left: flow cost</span><span><b>?</b> unexplored</span>
</div>

<p class="mut">Pins: base <code>__BASE__</code>, correction <code>__FIX__</code>, recorder fixtures <code>__FIX_REF__</code>.
Regenerate with <code>record-play.py</code> (same directory). The case matrix, cost and limits are in <a href="README.md">README.md</a>.
This page does not show that any life survives because of the correction.</p>
</main>
<script type="application/json" id="data">__DATA__</script>
<script>
"use strict";
const D = JSON.parse(document.getElementById("data").textContent);
const X0 = 10, Y0 = 6, W = 11, H = 3, S = 56, MX = 15, MY = 8;
const $ = id => document.getElementById(id);
const radio = n => document.querySelector('input[name="' + n + '"]:checked').value;
const KEY = {52: "west (key 4)", 54: "east (key 6)", 0: "no step"};
let step = 0, timer = null;

function sceneId() { return radio("scene") === "walk" ? "walk-" + radio("state") : "modes-" + radio("mode") + "-" + radio("entry"); }
function frames(v) { return D[v][sceneId()]; }
function idx(x, y) { return (y - Y0) * W + (x - X0); }
function esc(s) { return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;"); }

function stats(f) {
  let known = 0, suspicious = 0, blocked = 0;
  for (let i = 0; i < f.know.length; i++) {
    if (f.terrain[i] !== 1 && f.terrain[i] !== 2) continue;
    if (f.know[i] && f.icky[i]) blocked++;
    else if (f.know[i]) { known++; if (f.danger[i] > f.limit) suspicious++; }
  }
  return {known, suspicious, blocked};
}

function board(f, showCost) {
  const st = f.step, evald = {};
  f.evaluated.forEach(e => { evald[e[0] + "," + e[1]] = e[2]; });
  let o = '<svg viewBox="0 0 ' + W * S + " " + H * S + '" role="img" aria-label="Corridor map, step ' + (st + 1) + '">';
  for (let y = Y0; y < Y0 + H; y++) for (let x = X0; x < X0 + W; x++) {
    const i = idx(x, y), t = f.terrain[i], px = (x - X0) * S, py = (y - Y0) * S;
    const know = f.know[i], icky = f.icky[i], dg = f.danger[i];
    const open = t === 1 || t === 2;
    const fill = t === 3 ? "var(--wall)" : t === 0 ? "var(--unk)" : "var(--floor)";
    const sus = open && know && !icky && dg > f.limit;
    let tip = "(" + x + "," + y + ")";
    if (open) tip += " danger " + dg + (dg > f.limit ? " (over the limit " + f.limit + ")" : "");
    if (know && icky) tip += "; refused as too dangerous";
    else if (know) tip += "; noted as already checked" + (sus ? " but it was never checked and is dangerous" : "");
    if (evald[x + "," + y] !== undefined) tip += "; danger computed this step: " + evald[x + "," + y];
    o += "<g><title>" + esc(tip) + "</title><rect x=\"" + px + "\" y=\"" + py + "\" width=\"" + S + "\" height=\"" + S +
      "\" fill=\"" + fill + "\" stroke=\"var(--line)\"/>";
    if (sus) o += "<rect x=\"" + (px + 2) + "\" y=\"" + (py + 2) + "\" width=\"" + (S - 4) + "\" height=\"" + (S - 4) +
      "\" fill=\"none\" stroke=\"var(--bad)\" stroke-width=\"3\"/>";
    if (t === 0) o += "<text x=\"" + (px + S / 2) + "\" y=\"" + (py + S / 2 + 6) + "\" text-anchor=\"middle\" font-size=\"18\" fill=\"var(--mut)\">?</text>";
    if (t === 2) o += "<text x=\"" + (px + S / 2) + "\" y=\"" + (py + S / 2 + 8) + "\" text-anchor=\"middle\" font-size=\"26\" fill=\"var(--acc)\">&lt;</text>";
    if (open && know) o += "<text x=\"" + (px + 5) + "\" y=\"" + (py + 15) + "\" font-size=\"17\" font-weight=\"700\" fill=\"" +
      (icky ? "var(--bad)" : "var(--ok)") + "\">" + (icky ? "&#10007;" : "&#10003;") + "</text>";
    if (evald[x + "," + y] !== undefined) o += "<text x=\"" + (px + S - 6) + "\" y=\"" + (py + 14) + "\" text-anchor=\"end\" font-size=\"13\" fill=\"var(--acc)\">&#9679;</text>";
    if (showCost && open && f.cost && f.cost[i] < 250) o += "<text x=\"" + (px + 5) + "\" y=\"" + (py + S - 5) + "\" font-size=\"12\" fill=\"var(--mut)\">" + f.cost[i] + "</text>";
    if (open && dg >= 0) o += "<text x=\"" + (px + S - 5) + "\" y=\"" + (py + S - 5) + "\" text-anchor=\"end\" font-size=\"14\" font-weight=\"700\" fill=\"" +
      (dg > f.limit ? "var(--bad)" : "var(--mut)") + "\">" + dg + "</text>";
    o += "</g>";
  }
  const mx = (MX - X0) * S + S / 2, my = (MY - Y0) * S + S / 2;
  o += "<g><title>" + (f.mold_known ? "Grey mold (the Borg knows about it)" : "The Borg has forgotten the grey mold; it is still there") + "</title><circle cx=\"" + mx + "\" cy=\"" + my + "\" r=\"15\" fill=\"var(--bg)\" stroke=\"var(--warn)\" stroke-width=\"2\"" +
    (f.mold_known ? "" : " stroke-dasharray=\"4 3\"") + "/><text x=\"" + mx + "\" y=\"" + (my + 6) + "\" text-anchor=\"middle\" font-size=\"18\" font-weight=\"700\" fill=\"var(--warn)\">m</text></g>";
  const bx = (f.borg[0] - X0) * S + S / 2, by = (f.borg[1] - Y0) * S + S / 2;
  o += "<text x=\"" + bx + "\" y=\"" + (by + 8) + "\" text-anchor=\"middle\" font-size=\"26\" font-weight=\"700\" fill=\"var(--fg)\">@</text></svg>";
  return o;
}

function plural(n, w) { return n + " " + w + (n === 1 ? "" : "s"); }

function caption(v, f, sc) {
  const s = stats(f), corrected = v === "after";
  if (sc.startsWith("walk-")) {
    const state = sc.slice(5);
    if (f.step === 0) return "HP " + f.chp + ": the Borg refuses any square with danger above " + f.limit + ". The mold makes the three corridor squares next to it dangerous (16).";
    if (f.step === 1) {
      const notes = f.notes.filter(n => /Desperate/.test(n)).map(n => n.replace(/^# /, "")).join("; ");
      return "<code>borg_caution()</code> chooses “" + esc(notes || "retreat") + "” and steps " + KEY[f.key] + ". Retreating skips the danger check. " +
        (s.known ? plural(s.known, "square") + " are now noted “checked”" + (s.suspicious ? " — " + s.suspicious + " of them dangerous and never checked." : ".")
                 : "No square is noted as checked: nothing was checked, so nothing is claimed.");
    }
    if (f.step === 2) return {visible: "The Borg stands on the staircase and can see the mold.",
      remembered: "The Borg has not seen the mold for 60 turns but still remembers it (its record is kept: the mold never moves).",
      expired: "2500 turns have passed. The Borg has forgotten the mold, which is still there."}[state] +
      " Its notes about the squares are unchanged" + (f.wipe_pending ? " (a forget-everything flag is set)." : ".");
    if (f.ok && f.key === 54) {
      const bad = stats(f).suspicious;
      return "<code>borg_flow_dark()</code> plans a walk " + KEY[f.key] + ", toward the mold." + (bad
        ? " It trusts the “checked” notes and never computes the danger of the " + plural(bad, "square") + " it passes (ringed)."
        : " This time the squares really are safe for the Borg (it remembers nothing dangerous), so stepping east is right.");
    }
    return "<code>borg_flow_dark()</code> finds no step. It computed the danger of the squares in its way (blue dots), found them over the limit and marked " +
      plural(s.blocked, "square") + " refused.";
  }
  const mode = sc.split("-")[1], entry = sc.split("-")[2];
  if (f.step === 0) return "Same corridor. Next the Borg runs a flow in " + mode + " mode, then a normal flow.";
  if (f.step === 1) return "In " + mode + " mode the danger check is skipped (" + (entry === "spread" ? "flow spreading from the staircase" : "the dangerous square is the flow's starting point") +
    "). " + (s.known ? plural(s.known, "square") + " noted “checked”" + (s.suspicious ? ", " + s.suspicious + " of them dangerous and never checked." : ".") : "Nothing is noted as checked.");
  return entry === "spread"
    ? (f.ok ? "A normal flow from the unexplored end finds a route to the staircase straight through the dangerous squares: it trusted the notes."
            : "A normal flow computes the danger of the first square it reaches, refuses it, and finds no route.")
    : (f.ok ? "A normal flow accepts the dangerous square as a start without computing its danger."
            : "A normal flow computes the danger of the square and refuses to start there.");
}

function verdict(sc, fo, ff) {
  if (!fo || !ff) return "";
  const last = (sc.startsWith("walk-") ? 3 : 2);
  if (fo.step !== last) return "";
  if (sc === "walk-expired") return "Both builds step east: the Borg has forgotten the mold, so there is nothing left to detect. The correction cannot restore a forgotten monster.";
  const bad = sc.startsWith("walk-") ? (fo.ok && fo.key === 54) : fo.ok;
  const good = sc.startsWith("walk-") ? !(ff.ok && ff.key === 54) : !ff.ok;
  return (bad ? "Original: went through a square it never checked. " : "Original: no unsafe step. ") + (good ? "Corrected: assessed the danger and did not." : "Corrected: still went through.");
}

function show() {
  const sc = sceneId(), O = frames("before"), F = frames("after");
  step = Math.max(0, Math.min(step, O.length - 1));
  const fo = O[step], ff = F[step], cost = fo.step > 0;
  $("bo").innerHTML = board(fo, cost); $("bf").innerHTML = board(ff, cost);
  $("co").innerHTML = caption("before", fo, sc); $("cf").innerHTML = caption("after", ff, sc);
  const titles = sc.startsWith("walk-")
    ? ["The Borg is beside the mold", "It retreats to the staircase", "It stands on the staircase", "It explores again"]
    : ["The corridor", "An emergency flow runs", "A normal flow runs"];
  $("title").textContent = titles[step];
  $("lim").textContent = "red = over the limit of " + fo.limit;
  $("stepno").textContent = "Step " + (step + 1) + " of " + O.length;
  $("verdict").textContent = verdict(sc, fo, ff);
  $("prev").disabled = step === 0; $("next").disabled = step === O.length - 1;
  if (step === O.length - 1) stop();
}
function stop() { if (timer) { clearInterval(timer); timer = null; } $("play").textContent = "Play"; }
function play() {
  if (timer) { stop(); return; }
  if (step >= frames("before").length - 1) step = 0;
  show(); $("play").textContent = "Pause";
  timer = setInterval(() => { step++; show(); }, 2600);
}
function sync() {
  const walk = radio("scene") === "walk";
  $("walkopts").hidden = !walk; $("modeopts").hidden = walk;
  stop(); step = 0; show();
}
document.querySelectorAll("input").forEach(i => i.addEventListener("change", sync));
$("prev").onclick = () => { stop(); step--; show(); };
$("next").onclick = () => { stop(); step++; show(); };
$("play").onclick = play;
$("reset").onclick = () => { stop(); step = 0; show(); };
document.addEventListener("keydown", e => {
  if (e.target.tagName === "INPUT" || e.target.tagName === "BUTTON") return;
  if (e.key === "ArrowRight") { stop(); step++; show(); } else if (e.key === "ArrowLeft") { stop(); step--; show(); }
});
sync();
</script></body></html>'''

page = (PAGE.replace("__DATA__", payload).replace("__BASE__", meta["base"]).replace("__FIX__", meta["fix"])
        .replace("__FIX_REF__", meta["fixtures_ref"]))
(root / "play.html").write_text(page)
print(f"wrote {root / 'play.html'} ({len(page)} bytes)")
