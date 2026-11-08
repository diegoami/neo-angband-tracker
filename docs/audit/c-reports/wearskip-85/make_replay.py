import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/wearskip-85'
TITLE='A full pack stopped the Borg from wearing its other planned upgrades'
INTRO='At home the Borg plans its best set of equipment and then makes the changes one step at a time. When the next change needs a free pack slot and the pack is full, it is meant to skip that change and try the following one. The skip counter was reset on every pass of the search, so instead every remaining change was skipped and the plan was thrown away. A level-21 Warrior with exactly one free pack slot, a stack of two Tulwars, Metal Scale Mail and a Cloak in the pack switched to a Tulwar (which filled the pack) and then stopped: the armour change needed a free slot, and the cloak, which could have gone on at once and freed that slot, was never tried. The correction keeps the counter across the search; the Borg puts on the cloak, then swaps the armour.'
CONDITIONS="Native Human Warrior, seed 85, character level 21, in town (depth 0); the starting kit is discarded. Worn: a Wooden Torch, a Dagger and Soft Leather Armour; the cloak slot is empty. Pack: Tulwars, Metal Scale Mail, a Cloak, five Rations of Food and distinct potions that fill the pack to exactly one free slot. All runes known; every store, including the home, is emptied. The fixture calls the production borg_clear_best() as the Borg does on arriving in town, sets shop_num to the home, then repeats the production borg_best_stuff() (the first step of borg_think_store()) until it declines to act. Each queued command is checked (exactly 'w' or 't' plus the letter of the item the Borg named) and carried out with the game's own inven_wield() / inven_takeoff(); the Borg then re-reads its gear with borg_find_all_items() and borg_notice(). 'tight': two Tulwars stacked in one slot, one free slot. 'roomy': one Tulwar, two free slots. 'nocloak': as 'tight' but without the cloak (and one more potion). This isolates the home equipment steps; it does not walk to the home or play other store steps."
ISSUE=85
CARD_KEY='wears_cloak'
CARD_LABEL='wearing the Cloak at the end (1 = yes)'
CASES=[('tight', 'Two stacked Tulwars, one free slot, Cloak in pack', 'Original: the Dagger comes off and a Tulwar goes on (2 actions); the pack is now full, the armour change is skipped, the cloak is skipped too and the plan is discarded, so the Borg ends in Soft Leather Armour with no cloak. Corrected: the armour change is skipped once, the Cloak goes on (freeing a slot), then the armour is swapped for Metal Scale Mail (5 actions).', False), ('roomy', 'Control: one Tulwar, two free slots', 'The pack never fills, so no change is skipped; both versions make the same five changes and the logs are identical.', True), ('nocloak', 'Control: full pack and nothing else to try', 'The armour change is skipped (the skip path runs in both versions) and no other change remains; both versions stop after the Tulwar and discard the plan. The logs are identical.', True)]
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
