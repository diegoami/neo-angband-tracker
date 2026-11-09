import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/spasticscore-147-stacked'
TITLE="Stacked with #146: which grid the searching Borg walks to"
INTRO=("Issue #147's slip (the secret-door search scores a candidate grid with its north-west neighbour's search count) only steers the Borg once issue #146 is corrected, because on the pristine source the search can only choose the Borg's own grid. "
 "This run applies #146's correction alone, then #146's and #147's corrections together, and lets the bored Borg search and walk along a granite wall. "
 "With #146 alone, after its first search the Borg picks the grid next to it, which that search already covered, and creeps one grid at a time, searching every grid twice. "
 "With both corrections it picks a grid no search has covered yet and moves on two grids at a time. The pristine source is shown for reference: it never leaves its grid.")
CONDITIONS=("Native Human Warrior, seed 147, level 20, depth 5, alone in a closed 22x70 room with a granite north wall, no stairs or monsters, just arrived on the level (level timer 0). "
 "'wall': the Borg starts against the north wall at (35,2) and the production borg_flow_spastic(true) (the bored call) runs twelve times in a row. "
 "Control 'approach': the Borg starts at (35,5), three grids south of the wall, and the calls stop at its first search, before any search count could differ between grids. "
 "Fixture interventions: the granite north wall (the arena helper builds permanent rock); the fixture carries out each step the call queues (monster_swap); before each call it blanks the committed flow so that the grid at flow cost 0 afterwards is the grid the call chose. "
 "The room is wide so that its corners, which score higher, are beyond the search's reach at level 20. Not measured: a real dungeon level, other classes or levels, how often a Borg is bored next to a wall in play, and whether any secret door is found (in 4.2 they are found by walking next to them).")
ISSUE=147
CARD_KEY='repeat_searches'
CARD_LABEL='searches of a grid an earlier search already covered'
PHASES=(('base','Pristine source (reference)','base'),('with146','#146 only','original'),('stacked','#146 + #147','corrected'))
CASES=[('wall','Bored next to a wall: where does it walk after its first search?',
 "After searching at (35,2) the counts of (34..36, 1..3) are 5. With #146 alone the score reads each candidate's north-west neighbour: (34,2) is scored with (33,1)'s 0, so the Borg steps west onto a grid it has just covered, searches it again, and repeats: six searches, five on covered grids, in twelve calls. With both corrections (34,2) and (36,2) carry their own 5, so it walks to (37,2), which no search has covered, and continues to (39,2) and (41,2): four searches, none repeated. The pristine source searches its own grid three times and never moves.",False),
 ('approach','Control: walking to the wall, up to the first search','No grid has been searched yet, so every candidate\'s own count and its north-west neighbour\'s are both 0: both builds walk to (38,2) and search there once, and the logs are byte-identical. The pristine source finds no grid and does not move.',True)]
for mode,label,explanation,control in CASES:
 item={'label':label,'explanation':explanation,'control':control}
 for phase,_,_ in PHASES:
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':SUITE+' finished: 1/1 passed' in raw}
  for match in re.findall(r'OUTCOME (.*)',raw):
   entry.setdefault('outcome',{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
  entry['notes']=[l for l in raw.splitlines() if l.startswith(('SEARCH ','STEP ','OUTCOME '))]
  entry['value']=entry.get('outcome',{}).get(CARD_KEY,'n/a')
  item[phase]=entry
 item['before']=item['with146'];item['after']=item['stacked']
 data[mode]=item
CSS=("body{margin:0;background:#f4f1ea;color:#253a3c;font:18px/1.6 system-ui}"
 "main{max-width:1200px;margin:auto;padding:32px 16px}"
 "h1{font-size:36px;line-height:1.2}h2{font-size:22px}"
 ".cards{display:grid;grid-template-columns:repeat(3,1fr);gap:16px}"
 ".card{background:white;border:1px solid #cbd6d0;border-radius:12px;padding:20px;margin:18px 0;min-width:0}"
 ".cards .card{border-top:6px solid #648e80}.cards .base{border-top-color:#9aa5a0}.num{font-size:34px;font-weight:750}"
 ".summary{background:#e6eee6;padding:20px;border-left:5px solid #377367}"
 ".hidden{display:none}button,select{font:inherit;padding:6px 14px;margin:4px 8px 4px 0;max-width:100%}"
 "pre{white-space:pre-wrap;overflow-wrap:anywhere;font:13px/1.5 monospace;max-height:330px;overflow:auto}"
 "details{background:white;padding:20px;margin:20px 0}summary{font-weight:700;cursor:pointer}"
 "@media(max-width:800px){.cards{grid-template-columns:1fr}h1{font-size:28px}}")
def esc(s):return html.escape(str(s))
opts=''.join('<option value="%s">%s</option>'%(esc(m),esc(d['label'])) for m,d in data.items())
panels=''
for m,d in data.items():
 cards=''
 for phase,title,cls in PHASES:
  e=d[phase]
  cards+=('<section class="card %s"><h2>%s</h2><div class="num">%s</div><p>%s</p><p>Scenario check: <strong>%s</strong></p><pre>%s</pre></section>'
   %(cls,esc(title),esc(e['value']),esc(CARD_LABEL),'passed' if e['passed'] else 'failed',esc('\n'.join(e['notes']))))
 logs=''.join('<h3>%s</h3><pre>%s</pre>'%(esc(title),esc(d[phase]['raw'])) for phase,title,_ in PHASES)
 panels+=('<section class="scenario%s" data-mode="%s">'%('' if m==CASES[0][0] else ' hidden',esc(m))+
  '<p class="summary">'+esc(d['explanation'])+('<br><strong>Unchanged control:</strong> the #146-only and #146 + #147 logs are byte-identical.' if d['control'] else '')+'</p>'
  '<button class="show-decision">Show decision</button><button class="show-log">Show raw logs</button>'
  '<div class="cards decision hidden">'+cards+'</div><div class="logs hidden">'+logs+'</div></section>')
page=('<!doctype html><html lang="en"><meta charset="utf-8">'
 '<meta name="viewport" content="width=device-width,initial-scale=1">'
 '<title>'+esc(TITLE)+'</title><style>'+CSS+'</style><main>'
 '<p>Recorded native C runs: pristine source, #146 only, #146 + #147</p>'
 '<h1>'+esc(TITLE)+'</h1><p>'+esc(INTRO)+'</p>'
 '<p><label>Scenario: <select id="scenario">'+opts+'</select></label></p>'+panels+
 '<details><summary>Experiment conditions and limits</summary><p>'+esc(CONDITIONS)+'</p></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/%d">issue %d</a>; depends on <a href="https://github.com/diegoami/neo-angband-tracker/issues/146">issue 146</a>.</p>'%(ISSUE,ISSUE)+
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
