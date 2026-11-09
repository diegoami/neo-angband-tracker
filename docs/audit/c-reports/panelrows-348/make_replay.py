import html,json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
SUITE='native-c/panelrows-348'
TITLE='A tall or wide terminal made the Borg read past its own map'
INTRO="The Borg keeps its own copy of the level map, 66 rows of 198 grids (the largest level). Three of its searches look only at the part of the map the game is showing: looking for a lit area to walk to when it has no light, looking for a vault wall it can dig, and looking for a vault wall it can turn to mud. The size of that visible part grows with the terminal window. With the standard sidebar, a terminal of 69 or more rows, or 213 or more columns, shows more rows or columns than any level has, and the game puts the view at the level's edge. The searches then read past the end of the Borg's map: in a 70-row terminal the game crashed (segmentation fault) as soon as one of these searches ran; in a 230-column terminal it read neighbouring memory without crashing. The correction stops each search at the edge of the Borg's map; on standard terminals nothing changes."
CONDITIONS="Native Human Warrior (light and vault scans) or Human Druid (Stone to Mud scan), seed 348, character level 21, depth 10, on a full-size open level (198x66). Terminal sizes are set with term_init (standard sidebar layout: map starts at column 13, one status row above and below). The game's own center_panel() places the view around the player, and the Borg's panel origin is copied from the terminal exactly as borg_update() does. Fixture interventions: the terminal size, the arena, the Borg's map entries (features copied from the level; the 11x8 lit room is marked as lit in the Borg's map, as its call-light code records it), the vault flag, a wielded Mattock (Warrior vault cases), the Druid's two town books with every spell up to its level learned. Each case runs one production call: borg_flow_light(GOAL_FLEE), borg_flow_vault(35) or borg_excavate_vault(z_info->max_range - 2), as borg_think_dungeon() calls them, and reads back the first queued key. The replay page shows the AddressSanitizer build; the normal-build results are in the README table."
ISSUE=348
CARD_KEY='completed'
CARD_LABEL='the production call returned and the scenario finished (1); n/a means the run stopped on a crash or sanitizer report'
CASES=[('light-tall', 'Glow scan, 80x70 terminal (panel 68 rows on a 66-row level)', "Original: the glow scan reads row pointers 66 and 67 past the end of the Borg's 66-row map; the normal build crashes (segmentation fault) and AddressSanitizer reports a global-buffer-overflow READ in borg_flow_light (borg-flow-misc.c:836). Corrected: the scan stops at row 65, finds the 88 lit grids of the room and the Borg steps toward them (key 49).", False), ('light-wide', 'Glow scan, 230x24 terminal (panel 216 columns on a 198-column level)', 'Original: the glow scan reads 18 grids past the end of every map row; the normal build does not notice (it reads neighbouring heap memory and happens to find no stray glow), AddressSanitizer reports a heap-buffer-overflow READ in borg_flow_light (borg-flow-misc.c:839). Corrected: no out-of-map read; the same 66 lit grids and the same step (key 49).', False), ('light-normal', 'Control: glow scan, standard 80x24 terminal', 'The panel (66x22) lies inside the map; both versions find the same 66 lit grids and step toward them (key 49); byte-identical logs.', True), ('light-fit', 'Control: glow scan, 212x68 terminal (panel exactly 198x66, the largest that fits)', 'The panel is exactly the size of the map; the clamp never cuts the loop. Both versions find all 88 lit grids; byte-identical logs.', True), ('vault-tall', 'Vault-wall scan, 80x70 terminal', 'Original: borg_flow_vault(35) reads the missing rows 66 and 67 for grids within 35 steps; the normal build crashes, AddressSanitizer reports a global-buffer-overflow READ (borg-flow-misc.c:909). Corrected: the magma vein next to the permanent wall is found and the Borg flows toward it (key 50).', False), ('vault-wide', 'Vault-wall scan, 230x24 terminal', 'Original: reads past the end of rows near the east edge; silent in the normal build, AddressSanitizer heap-buffer-overflow READ (borg-flow-misc.c:909). Corrected: same target and step (key 50), no out-of-map read.', False), ('vault-normal', 'Control: vault-wall scan, standard 80x24 terminal', 'Panel inside the map; both versions flow to the same vein (key 50); byte-identical logs.', True), ('dig-tall', 'Stone to Mud vault scan (Druid), 80x70 terminal', 'Original: borg_excavate_vault() reads the missing rows; the normal build crashes, AddressSanitizer global-buffer-overflow READ (borg-think-dungeon-util.c:1016). Corrected: the Borg targets the vein and casts Turn Stone to Mud (first key 42, the targeting key).', False), ('dig-wide', 'Stone to Mud vault scan (Druid), 230x24 terminal', 'Original: silent read past the row ends in the normal build, AddressSanitizer heap-buffer-overflow READ (borg-think-dungeon-util.c:1016). Corrected: same cast, no out-of-map read.', False), ('dig-normal', 'Control: Stone to Mud vault scan, standard 80x24 terminal', 'Panel inside the map; both versions cast Turn Stone to Mud at the same vein; byte-identical logs.', True)]
for mode,label,explanation,control in CASES:
 item={'label':label,'explanation':explanation,'control':control}
 for phase in ('before','after'):
  raw=(root/f'asan-{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':SUITE+' finished: 1/1 passed' in raw}
  for tag,key in (('STATE ','state'),('OUTCOME ','outcome')):
   for match in re.findall(re.escape(tag)+r'(.*)',raw):
    entry.setdefault(key,{}).update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match)})
  notes=[l for l in raw.splitlines() if l.startswith(('GEOMETRY ','DECISION ','TRACK ','OUTCOME ','READ of size','SUMMARY: AddressSanitizer'))]
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
 '<p>Recorded native C runs, original and corrected source (AddressSanitizer build)</p>'
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
