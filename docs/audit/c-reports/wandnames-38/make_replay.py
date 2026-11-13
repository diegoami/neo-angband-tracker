import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/wandnames-38'
TITLE='Two wands were looked up under the wrong names, so the Borg aimed the wrong wand'
INTRO="When the Borg starts it looks up each wand it knows how to use by name. Two lines were copies of the lines above them: the Confuse Monster wand was looked up as Scare Monster, and the Lightning Balls wand as Lightning Bolts. So the attack chooser offered a Lightning Bolts wand (a 6d6 beam, about 21 damage) as a 64-damage ball, offered a Scare Monster wand as a confusion bolt, and never used a real Lightning Balls or Confuse Monster wand. The correction looks the two wands up by their own names."
CONDITIONS='Native Human Warrior, seed 38, character level 20, dungeon level 20, in a lit arena, every rune known, an awake black ogre (100 hit points, no electricity or cold immunity) four grids east in a straight line. Fixture interventions: every starting pack item except food is removed so the wands under test are the only ranged attacks; the wands are made directly with 10 charges and are fully known; the ogre is entered into the Borg monster list through the production observe_kill_diff(). The production borg_calculate_attack_effectiveness() scores each wand attack and the production borg_attack() chooses and queues the attack; the zap itself is not played.'
ISSUE=38
CARD_KEY='chose_cold'
CARD_LABEL='with Lightning Bolts and Frost Bolts wands, the Borg aims the Frost Bolts wand (1 = yes)'
CASES=[('bolts-vs-frost', 'Lightning Bolts wand against Frost Bolts wand', 'The Borg carries a Lightning Bolts wand (6d6 beam, about 21 damage) and a Frost Bolts wand (12d8 bolt, about 54 damage). Original: the Lightning Bolts wand is also offered as a Lightning Balls attack worth 64 damage, so the Borg chooses it and would fire a 21-damage beam. Corrected: the Lightning Bolts wand is offered only as a beam and the Borg chooses the Frost Bolts wand.', False), ('balls-only', 'Real Lightning Balls wand', 'The Borg carries only a Lightning Balls wand (64 damage, radius 2). Original: no attack type matches it, so the Borg never zaps it. Corrected: it is offered as the Lightning Balls attack and chosen.', False), ('confuse-only', 'Real Confuse Monster wand', 'The Borg carries only a Confuse Monster wand, which the Magic shop sells. Original: no attack type matches it. Corrected: it is offered as the confusion attack.', False), ('scare-only', 'Scare Monster wand', 'The Borg carries only a Scare Monster wand. Original: it is offered both as a fear bolt and, wrongly, as a confusion bolt. Corrected: only as a fear bolt.', False), ('frost-only', 'Control: Frost Bolts wand only', 'The Borg carries only a Frost Bolts wand, whose lookup was always right. It is chosen in both builds and the logs are byte-identical.', True)]
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
