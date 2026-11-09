import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/necrolight-296'
TITLE='A Necromancer in a lit room thought its spell was safe'
INTRO="Necromancers cast worse in the light: whenever their square is lit, the game adds 25 points to every spell's failure chance. The Borg estimates that failure chance before deciding to cast, and it refuses an attack spell whose estimate is over its limit (25% for Vampire Strike against an ordinary monster). Its estimate added the light penalty only when the Borg's own torch lit the square, and Necromancers normally carry no torch. A level-26 Necromancer standing in a permanently lit room facing a kobold therefore estimated 8% failure for Vampire Strike while the game used 33%, and cast it anyway. The correction uses the same lit-square test as the game: the estimate becomes 33% and the Borg does not cast the spell."
CONDITIONS="Native Human Necromancer, seed 296, level 26, depth 30, eight intelligence gains, [Fear and Torment] carried with only Vampire Strike learned, full mana (118), no light source. A 40x40 arena whose floor is permanently lit (SQUARE_GLOW); one awake Kobold five squares south. The fixture fills the Borg's map from the game cave and observes the kobold; the Borg's own light map is not touched (no light is carried, so it would be empty). The production borg_attack(false) runs once in acting mode; queued keys are read back. The game's spell_chance() and the Borg's borg_spell_fail_rate() are printed for the same spell. Controls: the same Necromancer with only its own square unlit, and a Human Mage (starting torch) on the lit arena casting Magic Missile. The cast is not executed: this measures the estimate and the decision, not damage, failed-cast frequency or survival."
ISSUE=296
CARD_KEY='cast_spell'
CARD_LABEL="casts Vampire Strike (1 = yes) with the game's failure chance above the Borg's 25% limit"
CASES=[('lit', 'Torchless Necromancer on a permanently lit square', "Original: the Borg estimates 8% failure while the game's spell_chance() is 33%; the estimate passes the 25% limit and the Borg casts Vampire Strike (keys m b b). Corrected: the estimate is 33%, equal to the game's; the Borg does not cast.", False), ('dark', 'Control: the same Necromancer, its own square unlit', "No light on the Borg's square: the game uses 8%, both versions estimate 8% and cast Vampire Strike. Logs are byte-identical.", True), ('mage', 'Control: a Mage on the lit square', 'Mages have no light penalty: the game and both versions use 2% for Magic Missile and the Borg casts it. Logs are byte-identical.', True)]
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
