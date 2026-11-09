import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/crushv2-66'
TITLE='A Necromancer Borg never cast Crush on a weak monster shooting at it from range'
INTRO="Crush kills every monster in view with less than four times the caster's level in hit points. The Borg values Crush by how much it lowers the danger at its grid. The danger code has two Crush adjustments, one for melee danger and one for ranged danger, but the ranged one cleared the melee value again instead of the ranged value. A monster that would die to Crush therefore kept all of its ranged danger, and Crush looked useless against anything that attacks from a distance. A level-26 Necromancer whose only attack spell is Crush, with an awake blacklock mage (36 hit points, under the limit of 104) five steps away, saw danger 936 both with and without Crush and did nothing. With the ranged value cleared, Crush removes all 936 danger, the Borg casts it, and the real cast kills the mage. The same holds for a cave orc that shoots arrows."
CONDITIONS="Native Human Necromancer, seed 66, level 26, depth 20, 183 hit points, carrying [Dark Rituals] with only Crush learned (no other attack spell, no missiles), in a synthetic 40x40 arena. The Borg's map is copied from the game cave and every grid is marked in view; one monster is placed five steps south, made aware and tracked by the Borg as awake. The production borg_attack(false) runs once; the keys it queues are resolved to the book and spell and executed with the real CMD_CAST command. The danger lines are diagnostic calls of borg_danger() with and without the Crush flag, the same calls borg_attack_aux_crush() makes. Cases: blacklock mage (ranged spells), cave orc (innate arrows), ogre (melee only, control), dark dwarven lord (132 hit points, too strong to crush, control)."
ISSUE=66
CARD_KEY='cast_crush'
CARD_LABEL='cast Crush (1 = the Borg queued Crush and the real cast ran)'
CASES=[('caster', 'Blacklock mage (36 hp, spells) five steps away', "Original: danger 936 with and without Crush, reduction 0, no attack chosen. Corrected: Crush clears the mage's ranged danger (936 to 0); the Borg queues 'mbb', the real cast kills the mage.", False), ('archer', 'Cave orc (50 hp, innate arrows) five steps away', 'Original: danger 48 with and without Crush, no attack. Corrected: Crush clears the arrow danger (48 to 0); the Borg casts Crush and the orc dies.', False), ('melee', 'Control: ogre (60 hp, no ranged attack) five steps away', 'The changed line runs (the ogre is crushable and in view) but its ranged danger is already 0; both versions see danger 0, choose no attack, and the logs are byte-identical.', True), ('tough', 'Control: dark dwarven lord (132 hp, too strong to crush)', 'Above the 104 hit-point limit Crush would not kill it; both versions keep danger 720, choose no attack, and the logs are byte-identical.', True)]
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
