import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/teleotier-176'
TITLE='Remove the unreachable fail-rate tier from the Teleport Other defence'
INTRO="Cleanup, no behaviour change; owner decision (option C). When the Borg considers teleporting a monster away, borg_defend_aux_tele_away() accepts less spell failure the more dangerous the situation is: 50% by default, 45% when the danger p1 is above twice the Borg's avoidance figure, 40% above three times. A third tier (+5, 'a little scary') sits in the else of the 'twice' test but asks for p1 above five halves of the avoidance, which a danger that is not above twice the avoidance never is. The change deletes the dead tier."
CONDITIONS="Native Human Mage, seed 176, clevel 16, depth 15, full HP (82) and SP, a torch, rations and only the [Magical Defences] book with only Teleport Other learned (spell_learn()), in a lit, open 24x40 arena with an awake hill giant 3 grids east, observed through observe_kill_diff(). The player is afraid (TMD_AFRAID set), which adds 20 to the failure rate and puts Teleport Other at 41%, between the 40% and 45% allowances, so the tiers decide the outcome. The production borg_defend(p1) runs with p1 set to a multiple of borg.avoidance (82): 'over3' 3.5x, 'upper' 2.75x (the band the dead test names), 'lower' 2.25x, 'under2' 1.5x; the queued keys are read back. Fixture interventions: the arena and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_VIEW, borg.avoidance is set to current HP, HP/SP are set to maximum, fear is set directly, borg_kills[k].awake is set by hand and p1 is passed directly instead of being computed by the caller."
ISSUE=176
CARD_KEY='cast'
CARD_LABEL='Teleport Other cast by borg_defend() (1 = cast and aimed; 0 = refused)'
CASES=[('over3', 'Control: danger 3.5x avoidance (allowance 40%)', 'Above three times the avoidance the allowance is 40%; Teleport Other fails 41% of the time, so both versions refuse it and borg_defend() does nothing. Logs byte-identical.', True), ('upper', 'Control: danger 2.75x avoidance, the band the dead test names (allowance 45%)', "2.75x is above five halves, the condition of the deleted tier, but the 'twice' test catches it first in both versions: allowance 45%, Teleport Other cast and aimed. Logs byte-identical.", True), ('lower', 'Control: danger 2.25x avoidance (allowance 45%)', 'Allowance 45%: cast in both versions. Logs byte-identical.', True), ('under2', 'Control: danger 1.5x avoidance (allowance 50%)', 'Not above twice the avoidance: the original evaluates the dead test (false) and keeps the default 50%; the changed version keeps 50% directly. Cast in both. Logs byte-identical.', True)]
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
