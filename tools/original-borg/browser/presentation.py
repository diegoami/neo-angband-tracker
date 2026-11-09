"""Conservative display projections of recorded evidence, never game inference."""
import re

TERRAIN = {'FLOOR':'.','GRANITE':'#','PERM':'#','QUARTZ':'#','MAGMA':'#','QUARTZ_K':'#','MAGMA_K':'#','UP':'<','DOWN':'>','LESS':'<','MORE':'>','OPEN':"'",'BROKEN':"'",'CLOSED':'+','SECRET':'#','RUBBLE':':','PASS_RUBBLE':':','LAVA':'~','NONE':' '}


def terrain_symbol(code):
    if code in TERRAIN:
        return TERRAIN[code]
    if isinstance(code,str) and (code.startswith('STORE_') or code=='HOME'):
        return '+'
    return '?'


def known_map(world):
    level=world.get('level') or {}
    width,height=level.get('width'),level.get('height')
    if not isinstance(width,int) or not isinstance(height,int) or not (0<width<=1000 and 0<height<=1000):
        return None
    grid=[[' ']*width for _ in range(height)]
    items=[]
    for cell in level.get('cells',[]):
        x,y=cell.get('x'),cell.get('y')
        if not isinstance(x,int) or not isinstance(y,int) or not (0<=x<width and 0<=y<height):
            continue
        remembered=cell.get('remembered')
        if not isinstance(remembered,dict):
            continue
        grid[y][x]=terrain_symbol(remembered.get('featCode'))
        if remembered.get('traps'):
            grid[y][x]='^'
        if remembered.get('items'):
            grid[y][x]='*'
            items.append({'x':x,'y':y,'items':remembered['items']})
    for monster in world.get('monsters') or []:
        location=monster.get('grid') or {}
        x,y=location.get('x'),location.get('y')
        glyph=monster.get('glyph')
        if monster.get('visible') is True and isinstance(glyph,str) and len(glyph)==1 and isinstance(x,int) and isinstance(y,int) and 0<=x<width and 0<=y<height:
            grid[y][x]=glyph
    player=(world.get('player') or {}).get('grid') or {}
    x,y=player.get('x'),player.get('y')
    if isinstance(x,int) and isinstance(y,int) and 0<=x<width and 0<=y<height:
        grid[y][x]='@'
    return {'width':width,'height':height,'rows':[''.join(line) for line in grid],'items':items,'player':player,'legend':'Viewer terrain projection from recorded featCode: # wall, . floor, < > stairs, + door/store, ~ lava, : rubble, ^ recorded trap, * recorded item, ? unmapped recorded terrain. Blank cells are unknown/unrecorded (or recorded NONE). Only monsters recorded visible are plotted. Monster glyphs are recorded; @ is the recorded player position. Original cell glyphs/colors were not recorded.'}


def item_groups(world, final_inventory):
    groups=[]
    if isinstance(world.get('equipment'),list):
        slots=world.get('equipmentSlots') or []
        equipment=[]
        for index,item in enumerate(world['equipment']):
            equipment.append({'row':chr(97+index)+')' if index<26 else str(index),'slot':slots[index].get('name') if index<len(slots) else None,'slot_index':index,'item':item,'description':item.get('name') if item else '(nothing)','color':item.get('nameColor') if item else None,'handle':item.get('handle') if item else None})
        groups.append({'title':'Equipment','rows':equipment})
    for key,title in [('inventory','Pack'),('quiver','Quiver')]:
        if not isinstance(world.get(key),list):
            continue
        entries=[]
        for index,item in enumerate(world[key]):
            entries.append({'row':chr(97+index)+')' if index<26 else str(index),'description':item.get('name') if item else '(nothing)','color':item.get('nameColor') if item else None,'handle':item.get('handle') if item else None,'item':item})
        groups.append({'title':title,'rows':entries})
    if not groups and final_inventory:
        for kind,title in [('equipped','Equipment'),('pack','Pack'),('quiver','Quiver')]:
            rows=[{'row':str(item.get('handle','?')),'slot_index':item.get('slot'),'description':item.get('description'),'item':item,'handle':item.get('handle')} for item in final_inventory if item.get('location')==kind]
            groups.append({'title':title,'rows':rows})
    return groups


def sheet_base(row):
    final=row.get('final') or {}
    return {'name':None,'race':row.get('played_race') or row.get('race'),'class':row.get('played_class') or row.get('class'),'level':final.get('level'),'max_level':final.get('max_level'),'hp':final.get('hp'),'max_hp':final.get('max_hp'),'sp':None,'max_sp':None,'depth':final.get('depth'),'max_depth':final.get('max_depth'),'gold':None,'exp':None,'ac':None,'speed':None,'turn':final.get('turn'),'stats':None,'status':[],'outcome':row.get('outcome'),'cause':row.get('cause'),'stall':row.get('stall'),'error':row.get('error'),'source_profile':row.get('harness_kind'),'seed':row.get('seed')}


def project_neo(row, record):
    world=record.get('finalKnownWorld') or {}
    player=world.get('player') or {}
    sheet=sheet_base(row)
    for source,target in [('fullName','name'),('race','race'),('cls','class'),('level','level'),('maxLevel','max_level'),('hp','hp'),('maxHp','max_hp'),('sp','sp'),('maxSp','max_sp'),('depth','depth'),('maxDepth','max_depth'),('gold','gold'),('exp','exp'),('ac','ac'),('speed','speed')]:
        if source in player:
            sheet[target]=player[source]
    sheet['stats']=list(player['stats']) if isinstance(player.get('stats'),list) else None
    sheet['status']=[str(value) for value in (player.get('status',{}).get('timed') or {}).values() if value is not None]
    return {'sheet':sheet,'groups':item_groups(world,record.get('finalInventory')),'map':known_map(world),'inventory_notice':'Letters are viewer row labels, not recorded menu keybindings. Item names, stacks, slot names, handles and any nameColor labels come from the record.','source_notice':'Final Neo KnownWorld snapshot; only recorded knowledge is displayed. Missing values remain not recorded.'}


def parse_native_map(row, text):
    lines=text.splitlines()
    sheet=sheet_base(row)
    for line in lines[:5]:
        name=re.match(r'(.+?) the .+, Level (\d+)/(\d+)$',line)
        if name:
            sheet['name']=name.group(1)
        money=re.match(r'Exp: (\d+)\s+Gold: (\d+)\s+Turn: (\d+)',line)
        if money:
            sheet['exp'],sheet['gold']=int(money.group(1)),int(money.group(2))
    groups=[]
    section=None
    for line in lines:
        title=re.match(r'^\s*\[([^]]+)\]\s*$',line)
        if title:
            section=None
            if title.group(1) in ['Character Equipment','Character Quiver','Character Inventory']:
                section={'title':title.group(1).removeprefix('Character '),'rows':[]}
                groups.append(section)
        elif section is not None:
            item=re.match(r'^([A-Za-z])\) (.*)$',line)
            if item:
                section['rows'].append({'row':item.group(1)+')','description':item.group(2),'color':None})
    first=next((i for i,line in enumerate(lines) if re.fullmatch(r'#{5,}',line)),None)
    map_data=None
    if first is not None:
        width=len(lines[first]);end=first+1
        while end<len(lines) and len(lines[end])==width and lines[end].startswith('#') and lines[end].endswith('#'):
            end+=1
        if end>first+1 and re.fullmatch(r'#{5,}',lines[end-1]):
            rows=lines[first:end]
            player=next(({'x':line.index('@'),'y':i} for i,line in enumerate(rows) if '@' in line),None)
            map_data={'rows':rows,'width':width,'height':len(rows),'player':player,'legend':'Exact recorded native Borg ASCII dump. No terrain, visibility, glyph or color reconstruction.'}
    return {'sheet':sheet,'groups':groups,'map':map_data,'inventory_notice':'Original native dump item letters and descriptions are preserved verbatim. This dump is a recorded Borg observation, not newly inspected save data.','source_notice':'Final HP/depth/outcome come from the final run record. EXP/gold and item sections come from the recorded map dump. Stale CharOutput and pre-death message HP are not used.'}
