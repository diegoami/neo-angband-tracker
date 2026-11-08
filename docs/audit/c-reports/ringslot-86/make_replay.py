import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/ringslot-86'
TITLE='The Borg took off its good ring instead of its weak one when it found a better ring'
INTRO='At home the Borg works out its best set of equipment by trying every candidate item in every slot, the left ring slot before the right one. A check meant to stop the same ring going on both hands was also applied to the left slot, against the right-slot entry, which at that point still holds the last ring tried on the right hand earlier in the search. So a ring that had just been tried on the right hand was never tried on the left. A level-21 Warrior wearing a Ring of Feather Falling (left) and a Ring of See Invisible (right), with a Ring of Free Action in the pack, therefore never considered Free Action in place of Feather Falling: it took off See Invisible and put Free Action on, keeping the Feather Falling ring (Borg power 591441). The correction drops the left-slot check (the right-slot check already prevents wearing one ring twice); the Borg takes off Feather Falling instead and ends with See Invisible and Free Action (power 595941).'
CONDITIONS="Native Human Warrior, seed 86, character level 21, in town (depth 0); the starting kit is discarded. Worn: a Wooden Torch, a Dagger and Soft Leather Armour, plus rings by mode. Pack: a Ring of Free Action and five Rations of Food. All runes known; every store, including the home, is emptied. The game's first ring slot is the Borg's INVEN_RIGHT, the second its INVEN_LEFT. The fixture calls the production borg_clear_best() as the Borg does on arriving in town, sets shop_num to the home, then repeats the production borg_best_stuff() (the first step of borg_think_store()) until it declines to act. Each queued command is checked (exactly 't' or 'w' plus the letter of the item the Borg named) and carried out with the game's own inven_takeoff() / inven_wield(); the Borg then re-reads its gear with borg_find_all_items() and borg_notice(). 'swap': See Invisible in the first ring slot (right), Feather Falling in the second (left). 'mirror': the same two rings on the other hands. 'empty': no ring worn. This isolates the home equipment step; it does not walk to the home or play other store steps."
ISSUE=86
CARD_KEY='wears_kept_ring'
CARD_LABEL='still wearing the Ring of See Invisible at the end (1 = yes)'
CASES=[('swap', 'Weak ring on the left hand, good ring on the right, better ring in pack', 'Original: the left-slot search skips Free Action (it was the last ring tried on the right), so the only plan containing it puts it on the right hand; the Borg takes off See Invisible and wears Free Action, keeping Feather Falling. Corrected: Free Action is also tried on the left; the Borg takes off Feather Falling and wears Free Action, keeping See Invisible.', False), ('mirror', 'Control: good ring on the left hand, weak ring on the right', 'The weak ring is on the right hand, which the search fills last and checks correctly; both versions take off Feather Falling and wear Free Action. The left-slot check still runs (and skips Free Action in the original) but the best plan is the same, and the logs are identical.', True), ('empty', 'Control: no ring worn', 'Both ring slots are empty; trying Free Action on the left hand gives the same power as on the right, so the plan does not change. Both versions wear Free Action in one step, and the logs are identical.', True)]
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
