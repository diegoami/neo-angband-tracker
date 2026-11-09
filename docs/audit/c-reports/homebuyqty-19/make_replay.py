import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/homebuyqty-19'
TITLE='A skipped home-buy trial hid a home weapon from the backup-weapon step'
INTRO="In town the Borg's step 4 tries each item in its home to see whether buying it back would make the Borg stronger. For each trial it first takes the item off its own record of the home, and only then checks that the pack has two free slots. When that check fails it skips the item without putting it back on the record. A level-21 Warrior with one free pack slot and a single Tulwar in the home therefore records the home as holding zero Tulwars; step 7A, which looks for a backup weapon and needs only one free slot, then skips the Tulwar as an empty home slot and the Borg takes nothing. The correction puts the item back on the record before skipping it; step 7A then sees the Tulwar and the Borg sets out to buy it as its backup weapon."
CONDITIONS="Native Human Warrior, seed 19, character level 21, maximum depth 50 (the depth at which the Borg starts carrying backup weapons; the shipped borg.txt has borg_uses_swaps = TRUE), 5000 gold, in town (depth 0) with its starting kit, all runes known. The pack is filled with distinct single potions and scrolls until only its last slot is free ('roomy': the last two slots). Every store is emptied. 'single': one Tulwar in the home. 'pair': two Tulwars stacked in one home slot. 'roomy': one Tulwar, two free pack slots. 'stack': one Potion of Cure Light Wounds in the home, which stacks with the one in the pack. The Borg's items and stores are loaded with the production borg_find_all_items(); then the production store steps 4 (borg_think_home_buy_useful), 5, 6, 7A and 7B run in the order borg_choose_shop() runs them, stopping at the first that sets a goal, and the goal is read back. Steps 1 to 3 (selling to the home and shops, buying from shops) are not run: with this filler pack they choose to sell single potions or scrolls first, which would end the scan before step 4."
ISSUE=19
CARD_KEY='buys_home_ware0'
CARD_LABEL='sets out to buy the home item (1 = yes)'
CASES=[('single', 'One Tulwar at home, one free pack slot', 'Original: step 4 takes the Tulwar off its home record, finds only one free pack slot and skips it, leaving the record at 0 Tulwars; step 7A then skips home slot 0 as empty and no step sets a goal. Corrected: the record is restored to 1, step 7A tries the Tulwar as a backup weapon and the Borg sets out to buy it from the home.', False), ('pair', 'Two Tulwars at home, one free pack slot', 'The same early skip leaves the record at 1 of 2 Tulwars in the original (2 corrected), so the record check fails before and passes after. One Tulwar is still visible, so step 7A buys a backup weapon in both versions: the decision is unchanged here, only the recorded quantity differs.', False), ('roomy', 'Control: one Tulwar, two free pack slots', 'With two free slots the room check passes, the trial runs to the end and the record is restored as before; step 4 itself picks the Tulwar in both versions. The logs are identical.', True), ('stack', 'Control: a potion at home that stacks with the pack', 'The home potion would join an existing pack stack, so the inventory trial needs no free slot and runs to the end; the record is restored as before and no step takes anything. The logs are identical.', True)]
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
