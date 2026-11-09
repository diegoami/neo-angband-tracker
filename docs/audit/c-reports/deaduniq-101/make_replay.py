import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/deaduniq-101'
TITLE="Borg started after Sauron's death swaps in a weaker helm or throws away an item on its first move"
INTRO="When the Borg starts, borg_init_flow_kill() is meant to copy the game's record of dead uniques into the Borg's own table, but its test is inverted (it skips the uniques and checks the non-uniques), so the table starts empty. The first map update fills it in, but borg_think() examines the character before that update and computes the Borg's reference power score (borg.power) from that examination. On a character that has killed Sauron and carries Scrolls of Mass Banishment, borg_power() adds 2,500 per scroll once Sauron is known dead, so the reference score of the first pass is 12,500 lower than every alternative the same pass scores. In this run a level-41 Warrior at depth 98 (maximum depth 99) wears a Metal Cap and carries a Hard Leather Cap. The original Borg swaps the Metal Cap for the weaker Hard Leather Cap; on the junk decision it drops and ignores (marks for destruction) a Potion of Berserk Strength, or, without the birth-kit potion, one of its five Scrolls of Mass Banishment. After the fix the table is filled at start-up, the reference score matches the alternatives, and both decisions do nothing. With Sauron alive both versions behave identically."
CONDITIONS="Native Human Warrior, seed 101, character level 41, depth 98, maximum depth 99, lit open 24x40 arena with no monsters; worn Metal Cap [3], Wooden Torch, the birth-kit dagger and soft leather armour; carried 30 Potions of Healing and 15 Potions of Speed (so BI_PREP_BIG_FIGHT is false, borg-trait.c:2928-2947), 5 Scrolls of Mass Banishment (none in wear-noscrolls), plus a Hard Leather Cap [2] (wear modes) or 5 Scrolls of Teleportation and 5 Potions of Cure Critical Wounds (junk modes); profile borg.txt from the shared evidence profile. Real code: production borg_init() (which runs borg_init_flow_kill), then the first pass in borg_think()'s order (borg_find_all_items, borg_notice, borg_update, borg.power = borg_power(); borg-think.c:186-217), the borg_notice(true) that borg_think_dungeon() runs before its decisions (borg-think-dungeon.c:1435), and the production decision borg_wear_stuff() (wear modes) or borg_drop_junk() (junk modes); the requested keys are checked and carried out with the game's inven_wield() or inven_drop(). Fixture interventions: Sauron's death is recorded as the game records it (lore kill count 1 and r_info max_num 0, mon-util.c:1101, load.c:533-535) instead of by fighting him; the character, arena and gear are built directly; the birth kit's spare torches are removed (and, in junk-nokitpotion, its potions); the decision function is called directly rather than reached through the whole dungeon thinker; the 'borg ignore' inscription that precedes the junk drop is checked in the keys but not carried out, only the drop."
ISSUE=101
CARD_KEY='acts'
CARD_LABEL='Borg acts on its first pass: swaps the helm or drops an item (1 yes, 0 no)'
CASES=[('wear', 'Sauron dead, 5 Mass Banishment: spare Hard Leather Cap', "Original: the first pass's reference power is 1,513,679 while the same gear scores 1,526,179 once Sauron is known dead; the Hard Leather Cap scores 1,524,179 (2,000 below the worn Metal Cap) and still beats the stale reference, so the Borg sends 'w' and swaps the Metal Cap [3] for the Hard Leather Cap [2]. Corrected: the reference is 1,526,179 and no swap is made.", False), ('junk', 'Sauron dead, 5 Mass Banishment: junk decision', "Original: borg_drop_junk() (enabled by the first map update's new-level branch) compares each possible drop with the stale reference, finds the Potion of Berserk Strength's removal 'better', and requests '{' 'borg ignore' then 'd': the potion is dropped and marked for destruction. Corrected: nothing is dropped.", False), ('junk-nokitpotion', 'Sauron dead, 5 Mass Banishment, no birth-kit potions: junk decision', "The same junk decision without the birth-kit potions. Original: the cheapest removal is one Scroll of Mass Banishment, which the Borg drops and marks 'borg ignore'. Corrected: nothing is dropped.", False), ('alive-wear', 'Control: Sauron alive, spare Hard Leather Cap', 'No unique is dead, so the table is empty in both versions and the reference score is consistent. No swap in either version; byte-identical logs (the corrected start-up loop runs over every race).', True), ('alive-junk', 'Control: Sauron alive, junk decision', 'Sauron alive, same pack as the junk case. Nothing is dropped in either version; byte-identical logs.', True), ('wear-noscrolls', 'Sauron dead, no Mass Banishment scrolls: spare Hard Leather Cap', 'Sauron dead but no Mass Banishment scrolls, so his death changes no power term. The first examination still reads Sauron as alive in the original (sauron_dead_trait=0) and as dead after the fix, but the reference score is the same and neither version swaps. Not byte-identical (the trait line differs); shown to bound the effect.', False)]
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
