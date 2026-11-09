"""Offline format/security regressions; fixtures represent recorded schemas, not new lives."""
import hashlib
import io
import json
from pathlib import Path
import tarfile
import tempfile
import threading
import unittest
import urllib.error
import urllib.request
from http.server import ThreadingHTTPServer
from unittest.mock import patch
import server


def digest(content):
    return hashlib.sha256(content).hexdigest()


def fixture(root, neo=True, receipt=True, cause='recorded cause', extra=None, outcome='stalled'):
    specimen = 'neo-5a9-original' if neo else 'angband-e473-original'
    shard = specimen + '-0'
    cell = f'classes-1800/{specimen}/default/123'
    commands = b'{"accepted":false,"intent":{"kind":"command","command":{"code":"shop-exit"}},"reason":"store is not open","turn":42,"type":"intent"}\n'
    messages = b'{"turn":42,"text":"You cannot see anything."}\n'
    states = b'{"turn":42,"hp":-1,"maxHp":12,"depth":1,"maxDepth":2,"level":1,"maxLevel":1,"energy":15,"decisions":100}\n'
    trace = b'BORG 1 recorded diagnostic\nMESSAGE 42 You die.\nSTATE death turn=42 decisions=100 depth=1 max_depth=2 level=1 max_level=1 hp=-1 energy=15\n'
    record = {'outcome': outcome, 'finalInventory': [{'description':'a Dagger'}], 'finalKnownWorld':{'player':{'hp':-1}}, 'causeOfDeath':cause}
    content = {'run/commands.jsonl':commands,'run/messages.jsonl':messages,'run/states.jsonl':states,'run/run.json':json.dumps(record).encode()} if neo else {'trace.txt':trace,'archive/Measure.map':b'@..\n...\n','result.json':json.dumps(record).encode(),'save/Measure':b'\0binary'}
    if extra:
        content.update(extra)
    row = {'schema':'borg-zoo/cell@1','seed':123,'outcome':outcome,'cause':cause,'class':'Mage','race':'Dwarf','specimen':specimen,'suite':'classes-1800','profile':'default','harness_kind':'neo-original' if neo else 'better-borg-c','final':{'turn':42,'hp':-1},'evidence':{'cell':cell,'files':{name:{'sha256':digest(data),'bytes':len(data)} for name,data in content.items()}}}
    folder = root / ('cells-' + shard)
    folder.mkdir(parents=True)
    (folder / (shard + '.jsonl')).write_text(json.dumps(row)+'\n')
    if receipt:
        (folder / 'download-receipt.json').write_text(json.dumps({'run':999}))
    raw = root / ('raw-' + shard)
    raw.mkdir()
    archive = raw / (raw.name + '.tgz')
    with tarfile.open(archive,'w:gz') as tar:
        for name,data in content.items():
            item = tarfile.TarInfo('./'+cell+'/'+name)
            item.size=len(data)
            tar.addfile(item,io.BytesIO(data))
        evil=tarfile.TarInfo('../../outside.txt');evil.size=4;tar.addfile(evil,io.BytesIO(b'evil'))
        link=tarfile.TarInfo('./'+cell+'/link');link.type=tarfile.SYMTYPE;link.linkname='/etc/passwd';tar.addfile(link)
    archive.with_suffix('.sha256').write_text(digest(archive.read_bytes())+'  '+archive.name+'\n')
    if receipt:
        (raw/'download-receipt.json').write_text('{}')
    return row, archive


class BrowserTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.root=Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def key(self,store,**filters):
        return store.catalogue(filters)['rows'][0]['id']

    def test_standard_gh_without_receipts_and_neo_rejected_command(self):
        fixture(self.root,receipt=False)
        store=server.Store([self.root]);key=self.key(store,outcome='stalled',seed='123',source_profile='neo')
        self.assertEqual(store.life(key)['raw_status'],'available')
        row=store.page(key,{'file':'run/commands.jsonl'})['rows'][0]['value']
        self.assertFalse(row['accepted']);self.assertEqual(row['reason'],'store is not open')
        self.assertEqual(store.observation(key,'finalInventory')['value'][0]['description'],'a Dagger')

    def test_native_death_map_states_and_no_invented_commands(self):
        fixture(self.root,neo=False,outcome='death')
        store=server.Store([self.root]);key=self.key(store)
        state=store.page(key,{'file':'trace.txt','mode':'states'})['rows'][0]['value']
        self.assertEqual(state['hp'],'-1');self.assertEqual(state['energy'],'15')
        self.assertIn('@..',store.page(key,{'file':'archive/Measure.map'})['rows'][0]['text'])
        self.assertTrue(store.page(key,{'file':'save/Measure'})['binary'])
        self.assertFalse(store.observation(key,'finalInventory')['available'])

    def test_hashes_match_then_mismatch_and_missing(self):
        row,archive=fixture(self.root)
        store=server.Store([self.root]);key=self.key(store)
        report=store.verify(key)
        self.assertEqual(report['archive']['status'],'match')
        self.assertTrue(all(f['status']=='match' for f in report['files'].values()))
        archive.with_suffix('.sha256').write_text('0'*64+'\n')
        self.assertEqual(store.verify(key)['archive']['status'],'mismatch')
        row['evidence']['files']['missing.txt']={'sha256':'0'*64}
        source=next(self.root.glob('cells-*/*.jsonl'));source.write_text(json.dumps(row)+'\n')
        store.refresh();self.assertEqual(store.verify(key)['files']['missing.txt']['status'],'missing/error')

    def test_pagination_tail_and_long_line_make_progress(self):
        content=b''.join((json.dumps({'turn':i})+'\n').encode() for i in range(205))
        fixture(self.root,extra={'run/messages.jsonl':content,'long.txt':b'x'*(server.MAX_TEXT+100)+b'\nend\n'})
        store=server.Store([self.root]);key=self.key(store)
        first=store.page(key,{'file':'run/messages.jsonl','limit':'100'})
        self.assertEqual(first['next_offset'],100)
        tail=store.page(key,{'file':'run/messages.jsonl','tail':'1','limit':'100'})
        self.assertEqual(tail['offset'],105);self.assertEqual(tail['rows'][-1]['value']['turn'],204)
        long=store.page(key,{'file':'long.txt','limit':'1'})
        self.assertTrue(long['rows']);self.assertGreater(long['next_offset'],0)
        second=store.page(key,{'file':'long.txt','offset':str(long['next_offset']),'limit':'1'})
        self.assertTrue(second['rows'])

    def test_two_cohorts_same_names_keep_distinct_records_and_own_raw(self):
        a=self.root/'a';b=self.root/'b';a.mkdir();b.mkdir()
        fixture(a,cause='first',outcome='death');fixture(b,cause='second',outcome='stalled')
        store=server.Store([a,b]);rows=store.catalogue({})['rows']
        self.assertEqual(len(rows),2)
        for row in rows:
            self.assertEqual(store.observation(row['id'],'finalKnownWorld')['value']['player']['hp'],-1)
            raw=store.page(row['id'],{'file':'run/run.json'})['rows'][0]['text']
            self.assertEqual(json.loads(raw)['causeOfDeath'],row['cause'])

    def test_unsafe_members_paths_links_missing_pending_and_page_bounds(self):
        fixture(self.root)
        store=server.Store([self.root]);key=self.key(store)
        self.assertFalse(any(f['name']=='link' for f in store.life(key)['files']))
        for name in ['../../outside.txt','/etc/passwd','link']:
            with self.assertRaises((ValueError,FileNotFoundError)):
                store.page(key,{'file':name})
        for query in [{'offset':'-1'},{'limit':'501'}]:
            with self.assertRaises(ValueError):store.catalogue(query)
        pending=self.root/'cells-pending-0';pending.mkdir()
        self.assertTrue(store.catalogue({})['pending_artifacts'])
        for p in self.root.glob('raw-*/*.tgz'):p.unlink()
        self.assertEqual(store.life(key)['raw_status'],'missing or pending')

    def test_receipt_arrival_updates_run_identity(self):
        fixture(self.root,receipt=False)
        store=server.Store([self.root]);key=self.key(store)
        self.assertIsNone(store.life(key)['summary']['run_id'])
        folder=next(self.root.glob('cells-*'))
        (folder/'download-receipt.json').write_text(json.dumps({'run':999}))
        store.refresh()
        self.assertEqual(store.life(key)['summary']['run_id'],999)

    def test_foreign_download_cache_cannot_substitute_raw(self):
        origin=self.root/'origin';foreign=self.root/'foreign';origin.mkdir();foreign.mkdir()
        fixture(origin,cause='correct');fixture(foreign,cause='wrong')
        for path in origin.glob('raw-*/*.tgz'):path.unlink()
        for path in foreign.glob('raw-*/download-receipt.json'):path.write_text(json.dumps({'run':111}))
        store=server.Store([origin],foreign,run='999');key=self.key(store)
        self.assertEqual(store.life(key)['raw_status'],'missing or pending')
        with self.assertRaises(ValueError):server.Store([origin],origin,run='999')

    def test_compact_character_sheet_survives_missing_or_corrupt_raw(self):
        row,archive=fixture(self.root)
        store=server.Store([self.root]);key=self.key(store)
        archive.write_bytes(b'not a tar archive')
        self.assertEqual(store.life(key)['raw_status'],'artifact error')
        self.assertEqual(store.presentation(key)['sheet']['hp'],-1)
        self.assertEqual(store.presentation(key)['groups'],[])
        archive.unlink()
        self.assertEqual(store.presentation(key)['sheet']['outcome'],'stalled')

    def test_http_loopback_api_and_download_is_explicit(self):
        fixture(self.root)
        store=server.Store([self.root]);http=ThreadingHTTPServer(('127.0.0.1',0),server.make_handler(store,'secret'))
        thread=threading.Thread(target=http.serve_forever,daemon=True);thread.start()
        url=f'http://127.0.0.1:{http.server_port}'
        try:
            data=json.load(urllib.request.urlopen(url+'/api/lives?seed=123'))
            self.assertEqual(data['total'],1)
            with self.assertRaises(urllib.error.HTTPError) as error:
                urllib.request.urlopen(urllib.request.Request(url+'/api/download?id='+data['rows'][0]['id'],method='POST'))
            self.assertEqual(error.exception.code,403);error.exception.close()
            request=urllib.request.Request(url+'/api/lives',headers={'Host':'evil.example'})
            with self.assertRaises(urllib.error.HTTPError) as error:urllib.request.urlopen(request)
            error.exception.close()
        finally:
            http.shutdown();http.server_close();thread.join()

    def test_no_network_on_catalogue_life_and_file(self):
        fixture(self.root)
        store=server.Store([self.root])
        with patch('server.subprocess.run',side_effect=AssertionError('network invoked')):
            key=self.key(store);store.life(key);store.page(key,{'file':'run/commands.jsonl'})
        with self.assertRaises(ValueError):store.fetch()


if __name__=='__main__':unittest.main()
