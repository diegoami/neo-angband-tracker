import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/stairphase-34'
TITLE='Escaping up a staircase, the Borg also read a scroll on the next level'
INTRO="When the Borg is in some danger while standing on an up staircase, its escape routine is meant to just take the stairs. In five of its escape steps the stair check was written as one half of the condition for a Phase Door: the check queues the climb key ('<') at once, and the rest of the condition still runs. A level-16 Warrior on an up staircase at the end of a corridor, next to a hill giant, queued the climb and then a Phase Door read. The game took the stairs, and then played the leftover keys on the new level: before the Borg had looked at that level, it read a Phase Door and was moved off the staircase it had arrived on. When a phase was blocked (the Borg had stood in an anti-summon spot ten turns earlier), the climb key stayed queued and the routine went on to queue a Teleportation read, which was then spent on the new level. The correction makes the stair escape a complete choice, as the two most dangerous escape steps already do: the Borg queues only '<', arrives on the new level and keeps its scrolls."
CONDITIONS="Native Human Warrior, seed 34, character level 16, depth 10, 5 Scrolls of Phase Door, 2 Scrolls of Teleportation, 5 Potions of Cure Light Wounds, a lit wooden torch and food. A lit one-wide corridor of 26 grids runs east from a dead end in solid granite; the Borg stands on the dead end (an up staircase, a down staircase or floor, by case) with an awake hill giant on the next grid. The Borg's danger estimate for its grid is 147; the player is set to 172 of 212 hit points (147 is about 85% of 172; 117 hit points in the climb control). Real code: the production borg_caution(), run once, which reaches borg_escape() with the danger of the Borg's grid; then the Borg's whole key queue is played through the game's own text UI (textui_process_command(), reading each key through the Borg's keypress hook) and the game loop (run_game_loop()), including the real level change and the generation of the new level. Fixture interventions: the level, the staircase and the monster are placed directly and the Borg's map is filled from the game's map; the pack is set as listed and sorted by the game; hit points are set as above; the Borg's clock is set to 5000 game turns with the level and town entered 1000 turns earlier, and in the antisummon case the Borg's anti-summon timer is set to ten turns ago, as borg_flow does when it stands in an anti-summon spot. Game messages are recorded with an EVENT_MESSAGE handler and are not shown to the Borg (no message flushes the Borg's key queue in this situation; only 'You failed', recall and deep-descent lift-off messages do). The Borg is not asked to think again once its queue is empty."
ISSUE=34
CARD_KEY='scroll_read_on_new_level'
CARD_LABEL='scroll read on the new level from keys queued on the old one (1 = yes)'
CASES=[('upstair', 'Up staircase, Phase Door possible', "Original: keys '<', Esc, Esc, 'r', 'd' (Danger Level 4.1). The game climbs to depth 9, the Borg arrives on a down staircase at (59,28), then reads a Phase Door there and lands at (63,36), off the staircase, with 4 scrolls left. Corrected: only '<' is queued; the Borg arrives at (59,28) on the staircase with all 5 scrolls.", False), ('antisummon', 'Up staircase, phase blocked by a recent anti-summon spot', "Original: '<' is queued by the stair check, the phase is refused by the anti-summon timer, and the routine goes on to Danger Level 4.2 and queues a Teleportation read after the '<'. The game climbs and the Borg reads Teleportation on the new level (2 to 1 scrolls). Corrected: the stair escape is final; only '<' is queued and no scroll is read.", False), ('downstair', 'Control: down staircase (the stair check fails, the phase is chosen)', 'The changed condition runs, the stair check returns false and the Borg reads a Phase Door on the current level in both versions; the logs are byte-identical.', True), ('floor', 'Control: plain floor (the stair check fails, the phase is chosen)', 'The same without any staircase: both versions read a Phase Door on depth 10; the logs are byte-identical.', True), ('climb', 'Control: up staircase, more hurt (climbing is the only choice)', 'At 117 hit points the danger is above the hit points and the earlier, unchanged escape step (Danger Level 2.1) takes the stairs alone in both versions; the logs are byte-identical. This control does not reach the changed lines.', True)]
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
