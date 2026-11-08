import json,re,sys
from pathlib import Path
root=Path(sys.argv[1]);data={}
CASES={
 'excavate':('Excavating a vault dig site','A level-12 Druid with Stone to Mud digs a quartz grid beside a permanent vault wall. The original clears the grid\'s glow flag and then sets the glow again under a comment reading "Dark" — the freshly dug grid is remembered as glowing. The correction sets the dark flag as the comment says.'),
 'no-target':('Control: no dig site beside a permanent wall','With no excavatable grid adjacent to a permanent wall the routine finds nothing and no flags are touched. The complete logs are byte-identical.')}
for mode,(label,explanation) in CASES.items():
 item={'label':label,'explanation':explanation}
 for phase in ('before','after'):
  raw=(root/f'{phase}-{mode}.log').read_text()
  entry={'raw':raw,'passed':'native-c/excavate-236 finished: 1/1 passed' in raw}
  for tag,key in (('SETUP vault=','setup'),('DECISION ','decision'),('STATE ','state'),('OUTCOME ','outcome')):
   match=re.search(re.escape(tag)+r'(.*)',raw)
   entry[key]={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match[1])} if match else None
  seq=re.search(r'sequence=(.*)',raw)
  entry['sequence']=seq[1].strip() if seq else None
  item[phase]=entry
 data[mode]=item
CSS=("body{margin:0;background:#f4f1ea;color:#253a3c;font:18px/1.6 system-ui}"
 "main{max-width:1100px;margin:auto;padding:32px 24px}"
 "h1{font-size:40px;line-height:1.2}h2{font-size:24px}"
 ".cards{display:grid;grid-template-columns:1fr 1fr;gap:20px}"
 ".card{background:white;border:1px solid #cbd6d0;border-radius:12px;padding:24px;margin:18px 0}"
 ".cards .card{border-top:6px solid #648e80}.num{font-size:34px;font-weight:750}"
 ".summary{background:#e6eee6;padding:20px;border-left:5px solid #377367}"
 "pre{white-space:pre-wrap;overflow-wrap:anywhere;font:13px/1.5 monospace;max-height:330px;overflow:auto}"
 "details{background:white;padding:20px;margin:20px 0}summary{font-weight:700;cursor:pointer}"
 "@media(max-width:650px){.cards{grid-template-columns:1fr}h1{font-size:30px}}")
def state_txt(item, phase):
 s = item[phase]['state']
 return f"glow {s['glow']}, dark {s['dark']}"
html=('<!doctype html><html lang="en"><meta charset="utf-8">'
 '<meta name="viewport" content="width=device-width,initial-scale=1">'
 '<title>The excavated vault grid was remembered as glowing</title>'
 '<style>'+CSS+'</style><main>'
 '<p>Recorded native vault excavations</p>'
 '<h1>The excavated vault grid was remembered as glowing</h1>'
 '<p>When the Borg digs a vault wall with stone to mud, it updates its map of the dug grid: clear the glow flag, then set the flag the comment calls "Dark". The original set the glow flag again instead of the dark flag — the freshly dug grid was remembered as glowing, and glow-driven decisions could treat a dark excavation pocket as safe light. The correction sets the dark flag.</p>'
 '<section class="card"><label for="scenario">Choose a recorded situation</label><br>'
 '<select id="scenario"></select><p id="explanation"></p><br>'
 '<button data-step="0">The dig site</button><button data-step="1">The tracked flags</button></section>'
 '<div class="cards">'
 '<section id="original" class="card"><h2>Original</h2><div class="num"></div><p class="state"></p><p class="action"></p></section>'
 '<section id="compared" class="card"><h2>Corrected</h2><div class="num"></div><p class="state"></p><p class="action"></p></section>'
 '</div><p class="summary" id="outcome"></p>'
 '<h2>What the controls establish</h2>'
 '<p>With no dig site beside a permanent wall, the routine finds nothing in either version and the logs match byte for byte, so the correction changes only the excavation case.</p>'
 '<details><summary>Experiment conditions and limits</summary><p>Native Human Druid level 12, depth 30, seed 1000, glowing constructed arena, the Gifts of Nature book carried, Turn Stone to Mud learned, full mana. The vault flag is set directly and a quartz grid is planted beside a permanent wall (recorded fixture intervention) — the state a vault wall run would present. The production excavation routine runs directly; the cast keys are shown, not executed. This isolates the flag marking, not full autonomous play.</p></details>'
 '<details><summary>Recorded native output</summary><h3>Original</h3><pre id="original-log"></pre>'
 '<h3>Corrected</h3><pre id="compared-log"></pre></details>'
 '<p>Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/236">issue236</a>.</p>'
 '</main><script>'
 'let step=1;const data=__DATA__;const select=document.getElementById("scenario");'
 'for(const [key,c] of Object.entries(data)){let o=document.createElement("option");o.value=key;o.textContent=c.label;select.append(o)}'
 'function stateLabel(c){if(!c||!c.state)return"?";return "glow "+c.state.glow+", dark "+c.state.dark}'
 'function draw(){let c=data[select.value];document.getElementById("explanation").textContent=c.explanation;'
 'for(const [panel,phase] of [["original","before"],["compared","after"]]){let d=c[phase],p=document.getElementById(panel);'
 'p.querySelector(".num").textContent=step===0?"Dig site: quartz beside a permanent wall.":stateLabel(d);'
 'p.querySelector(".state").textContent=step===0?"The routine casts stone to mud at the quartz.":"The tracked flags after the dig.";'
 'p.querySelector(".action").textContent=step===1?stateLabel(d):"";'
 'document.getElementById(panel+"-log").textContent=d.raw}'
 'document.getElementById("outcome").textContent=select.value==="excavate"?"The original marks the dug grid glowing; the correction marks it dark.":"Both versions agree in this control; the logs match byte for byte."}'
 'select.addEventListener("change",()=>{step=1;draw()});for(const b of document.querySelectorAll("[data-step]"))b.addEventListener("click",()=>{step=Number(b.dataset.step);draw()});draw();'
 '</script></html>')
for mode,item in data.items():
 html=html.replace('__RAW_'+mode+'__', json.dumps(item))
html=html.replace('const data=__DATA__;','const data='+json.dumps(data).replace('<', '\\u003c')+';')
(root/'replay.html').write_text(html)
(root/'replay-data.json').write_text(json.dumps(data,indent=2)+'\n')
print('Open in your browser:',root/'replay.html')
