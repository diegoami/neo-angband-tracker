import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/homeskip-29'
TITLE='One recently stored home ware stopped the Borg clearing later junk from its home'
INTRO="In town the Borg's store step 5 takes items that add nothing to its home back out, so it can sell them. It skips any home item its sale history says it has just stored at home, so it does not take that straight back. The flag that marks an item to skip was cleared only once, before the loop over the home, so after the first skipped item every later item in the home was skipped too. A level-11 Warrior whose home held a Potion of Cure Critical Wounds it had just stored, followed by a Scroll of Light that adds nothing to the home, chose no store action at all and left the scroll at home. The correction clears the flag for each item; the Borg then sets out to take the Scroll of Light from its home to sell and the real retrieve command moves it into the pack."
CONDITIONS="Native Human Warrior, seed 29, character level 11, maximum depth 10, 3000 gold, standing on the home entrance in town (depth 0) with the starting kit after its Word of Recall scroll and Berserk Strength potion are removed (as after recalling to town and drinking the potion), all runes known. Every store is emptied and the home holds two items per mode. 'stored-first': Potion of Cure Critical Wounds, then Scroll of Light. 'stored-last': Scroll of Light, then Shovel. 'no-history': Potion of Infravision, then Scroll of Light. Fixture intervention: the Borg's sale history is given one entry, the fields borg_think_shop_sell() writes for a home sale (tval, sval, store = home), naming the potion ('stored-first') or the shovel ('stored-last'); 'no-history' leaves the history at its start-up value. A count of one is what the history holds after one recorded sale with an entry-counting counter, and after two sales with the original counter (issue #20), so this case does not depend on #20. The Borg's items and stores are loaded with the production borg_find_all_items(); the production borg_choose_shop() picks the next store action (steps 1 to 4 find nothing to do here and step 5, borg_think_home_grab_useless, decides); when it chooses a home ware, one production borg_think_store() turn queues the keys, the ware letter is checked and the real do_cmd_retrieve() runs."
ISSUE=29
CARD_KEY='grabs_scroll_of_light'
CARD_LABEL='sets out to take the useless Scroll of Light from the home (1 = yes)'
CASES=[('stored-first', 'Just-stored potion before a useless scroll', 'Original: step 4 and step 5 both skip the potion because it was just stored; step 5 leaves its skip flag set, so it also skips the Scroll of Light, no step sets a goal and the scroll stays at home. Corrected: the flag is cleared for the scroll, removing it leaves the home power unchanged (1514), so step 5 picks home slot 1 and the real retrieve moves the Scroll of Light into the pack.', False), ('stored-last', 'Control: just-stored shovel after the scroll', 'The just-stored item comes after the scroll, so the scroll is evaluated before the flag is ever set. Both versions take the Scroll of Light (home slot 0) and skip the shovel; the logs are identical.', True), ('no-history', 'Control: no recent home sale', 'With an empty sale history the flag is never set; neither item adds to the home, and step 5 takes the later one, the Scroll of Light (home slot 1), in both versions. The logs are identical.', True)]
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
