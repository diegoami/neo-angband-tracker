import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/flowkillhall-98'
TITLE='One skipped monster stopped the Borg hunting any monster'
INTRO="When the Borg looks for a monster to walk up to and fight, it goes through every monster it knows about and drops the unsuitable ones. The note that a monster should be dropped was never cleared for the next monster, so once one monster was dropped every monster after it in the list was dropped too. A level-20 Warrior with one soldier ant two steps away (dropped by design: stepping closer lets it bite first) and a second soldier ant in plain view five steps away hunted neither and stood still. A second check, meant to keep the Borg in a corridor rather than chase a pack monster out in an open room, counted walls on top of the Borg's own count and so never fired; the Borg left its corridor for a wild dog in the open room. The correction judges each monster on its own: the Borg walks toward the second ant, and stays in its corridor instead of chasing the dog."
CONDITIONS="Native Human Warrior, seed 98, level 20, depth 5, full hit points (247), torch and rations, a lit 38x22 room. Corridor cases fill columns 1-14 with granite except row 12, so the Borg at (10,12) stands in a one-wide corridor that opens into the room at column 15. Monsters are placed with t_add_monster (single monsters; the wild dog's pack is not generated) and noted by the Borg's own observe_kill_diff in the stated order, which fixes their order in the Borg's monster list. The production borg_flow_kill(true, 250) runs once and the first queued key is read back. This isolates the hunt decision; it does not play out the walk or the fight."
ISSUE=98
CARD_KEY='flowed'
CARD_LABEL='sets off toward a monster to fight it (1 = yes)'
CASES=[('leak', 'Open room: ant 2 steps east noted first, ant 5 steps south second', 'Original: the near ant is dropped (two steps away, melee only), the drop note is kept, the far ant is dropped too; no target, the Borg does not move. Corrected: the far ant at (10,17) is the target and the Borg steps south (key 50).', False), ('reverse', 'Control: the same two ants, far one noted first', 'The far ant is judged before the near one sets the note; both versions target it and step south. Logs are byte-identical.', True), ('hall', 'Borg in a corridor, wild dog (pack monster) in the open room', "Original: the dog's wall count continues from the Borg's own six corridor walls and the 'fewer than four walls' test never fires; the Borg steps east out of the corridor toward the dog (key 54). Corrected: the dog has no walls around it, it is dropped, and the Borg stays put.", False), ('hallctl', 'Control: Borg and wild dog both in the corridor', 'The dog has six walls around it, so both versions keep it and step west toward it (key 52). Logs are byte-identical.', True)]
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
