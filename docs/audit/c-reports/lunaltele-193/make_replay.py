import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/lunaltele-193'
TITLE="A monster teleport did not end the Borg's Lunal mode"
INTRO='Lunal mode is a Borg setting (borg_lunal_mode in borg.txt, or engaged by the Borg itself when it is far stronger than the level) in which the Borg bounces between staircases to dive quickly. While it is on, the Borg skips its usual safety checks: it walks without checking how dangerous each grid is, it refuses to rest, and it takes the stairs down even when it is not prepared for the next depth. The Borg\'s code intends to switch Lunal mode off when a monster spell moves it away from its grid ("he is not on the stairs anymore"), but that check never fires: every monster spell line is recognised earlier as a spell reaction. In this run a tengu commands the Borg to return (and, separately, a mage commands it to go away). The original Borg stays in Lunal mode and immediately starts walking to the down staircase, although its own preparedness check says it is not ready for depth 11 ("restock phase door"). After the fix the Borg leaves Lunal mode when the teleport reaction is handled, and the same stair decision refuses to go down. A spell that does not move the Borg (a shrieker\'s shriek) leaves Lunal mode on in both versions.'
CONDITIONS="Native Human Warrior, seed 193, character level 15, depth 10, lit open 24x40 arena with one down staircase at (24,18) and one up staircase at (3,3); profile borg.txt with borg_lunal_mode = TRUE (the configured Lunal mode, read by borg_init). Real code: the game's monster spell code (do_mon_spell with the game's teleport-to/teleport-away/shriek effects, which really move the player), the Borg's production message parser (borg_parse), its map/message update (borg_update, three passes), borg_notice, borg_prepared and the production stair decision borg_flow_stair_more(GOAL_BORE, false, false) as called from borg_leave_level when the Borg is bored on a level. Fixture interventions: the arena, stairs and caster are placed directly; the spell is cast by calling do_mon_spell directly (the monster's own spell choice is not run; the tengu and mage have the cast spell in their stock spell lists, the shrieker has SHRIEK); an EVENT_MESSAGE handler records each line the game prints and hands it to borg_parse instead of the Borg scraping it from the screen; in teleaway-unseen the spell is cast as by an unseen caster (seen=false) so the game prints its 'Something ...' line; in teleto-nolunal the fixture sets borg.lunal_mode = false after borg_init."
ISSUE=193
CARD_KEY='heads_down'
CARD_LABEL='Borg starts walking to the down staircase to depth 11, for which it is not prepared (1 yes, 0 no)'
CASES=[('teleto', "Tengu: 'The tengu commands you to return.'", "The tengu's teleport-to moves the Borg from (10,12) to (12,12), next to the tengu. Original: Lunal mode stays on (lunal_mode after=1) and the stair decision starts walking to the down staircase although the Borg is not prepared for depth 11. Corrected: Lunal mode is switched off when the SPELL_068 reaction is handled, and the same decision refuses the descent.", False), ('teleaway', "Mage: 'The mage commands you to go away.'", "The mage's teleport-away moves the Borg from (10,12) to (38,1). Original: Lunal mode stays on and the Borg heads for the down staircase. Corrected: Lunal mode is off after the SPELL_070 reaction and the descent is refused.", False), ('teleaway-unseen', "Unseen caster: 'Something commands you to go away.'", 'The same teleport-away, cast as by a monster the player cannot see. The Borg recognises it as SPELL_070:Something. Original: stays in Lunal mode and heads down. Corrected: Lunal mode off, descent refused.', False), ('shriek', "Control: 'The shrieker mushroom patch makes a high-pitched shriek.'", 'A monster spell that does not move the Borg goes through the same spell-reaction code. Lunal mode stays on and the Borg heads for the down staircase in both versions; byte-identical logs.', True), ('teleto-nolunal', 'Control: tengu teleport-to with Lunal mode not in effect', 'The same tengu teleport-to with Lunal mode already off. Both versions refuse the descent because the Borg is not prepared for depth 11; byte-identical logs.', True)]
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
