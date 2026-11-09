import unittest
from presentation import known_map, parse_native_map, project_neo


class PresentationTests(unittest.TestCase):
    def row(self,outcome='death'):
        return {'race':'Hobbit','class':'Druid','seed':123,'outcome':outcome,'cause':'poison' if outcome=='death' else None,'harness_kind':'better-borg-c','final':{'hp':-1 if outcome=='death' else 15,'level':2,'max_level':3,'depth':1,'max_depth':2,'turn':999},'birth_record':{'hp':19,'gold':500,'inventory':[{'name':'birth-only dagger'}]}}

    def test_native_headers_items_letters_and_final_negative_hp(self):
        text='Measure the Hobbit Druid, Level 2/3\nExp: 100  Gold: 8  Turn: 999\nKilled on level: 1 (max. 2) by poison\n#####\n# @ #\n#####\n  [Last Messages]\n# Loc: HP:10/19\n  [Character Equipment]\na) Whip (1d3)\ni) Cloak\nm) (nothing)\n  [Character Quiver]\na) (nothing)\n  [Character Inventory]\na) 7 Rations of Food\n  [Player State at Death]\nskill 45 (gold) value=0.\n'
        result=parse_native_map(self.row(),text)
        self.assertEqual(result['sheet']['hp'],-1)
        self.assertIsNone(result['sheet']['max_hp'])
        self.assertEqual(result['sheet']['gold'],8)
        self.assertEqual(result['sheet']['exp'],100)
        self.assertEqual([r['row'] for r in result['groups'][0]['rows']],['a)','i)','m)'])
        self.assertEqual(result['groups'][2]['rows'][0]['description'],'7 Rations of Food')
        self.assertEqual(result['map']['rows'],['#####','# @ #','#####'])

    def test_internal_all_wall_row_does_not_truncate_native_frame(self):
        result=parse_native_map(self.row(),'#####\n#.@.#\n#####\n#...#\n#####\n  [Last Messages]\n')
        self.assertEqual(result['map']['height'],5)
        self.assertEqual(result['map']['rows'][3],'#...#')

    def test_native_alive_legacy_killed_header_does_not_create_death(self):
        result=parse_native_map(self.row('turn_cap'),'Measure the Hobbit Druid, Level 2/3\nExp: 20 Gold: 10 Turn: 999\nKilled on level: 2 (max. 3) by (alive and well)\n')
        self.assertEqual(result['sheet']['outcome'],'turn_cap')
        self.assertIsNone(result['sheet']['cause'])
        self.assertEqual(result['sheet']['hp'],15)

    def test_unknown_stats_and_gear_do_not_promote_birth_values(self):
        result=project_neo(self.row(),{})
        self.assertIsNone(result['sheet']['gold'])
        self.assertIsNone(result['sheet']['max_hp'])
        self.assertIsNone(result['sheet']['sp'])
        self.assertIsNone(result['sheet']['stats'])
        self.assertEqual(result['groups'],[])
        self.assertIsNone(result['map'])

    def test_neo_recorded_current_fields_slots_and_empty_slots(self):
        world={'player':{'stats':[10,11,12,13,14],'gold':12,'hp':-2,'maxHp':20,'exp':77,'sp':1,'maxSp':4,'status':{'timed':{'BLIND':'Blind','SLOW':None}}},'equipmentSlots':[{'name':'weapon'},{'name':'shooting'}],'equipment':[{'handle':4,'name':'a Whip','nameColor':'white'},None],'inventory':[{'handle':7,'name':'3 Rations of Food','number':3,'nameColor':'light umber'}],'quiver':[]}
        result=project_neo(self.row(),{'finalKnownWorld':world})
        self.assertEqual(result['sheet']['stats'],[10,11,12,13,14]);self.assertEqual(result['sheet']['hp'],-2);self.assertEqual(result['sheet']['gold'],12)
        self.assertEqual(result['sheet']['status'],['Blind'])
        self.assertEqual(result['groups'][0]['rows'][0]['slot'],'weapon')
        self.assertEqual(result['groups'][0]['rows'][1]['description'],'(nothing)')
        self.assertEqual(result['groups'][1]['rows'][0]['description'],'3 Rations of Food')
        self.assertIn('viewer row labels',result['inventory_notice'])
        self.assertEqual(project_neo(self.row(),{'finalKnownWorld':{'inventory':None,'equipment':None}})['groups'],[])

    def test_map_unknown_blank_recorded_terrain_items_and_monster_visibility(self):
        world={'level':{'width':5,'height':3,'cells':[{'x':0,'y':0,'remembered':{'featCode':'GRANITE'}},{'x':1,'y':0,'remembered':{'featCode':'FLOOR','items':[{'name':'potion'}]}},{'x':2,'y':0,'remembered':{'featCode':'DOWN'}},{'x':3,'y':0,'remembered':{'featCode':'UNKNOWN_RECORD'}},{'x':4,'y':0,'remembered':{'featCode':'FLOOR'}}]},'player':{'grid':{'x':2,'y':2}},'monsters':[{'visible':True,'glyph':'a','grid':{'x':4,'y':0}},{'visible':False,'glyph':'D','grid':{'x':4,'y':1}}]}
        result=known_map(world)
        self.assertEqual(result['rows'],['#*>?a','     ','  @  '])
        self.assertIn('Original cell glyphs/colors were not recorded',result['legend'])
        self.assertEqual(result['player'],{'x':2,'y':2})
        self.assertIsNone(known_map({'level':{'width':10001,'height':2}}))


if __name__=='__main__':unittest.main()
