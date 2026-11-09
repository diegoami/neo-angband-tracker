"""Loopback-only evidence reader. No game execution, extraction, or analysis."""
from collections import deque
from presentation import project_neo, parse_native_map
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import secrets
import shutil
import subprocess
import tarfile
import tempfile
import threading
import urllib.parse
import webbrowser
import zipfile
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

MAX_PAGE = 500
MAX_TEXT = 2 * 1024 * 1024
SAFE_ARTIFACT = re.compile(r'^(?:cells|raw)-[A-Za-z0-9_.-]+$')


def safe_name(name):
    path = PurePosixPath(name)
    return bool(name) and not path.is_absolute() and '..' not in path.parts and '\\' not in name


def sha_stream(stream):
    digest = hashlib.sha256()
    for chunk in iter(lambda: stream.read(1024 * 1024), b''):
        digest.update(chunk)
    return digest.hexdigest()


class Store:
    def __init__(self, roots, download_root=None, run=None, repo='diegoami/borg-zoo'):
        self.roots = [Path(p).resolve() for p in roots]
        self.download_root = Path(download_root).resolve() if download_root else None
        if self.download_root in self.roots:
            raise ValueError('Download cache must be separate from read-only evidence roots')
        if self.download_root and self.download_root not in self.roots:
            self.roots.append(self.download_root)
        self.run, self.repo = run, repo
        self.rows, self.errors, self.pending = {}, [], []
        self._catalogue_cache = {}
        self._tar_indexes = {}
        self._hashes = {}
        self.lock = threading.RLock()

    def inside(self, path):
        actual = Path(path).resolve()
        if not any(actual.is_relative_to(root) for root in self.roots):
            raise ValueError('Path leaves the selected evidence cache')
        return actual

    def refresh(self):
        with self.lock:
            rows, errors, pending, identities = {}, [], [], set()
            for root in self.roots:
                for folder in sorted(root.glob('cells-*')):
                    try:
                        self.inside(folder)
                        if not (folder / 'download-receipt.json').is_file():
                            pending.append(folder.name + ' (no receipt; only parseable cached records shown)')
                        run_id = None
                        receipt = folder / 'download-receipt.json'
                        if receipt.is_file():
                            metadata = json.loads(receipt.read_text())
                            run_id = metadata.get('run') or metadata.get('artifact', {}).get('workflow_run', {}).get('id')
                        for source in sorted(folder.glob('*.jsonl')):
                            self.inside(source)
                            stamp = (source.stat().st_mtime_ns, source.stat().st_size, run_id)
                            cached = self._catalogue_cache.get(str(source))
                            if cached and cached[0] == stamp:
                                entries = cached[1]
                            else:
                                entries = []
                                with source.open() as handle:
                                    for line_number, line in enumerate(handle):
                                        if not line.strip():
                                            continue
                                        row = json.loads(line)
                                        if row.get('schema') != 'borg-zoo/cell@1':
                                            continue
                                        key = hashlib.sha256(f'{source}:{line_number}'.encode()).hexdigest()[:24]
                                        summary = {k: row.get(k) for k in ['seed', 'outcome', 'cause', 'class', 'race', 'specimen', 'profile', 'suite', 'error', 'stall', 'wall_seconds']}
                                        summary.update(run_id=run_id, record_sha256=hashlib.sha256(line.encode()).hexdigest(), artifact_root=str(root), id=key, final=row.get('final'), source_profile=row.get('harness_kind', 'unknown'), shard=folder.name.removeprefix('cells-'), assessment='unassessed recorded facts')
                                        entries.append((key, summary, str(source), line_number))
                                self._catalogue_cache[str(source)] = (stamp, entries)
                            for key, summary, source_path, line_number in entries:
                                # Multiple explicitly supplied roots may contain the same artifact.
                                identity = summary['record_sha256']
                                if identity in identities:
                                    continue
                                identities.add(identity)
                                item = dict(summary, _identity=identity)
                                rows[key] = (item, source_path, line_number)
                    except (OSError, ValueError, json.JSONDecodeError) as error:
                        errors.append({'artifact': folder.name, 'error': str(error)})
            self.rows, self.errors, self.pending = rows, errors, pending

    def catalogue(self, query):
        self.refresh()
        selected = []
        for summary, _, _ in self.rows.values():
            if any(query.get(k) and query[k].casefold() not in str(summary.get(k, '')).casefold() for k in ['source_profile', 'outcome', 'class', 'race', 'seed', 'cause']):
                continue
            selected.append({k: v for k, v in summary.items() if k != '_identity'})
        selected.sort(key=lambda x: (x['specimen'] or '', int(x['seed'])))
        offset, limit = paging(query)
        facets = {k: sorted({str(row[0].get(k)) for row in self.rows.values() if row[0].get(k) is not None}) for k in ['source_profile', 'outcome', 'class', 'race']}
        return {'total': len(selected), 'cached_total': len(self.rows), 'rows': selected[offset:offset + limit], 'offset': offset, 'limit': limit, 'facets': facets, 'pending_artifacts': self.pending, 'errors': self.errors, 'downloads_enabled': bool(self.run), 'notice': 'Cached completed artifacts only; ongoing or unavailable shards are not deaths. Observations are unassessed.'}

    def cell(self, key):
        if key not in self.rows:
            self.refresh()
        if key not in self.rows:
            raise KeyError('Unknown life id; refresh the catalogue')
        summary, source, line_number = self.rows[key]
        with self.inside(source).open() as handle:
            for index, line in enumerate(handle):
                if index == line_number:
                    return summary, json.loads(line)
        raise KeyError('Catalogue row is no longer available')

    def archive(self, summary):
        name = 'raw-' + summary['shard']
        origin = Path(summary['artifact_root'])
        roots = [origin]
        if self.download_root and str(summary.get('run_id')) == str(self.run) and self.run:
            roots.append(self.download_root)
        for root in roots:
            folder = root / name
            self.inside(folder)
            if root != origin:
                receipt = folder / 'download-receipt.json'
                if not receipt.is_file():
                    continue
                metadata = json.loads(self.inside(receipt).read_text())
                raw_run = metadata.get('run') or metadata.get('artifact', {}).get('workflow_run', {}).get('id')
                if str(raw_run) != str(summary.get('run_id')) or metadata.get('repository', self.repo) != self.repo:
                    continue
            files = sorted(folder.glob('*.tgz')) + sorted(folder.glob('*.tar.gz'))
            if files:
                return self.inside(files[0])
        return None

    def members(self, archive):
        stamp = (str(archive), archive.stat().st_size, archive.stat().st_mtime_ns)
        if stamp not in self._tar_indexes:
            with tarfile.open(archive, 'r:*') as handle:
                members = {}
                for member in handle:
                    name = member.name.removeprefix('./')
                    if not safe_name(name) or member.issym() or member.islnk():
                        continue
                    if member.isfile():
                        if name in members:
                            raise ValueError('Duplicate archive member: ' + name)
                        members[name] = member
            self._tar_indexes[stamp] = members
        return self._tar_indexes[stamp]

    def life(self, key):
        summary, row = self.cell(key)
        archive = self.archive(summary)
        prefix = str(row.get('evidence', {}).get('cell', ''))
        if not safe_name(prefix):
            raise ValueError('Unsafe or absent evidence cell path')
        expected = row.get('evidence', {}).get('files', {})
        files = []
        raw_error = None
        if archive:
            try:
                for name, member in self.members(archive).items():
                    if name.startswith(prefix + '/'):
                        relative = name[len(prefix) + 1:]
                        files.append({'name': relative, 'bytes': member.size, 'expected_sha256': expected.get(relative, {}).get('sha256'), 'sha_status': 'not verified'})
            except (OSError,tarfile.TarError,ValueError) as error:
                raw_error=str(error)
        return {'summary': {k:v for k,v in summary.items() if k != '_identity'}, 'record': row, 'files': files, 'raw_status': 'artifact error' if raw_error else 'available' if archive else 'missing or pending', 'raw_error':raw_error, 'raw_artifact': 'raw-' + summary['shard'], 'downloads_enabled': bool(self.run and str(summary.get('run_id')) == str(self.run)), 'counter_meanings': {'energy': 'native player.total_energy: cumulative energy spent' if summary['source_profile']=='better-borg-c' else 'Neo state.actor.energy: current player energy'}, 'assessment': 'Unassessed observations. No replay or diagnosis has been performed.', 'archive_sha_status': 'not verified' if archive else 'unavailable'}

    def open_member(self, key, relative):
        if not safe_name(relative):
            raise ValueError('Unsafe member path')
        summary, row = self.cell(key)
        archive = self.archive(summary)
        if archive is None:
            raise FileNotFoundError('Raw shard is missing or pending; download explicitly')
        prefix = str(row.get('evidence', {}).get('cell', ''))
        if not safe_name(prefix):
            raise ValueError('Unsafe evidence cell path')
        name = prefix + '/' + relative
        members = self.members(archive)
        if name not in members:
            raise FileNotFoundError('No regular evidence member with that name')
        handle = tarfile.open(archive, 'r:*')
        return handle, handle.extractfile(members[name]), row.get('evidence', {}).get('files', {}).get(relative, {})

    def page(self, key, query):
        relative = query.get('file', '')
        offset, limit = paging(query)
        mode, tail = query.get('mode', 'lines'), query.get('tail') == '1'
        if mode not in ['lines', 'states', 'messages', 'commands']:
            raise ValueError('Unknown view mode')
        archive, stream, expected = self.open_member(key, relative)
        rows, count, consumed = deque(), 0, 0
        try:
            for number, raw in enumerate(iter(lambda: stream.readline(MAX_TEXT + 1), b'')):
                if len(raw) > MAX_TEXT:
                    raw = raw[:MAX_TEXT - 40] + b' [line truncated; fragment]'
                if b'\x00' in raw:
                    return {'binary': True, 'rows': [], 'notice': 'Binary evidence: metadata and hashes only.'}
                text = raw.decode('utf-8', errors='replace').rstrip('\r\n')
                if relative.endswith('trace.txt') and mode != 'lines':
                    starts = {'states': ('STATE ',), 'messages': ('MESSAGE ', 'MSG '), 'commands': ('KEY ', 'COMMAND ', 'BORG ')}[mode]
                    if not text.startswith(starts):
                        continue
                if not tail and count < offset:
                    count += 1
                    continue
                if not tail and (len(rows) >= limit or consumed + len(raw) > MAX_TEXT):
                    return {'rows': [r[0] for r in rows], 'next_offset': count, 'offset': offset, 'truncated': consumed + len(raw) > MAX_TEXT, 'sha_status': 'not verified', 'expected_sha256': expected.get('sha256')}
                parsed = None
                if relative.endswith('.jsonl'):
                    try:
                        parsed = json.loads(text)
                    except json.JSONDecodeError:
                        pass
                elif text.startswith('STATE '):
                    parsed = dict(re.findall(r'(\w+)=([^ ]+)', text))
                rows.append(({'line': number + 1, 'text': text, 'value': parsed}, len(raw)))
                consumed += len(raw)
                count += 1
                if tail:
                    while len(rows) > limit or consumed > MAX_TEXT:
                        consumed -= rows.popleft()[1]
            return {'rows': [r[0] for r in rows], 'next_offset': None, 'offset': max(0, count - len(rows)) if tail else offset, 'total_matching': count if tail else None, 'sha_status': 'not verified', 'expected_sha256': expected.get('sha256')}
        finally:
            stream.close()
            archive.close()

    def verify(self, key):
        summary, row = self.cell(key)
        archive = self.archive(summary)
        if archive is None:
            raise FileNotFoundError('Raw shard is missing or pending')
        stamp = (str(archive), archive.stat().st_size, archive.stat().st_mtime_ns)
        if stamp not in self._hashes:
            with archive.open('rb') as handle:
                self._hashes[stamp] = sha_stream(handle)
        sidecar = archive.with_suffix('.sha256')
        expected_archive = None
        if sidecar.is_file():
            tokens = self.inside(sidecar).read_text().split()
            expected_archive = tokens[0] if tokens else None
        files = {}
        for relative, expected in row.get('evidence', {}).get('files', {}).items():
            try:
                handle, stream, _ = self.open_member(key, relative)
                try:
                    actual = sha_stream(stream)
                finally:
                    stream.close()
                    handle.close()
                files[relative] = {'actual': actual, 'expected': expected.get('sha256'), 'status': 'match' if actual == expected.get('sha256') else 'mismatch' if expected.get('sha256') else 'no expected hash'}
            except (OSError, KeyError, ValueError) as error:
                files[relative] = {'status': 'missing/error', 'error': str(error)}
        return {'archive': {'actual': self._hashes[stamp], 'expected': expected_archive, 'status': 'match' if expected_archive == self._hashes[stamp] else 'mismatch' if expected_archive else 'no expected hash'}, 'files': files, 'notice': 'Integrity comparison only; this does not authenticate source claims or diagnose play.'}

    def presentation(self, key):
        summary,row=self.cell(key)
        try:
            return self._presentation(key)
        except (OSError,tarfile.TarError,ValueError) as error:
            value=project_neo(row,{}) if summary['source_profile']=='neo-original' else parse_native_map(row,'')
            value['source_notice']='Raw presentation unavailable: '+str(error)+'. Showing compact final fields only; missing current values are not recorded.'
            return value

    def _presentation(self, key):
        summary,row=self.cell(key)
        if not self.archive(summary):
            value=project_neo(row,{}) if summary['source_profile']=='neo-original' else parse_native_map(row,'')
            value['source_notice']='Raw evidence is missing or pending. Showing compact final fields only; current inventory and unavailable stats are not recorded.'
            return value
        if summary['source_profile']=='neo-original':
            handle,stream,_=self.open_member(key,'run/run.json')
            try:
                content=stream.read(4*MAX_TEXT+1)
                if len(content)>4*MAX_TEXT:raise ValueError('Record too large for presentation; use raw pages')
                return project_neo(row,json.loads(content))
            finally:
                stream.close();handle.close()
        files=self.life(key)['files']
        map_file=next((f['name'] for f in files if f['name'].endswith('.map')),None)
        if not map_file:
            return parse_native_map(row,'')
        handle,stream,_=self.open_member(key,map_file)
        try:
            content=stream.read(4*MAX_TEXT+1)
            if len(content)>4*MAX_TEXT:raise ValueError('Map too large for presentation; use raw pages')
            return parse_native_map(row,content.decode('utf-8',errors='replace'))
        finally:
            stream.close();handle.close()

    def observation(self, key, field):
        if field not in ['finalInventory', 'finalKnownWorld', 'birth', 'provenance']:
            raise ValueError('Unknown observation field')
        summary, row = self.cell(key)
        if summary['source_profile'] != 'neo-original':
            return {'available': False, 'notice': 'Native inventory/map observations remain in archive/*.map and result.json; no structured field is invented.'}
        handle, stream, _ = self.open_member(key, 'run/run.json')
        try:
            content = stream.read(4 * MAX_TEXT + 1)
            if len(content) > 4 * MAX_TEXT:
                raise ValueError('Record too large for structured preview; use paged raw file')
            record = json.loads(content)
        finally:
            stream.close()
            handle.close()
        return {'available': field in record, 'value': record.get(field), 'notice': 'Original recorded field only; no reconstruction.'}

    def fetch(self, artifact_name=None):
        if not self.run or not self.download_root:
            raise ValueError('Downloads require explicit --github-run at startup')
        args = ['gh', 'api', f'repos/{self.repo}/actions/runs/{self.run}/artifacts', '--paginate', '--jq', '.artifacts[] | {id,name,expired,digest}']
        result = subprocess.run(args, check=True, capture_output=True, text=True, timeout=90)
        artifacts = [json.loads(line) for line in result.stdout.splitlines() if line.strip()]
        selected = [a for a in artifacts if a['name'] == artifact_name] if artifact_name else [a for a in artifacts if a['name'].startswith('cells-')]
        if artifact_name and not selected:
            raise FileNotFoundError('Artifact not published yet or unavailable')
        self.download_root.mkdir(parents=True, exist_ok=True)
        statuses = []
        for artifact in selected:
            name = artifact['name']
            if not SAFE_ARTIFACT.fullmatch(name) or artifact['expired']:
                statuses.append({'name': name, 'status': 'expired/unsupported'})
                continue
            final = self.download_root / name
            if final.exists():
                statuses.append({'name': name, 'status': 'already cached; preserved'})
                continue
            with tempfile.TemporaryDirectory(prefix='.download-', dir=self.download_root) as temporary:
                temp = Path(temporary)
                with (temp / 'artifact.zip').open('wb') as target:
                    subprocess.run(['gh', 'api', f'repos/{self.repo}/actions/artifacts/{artifact["id"]}/zip'], stdout=target, check=True, timeout=180)
                with (temp / 'artifact.zip').open('rb') as downloaded:
                    zip_hash = sha_stream(downloaded)
                expected_zip = artifact.get('digest')
                if expected_zip and expected_zip != 'sha256:' + zip_hash:
                    raise ValueError('Downloaded GitHub artifact digest mismatch')
                with zipfile.ZipFile(temp / 'artifact.zip') as zipped:
                    for member in zipped.infolist():
                        if member.is_dir():
                            continue
                        if not safe_name(member.filename) or len(PurePosixPath(member.filename).parts) != 1 or member.file_size > 4 * 1024**3:
                            raise ValueError('Unsafe artifact ZIP member')
                        with zipped.open(member) as source, (temp / member.filename).open('xb') as target:
                            shutil.copyfileobj(source, target)
                (temp / 'download-receipt.json').write_text(json.dumps({'artifact': artifact, 'run': self.run, 'repository': self.repo, 'zipSha256': zip_hash}) + '\n')
                # Cache is append-only; no existing destination is replaced.
                if final.exists():
                    raise FileExistsError('Artifact appeared while downloading; preserved')
                temp.rename(final)
            statuses.append({'name': name, 'status': 'downloaded'})
        return statuses


def paging(query):
    offset, limit = int(query.get('offset', 0)), int(query.get('limit', 100))
    if offset < 0 or offset > 10000000 or limit < 1 or limit > MAX_PAGE:
        raise ValueError('Invalid page bounds')
    return offset, limit


def make_handler(store, token):
    html=Path(__file__).with_name('index.html').read_text().replace('__TOKEN__',token).encode()
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            pass

        def send_json(self, status, payload):
            body = json.dumps(payload, ensure_ascii=False).encode()
            self.send_response(status)
            self.send_header('Content-Type', 'application/json; charset=utf-8')
            self.send_header('Content-Length', str(len(body)))
            self.send_header('Cache-Control', 'no-store')
            self.send_header('X-Content-Type-Options', 'nosniff')
            self.end_headers()
            self.wfile.write(body)

        def trusted_host(self):
            return self.headers.get('Host', '').split(':')[0] in ['127.0.0.1', 'localhost']

        def do_GET(self):
            if not self.trusted_host():
                return self.send_json(403, {'error': 'Loopback host only'})
            url = urllib.parse.urlsplit(self.path)
            query = dict(urllib.parse.parse_qsl(url.query))
            try:
                if url.path == '/':
                    body = html
                    self.send_response(200)
                    self.send_header('Content-Type', 'text/html; charset=utf-8')
                    self.send_header('Content-Length', str(len(body)))
                    self.send_header('Content-Security-Policy', "default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; img-src 'self'")
                    self.end_headers()
                    self.wfile.write(body)
                elif url.path == '/api/lives':
                    self.send_json(200, store.catalogue(query))
                elif url.path == '/api/life':
                    self.send_json(200, store.life(query['id']))
                elif url.path == '/api/file':
                    self.send_json(200, store.page(query['id'], query))
                elif url.path == '/api/presentation':
                    self.send_json(200, store.presentation(query['id']))
                elif url.path == '/api/observation':
                    self.send_json(200, store.observation(query['id'], query['field']))
                elif url.path == '/api/verify':
                    self.send_json(200, store.verify(query['id']))
                else:
                    self.send_json(404, {'error': 'Unknown endpoint'})
            except (KeyError, ValueError, OSError, tarfile.TarError) as error:
                self.send_json(400, {'error': str(error)})

        def do_POST(self):
            if not self.trusted_host() or self.headers.get('X-Browser-Token') != token:
                return self.send_json(403, {'error': 'Explicit local browser action required'})
            url = urllib.parse.urlsplit(self.path)
            query = dict(urllib.parse.parse_qsl(url.query))
            try:
                if url.path == '/api/catalogues':
                    return self.send_json(200, {'downloads': store.fetch()})
                if url.path != '/api/download':
                    return self.send_json(404, {'error': 'Unknown endpoint'})
                summary, _ = store.cell(query['id'])
                if str(summary.get('run_id')) != str(store.run):
                    raise ValueError('Selected cache has no matching workflow-run receipt; download from its original run separately')
                self.send_json(200, {'downloads': store.fetch('raw-' + summary['shard'])})
            except (KeyError, ValueError, OSError, subprocess.SubprocessError) as error:
                self.send_json(400, {'error': str(error)})
    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', action='append', default=[], help='Existing evidence cache, read-only; may be repeated')
    parser.add_argument('--github-run', help='Explicitly fetch compact catalogues for this GitHub run; raw shards only on button click')
    parser.add_argument('--repository', default='diegoami/borg-zoo')
    parser.add_argument('--download-cache', help='Separate append-only download cache')
    parser.add_argument('--port', type=int, default=0)
    parser.add_argument('--no-open', action='store_true')
    args = parser.parse_args()
    if not args.cache and not args.github_run:
        parser.error('Choose --cache or --github-run')
    if args.github_run and not args.github_run.isdigit():
        parser.error('--github-run must be numeric')
    if not re.fullmatch(r'[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+', args.repository):
        parser.error('Invalid repository')
    for selected in args.cache:
        if not Path(selected).is_dir():
            parser.error('Cache directory not found: ' + selected + '; use --github-run RUN_ID to download catalogues')
    destination = args.download_cache or str(Path.home() / '.cache/neo-borg-log-browser' / str(args.github_run or 'local'))
    store = Store(args.cache, destination if args.github_run else None, args.github_run, args.repository)
    if args.github_run:
        print(json.dumps({'catalogue_downloads': store.fetch()}), flush=True)
    token = secrets.token_hex(24)
    server = ThreadingHTTPServer(('127.0.0.1', args.port), make_handler(store, token))
    url = f'http://127.0.0.1:{server.server_port}/'
    print(url, flush=True)
    if not args.no_open:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
