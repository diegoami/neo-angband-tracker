import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/drophole-136'
TITLE='A full-pack Borg above level 35 dropped a spare amulet and kept a worthless potion'
INTRO="When the Borg's pack is full it drops the item whose loss costs it least. Above character level 35 the drop code is meant to protect a short list of valuable consumables (healing and mana potions, a few attack rods and wands, two protective scrolls) by giving them a very high value. The protection test was written as 'not A or not B', which is true for every item, so every potion, rod and wand counted as worth 300,000 gold and every scroll 30,000. A level-40 Warrior with a full pack therefore kept a Potion of Salt Water (worth nothing) and dropped a spare Amulet of ESP (25,000 gold), or a spare Amulet of Slow Digestion, inscribed for the Borg to destroy. With the comparisons corrected to 'is A or is B', only the listed items are protected and the Salt Water is dropped."
CONDITIONS="Native Human Warrior, seed 136, depth 20, every rune known, standing on an open floor with no monsters. The starting kit's carried items are removed (the wielded torch stays), then the pack holds 3 rations, one spare amulet, one potion, and is filled to the last slot with fully known artifact weapons, which the full-pack drop never considers. The production borg_drop_hole(false) runs once; the queued keys are read back and checked to be the inscribe-and-drop sequence for one inventory letter, and that item is then dropped with the game's own inven_drop(). Fixture interventions: experience granted directly to reach the level, pack contents built directly, the Borg's map copied from the arena. This isolates the choice of item to drop; it does not play the later crushing of the dropped item or any trip to sell."
ISSUE=136
CARD_KEY='dropped_amulet'
CARD_LABEL='dropped the spare amulet (1 = amulet dropped, 0 = potion dropped)'
CASES=[('salt-water', 'Level 40: spare Amulet of Slow Digestion and a Potion of Salt Water', 'Original: the Salt Water counts as worth 300,000 gold, so the Borg drops the amulet (worth 200 gold). Corrected: the Salt Water counts as worth nothing and is dropped.', False), ('spare-esp', 'Level 40: wearing an Amulet of ESP, carrying a second one and a Potion of Salt Water', 'Original: the Borg drops the spare 25,000-gold Amulet of ESP and keeps the Salt Water. Corrected: it drops the Salt Water and keeps the amulet.', False), ('restore-mana', 'Control: level 40, spare Amulet of Slow Digestion and a Potion of Restore Mana', 'Restore Mana is on the protected list. The changed potion test runs in both versions and values it at 300,000 gold either way, so both drop the amulet; the logs are byte-identical.', True), ('level30', 'Control: level 30, spare Amulet of Slow Digestion and a Potion of Salt Water', 'Below level 36 the drop code skips the high-level valuation block entirely; both versions drop the Salt Water and the logs are byte-identical. This shows the rest of the drop choice is unchanged.', True)]
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
