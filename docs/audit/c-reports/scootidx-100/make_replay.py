import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/scootidx-100'
TITLE='A monster out of sight made the Borg phase away from a harmless neighbour'
INTRO="Rogues, warriors and other archers use 'Shoot N Scoot': when a dangerous monster reaches melee range they cast Phase Door to get distance and keep shooting. To decide whether the monster next to the Borg is worth it, one test looks up that monster's danger. It looked up the wrong monster: it used the direction number of the grid (0 to 7) as the monster number, so it read whichever monster the Borg happened to record in that slot. A level-38 Rogue at full health (296 hit points) with only a mummified orc beside it (danger 16 at its grid) therefore cast Phase Door, because monster number 1 in its records was an ogre mage down a corridor, out of sight of the Rogue. It spent a turn and 2 spell points teleporting away from a neighbour whose own danger the test rates 0, to a random grid. The correction looks up the monster actually standing in that grid, and the Rogue stays put."
CONDITIONS="Native Human Rogue, seed 100, level 38, depth 30, 296/296 hit points, 154 spell points, Phase Door learned from [First Spells] (no other spell), a sling with 30 iron shots and 5 Phase Door scrolls, in a lit 40x40 arena. The Borg's map is copied from the game cave. An awake ogre mage stands at the end of a walled corridor running west from the grid north of the Borg; the Borg saw it from that grid a moment earlier (the fixture moves the player there to observe it, then back), so it is the Borg's monster number 1 but is out of sight of the Borg's grid. 'north': an awake mummified orc (monster number 2, no spells, not in a group) stands north of the Borg, in direction slot 1. 'south' (control): the same orc stands south, direction slot 0, an unused monster number. 'pack' (control): an olog, a pack monster close to the Borg's level, stands north; it qualifies for Shoot'N'Scoot without the danger test. The production borg_caution() runs once; the queued keys are resolved to the book and spell and executed with the real CMD_CAST command. The danger lines are diagnostic calls of borg_danger_one_kill() with the two candidate indices."
ISSUE=100
CARD_KEY='cast_phase'
CARD_LABEL="cast Phase Door to Shoot'N'Scoot (1 = yes)"
CASES=[('north', "Mummified orc north (direction slot 1 = the ogre mage's number)", "Original: the test reads the ogre mage's danger at the orc's grid (144, above 30% of 296 = 88) instead of the orc's (0); '# Shoot N Scoot. (Danger Level 7.1)', Phase Door is cast, the Rogue moves from (20,20) to (29,23) and spends 2 spell points. Corrected: the orc's own danger is read; no Shoot'N'Scoot, no cast.", False), ('south', 'Control: mummified orc south (direction slot 0, unused)', "Slot 0 holds no monster and the orc's own danger is 0, so neither version scoots; the logs are identical.", True), ('pack', 'Control: olog north (pack monster, qualifies anyway)', "The olog qualifies through the pack-monster test, so both versions Shoot'N'Scoot and cast Phase Door to the same grid; the logs are identical.", True)]
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
