import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/dangerfull-69'
TITLE='Say that grid danger always uses the full damage on purpose'
INTRO="Comment-only cleanup, no behaviour change; owner decision (option A). borg_danger() answers 'how dangerous is this grid?' for almost every Borg decision. Nearly all callers pass full_damage = false, but the function sets it to true before adding up the monsters, so every caller gets the worst-case estimate. The owner decided the override is deliberate: the reduced estimate it would unmask rests on a hit-chance formula that is tracked separately (#63). The change adds a comment saying the override is deliberate and that reduced estimates come only from direct calls to borg_danger_one_kill(). Nothing became dead code because of the override (those direct calls still use the reduced path), so no code is removed; the compiled object is identical."
CONDITIONS="Native Human Warrior at dungeon level 10, seed 69, 60% HP, a torch and rations, in a lit, open 24x40 arena with one monster due east, observed through the Borg's own observe_kill_diff() and tracked as awake. The production borg_danger() runs at the player's grid and at the grid next to the monster, with full_damage false (as its callers pass it) and true, plus once with c=4 at the player's grid; borg_danger_one_kill() is called directly with both values at the grid next to the monster for reference. 'bear': cave bear 3 grids away. 'wolf': wolf (+10 speed) 2 grids away. 'orc': cave orc (arrows) 3 grids away. 'shaman': kobold shaman (spells) 3 grids away. The scenario check requires borg_danger() to give the same answer for false and true. Fixture interventions: the arena and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_VIEW, borg.avoidance is set to current HP, and only borg_kills[k].awake is set by hand."
ISSUE=69
CARD_KEY='adj'
CARD_LABEL='borg_danger() next to the monster, as callers ask for it (full_damage false)'
CASES=[('bear', 'Control: awake cave bear, 3 grids away', "borg_danger() gives 0 at the player's grid and 20 next to the bear whether full_damage is false or true; a direct borg_danger_one_kill() call gives 0 (reduced) and 20 (full). Logs byte-identical.", True), ('wolf', 'Control: awake wolf, 2 grids away', "16 at the player's grid, 48 next to the wolf, for both argument values; the direct call gives 0 and 48. Logs byte-identical.", True), ('orc', 'Control: cave orc with arrows, 3 grids away', '13 and 22 for both argument values; the direct call gives 5 and 22. Logs byte-identical.', True), ('shaman', 'Control: kobold shaman (spells), 3 grids away', '22 and 22 for both argument values; the direct call gives 8 and 22. Logs byte-identical.', True)]
for mode,label,explanation,control in CASES:
 item={'label':label,'explanation':explanation,'control':control}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':SUITE+' finished: 1/1 passed' in raw}
  for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
   for match in re.findall(re.escape(tag)+r'(.*)',raw):
    entry.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
  notes=[l for l in raw.splitlines() if l.startswith(('PREPARED ','SWAP ','DECISION ','TRACK ','KEYS '))]
  entry['notes']=notes
  entry['value']=entry.get('outcome',{}).get(CARD_KEY,entry.get('state',{}).get(CARD_KEY,'n/a'))
  item[phase]=entry
 data[mode]=item
CSS=("body{margin:0;background:#f4f1ea;color:#253a3c;font:18px/1.6 system-ui}"
 "main{max-width:1100px;margin:auto;padding:32px 24px}"
 "h1{font-size:38px;line-height:1.2}h2{font-size:24px}"
 ".cards{display:grid;grid-template-columns:1fr 1fr;gap:20px}"
 ".card{background:white;border:1px solid #cbd6d0;border-radius:12px;padding:24px;margin:18px 0}"
 ".cards .card{border-top:6px solid #648e80}.num{font-size:34px;font-weight:750}"
 ".summary{background:#e6eee6;padding:20px;border-left:5px solid #377367}"
 ".hidden{display:none}button,select{font:inherit;padding:6px 14px;margin:4px 8px 4px 0}"
 "pre{white-space:pre-wrap;overflow-wrap:anywhere;font:13px/1.5 monospace;max-height:330px;overflow:auto}"
 "details{background:white;padding:20px;margin:20px 0}summary{font-weight:700;cursor:pointer}"
 "@media(max-width:650px){.cards{grid-template-columns:1fr}h1{font-size:28px}}")
def esc(s):return html.escape(str(s))
opts=''.join('<option value="%s">%s</option>'%(esc(m),esc(d['label'])) for m,d in data.items())
panels=''
for m,d in data.items():
 b,a=d['before'],d['after']
 panels+=('<section class="scenario%s" data-mode="%s">'%('' if m==CASES[0][0] else ' hidden',esc(m))+
  '<p class="summary">'+esc(d['explanation'])+('<br><strong>Unchanged control:</strong> both logs are byte-identical.' if d['control'] else '')+'</p>'
  '<button class="show-decision">Show decision</button><button class="show-log">Show raw logs</button>'
  '<div class="cards decision hidden">'
  '<section class="card original"><h2>Original</h2><div class="num">'+esc(b['value'])+'</div><p>'+esc(CARD_LABEL)+'</p><p>Scenario check: <strong>'+('passed' if b['passed'] else 'failed')+'</strong></p><pre>'+esc('\n'.join(b['notes']))+'</pre></section>'
  '<section class="card corrected"><h2>Corrected</h2><div class="num">'+esc(a['value'])+'</div><p>'+esc(CARD_LABEL)+'</p><p>Scenario check: <strong>'+('passed' if a['passed'] else 'failed')+'</strong></p><pre>'+esc('\n'.join(a['notes']))+'</pre></section>'
  '</div><div class="logs hidden"><h3>Original</h3><pre>'+esc(b['raw'])+'</pre><h3>Corrected</h3><pre>'+esc(a['raw'])+'</pre></div></section>')
page=('<!doctype html><html lang="en"><meta charset="utf-8">'
 '<meta name="viewport" content="width=device-width,initial-scale=1">'
 '<title>'+esc(TITLE)+'</title><style>'+CSS+'</style><main>'
 '<p>Recorded native C runs, original and corrected source</p>'
 '<h1>'+esc(TITLE)+'</h1><p>'+esc(INTRO)+'</p>'
 '<p><label>Scenario: <select id="scenario">'+opts+'</select></label></p>'+panels+
 '<details><summary>Experiment conditions and limits</summary><p>'+esc(CONDITIONS)+'</p></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/%d">issue %d</a>.</p>'%(ISSUE,ISSUE)+
 '</main><script>'
 'var sel=document.getElementById("scenario");'
 'function show(){var m=sel.value;document.querySelectorAll(".scenario").forEach(function(s){s.classList.toggle("hidden",s.getAttribute("data-mode")!==m);});}'
 'sel.addEventListener("change",show);'
 'document.querySelectorAll(".show-decision").forEach(function(b){b.addEventListener("click",function(){b.parentNode.querySelector(".decision").classList.remove("hidden");});});'
 'document.querySelectorAll(".show-log").forEach(function(b){b.addEventListener("click",function(){b.parentNode.querySelector(".logs").classList.remove("hidden");});});'
 'show();</script></html>')
(root/'replay.html').write_text(page)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
