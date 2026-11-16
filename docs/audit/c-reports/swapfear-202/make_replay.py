import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/swapfear-202'
TITLE='A fear-protecting spare weapon got no credit, so the Borg kept the wrong one'
INTRO="When the Borg carries more than one spare weapon it keeps the most useful as its swap weapon and offers the others to the shops. Its scoring adds a bonus for a swap that protects from fear when the Borg has no fear protection of its own, but the flag that bonus reads was never set from the weapon, so a fear-protecting weapon was scored as if it gave nothing. The correction sets that flag (and the matching armour flag) from the item, exactly as protection from confusion and blindness already are."
CONDITIONS='Native Human Warrior, seed 202, character level 29 (30 in the brave control, where the Warrior class gives its own fear protection), deepest level 50 (the Borg only carries a swap weapon from level 50), no gold, a torch and rations, in a lit room, every rune known. Fixture interventions: the starting Potion of Berserk Strength is removed (it is the cheapest spare and would be offered first); the two spare Maces are made directly, one with the PROT_FEAR object flag set (the flag the (Holy Avenger) and of Fury egos and 26 artifacts carry) and one plain with one extra point of damage. The production swap scan, borg_power() and the shop sell-useless decision run unchanged. No combat, fear episode or death is measured.'
ISSUE=202
CARD_KEY='sells_fear'
CARD_LABEL='the fear-protecting mace is the one offered for sale (1 = yes)'
CASES=[('fear-plain', 'Fear-protecting Mace against a plain Mace with +1 damage', 'A level-29 Warrior with no fear protection of its own carries two spare Maces: one protects from fear, the other has one extra point of damage. Original: the fear protection is never recorded, so the +1 Mace scores higher, is kept as the swap weapon, and the fear-protecting Mace is offered for sale. Corrected: the fear-protection bonus applies, the fear-protecting Mace is kept and the +1 Mace is offered for sale.', False), ('fear-brave', 'Control: the same Maces, Warrior already fearless', 'The same two Maces, but at level 30 the Warrior class protects from fear by itself, so a swap\'s fear protection is worth nothing and the bonus correctly does not apply. The +1 Mace is kept in both builds and the logs are byte-identical.', True), ('plain-plain', 'Control: two plain Maces', 'Two plain Maces, one with +1 damage. No fear protection anywhere: the +1 Mace is kept and the other offered in both builds, with byte-identical logs.', True)]
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
