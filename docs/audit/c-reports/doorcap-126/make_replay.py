import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/doorcap-126'
TITLE='Remove an unreachable door-count test from breeder-level door closing'
INTRO="Cleanup, no behaviour change; owner decision. On levels with breeding monsters the Borg closes doors to contain them: before each step, borg_play_step() looks for an open door next to it and closes it, and notes closed doors in a list (track_door). The scan skipped every door when the list held 255 or more entries, under the comment 'Skip repeatedly closed doors'. The list has room for 100 entries and only grows while it is below that size, so the test is never true (and it counts the whole list, not the door being looked at). The change removes the test and its comment. Neither 'skip doors already listed' nor a cap is implemented."
CONDITIONS="Native Dwarf Rogue, seed 126, clevel 25, depth 5, full HP, a lantern, in a lit, open 24x40 arena. The Borg's map is seeded from the real cave; the columns from x=30 eastward are unknown, so the production borg_flow_dark(false) commits a route east and takes its first step through borg_flow_old() and borg_play_step(). 'onedoor': an open door directly north of the player. 'twodoors': open doors north and south. 'full': one door north, with the door list already at its capacity of 100 entries (recorded at other grids). 'nobreeder': one door north, not a breeder level. The first queued key, the goal grid and the door list are read back. Fixture interventions: the arena, doors and lighting are set directly, borg_grids is seeded from the cave with BORG_OKAY|BORG_MARK, borg.avoidance is set to current HP, the breeder flag (borg.near.breeder, set by borg_check_breeders() when three breeders are known) is set directly, and in 'full' the door list is filled directly."
ISSUE=126
CARD_KEY='close'
CARD_LABEL="borg_play_step() closed a door ('c' queued; 0 = stepped instead)"
CASES=[('onedoor', 'Control: breeder level, one open door north', "Both versions close the door north (goal (10,11), 'c' queued) instead of stepping east, and note it in the door list (0 to 1). Logs byte-identical.", True), ('twodoors', 'Control: breeder level, open doors north and south', 'Both versions close the last door scanned, the one south (10,13), and note it. Logs byte-identical.', True), ('full', 'Control: breeder level, door list full (100 of 100)', "The list is at its real limit of 100, below the removed test's 255: both versions still close the door north, and neither can note it (the list stays at 100). Logs byte-identical.", True), ('nobreeder', 'Control: not a breeder level, one open door north', 'Without the breeder flag the door scan is skipped and both versions step east (key 6). Logs byte-identical.', True)]
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
