import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile


def pe_info(data):
    assert data[:2] == b'MZ', 'Missing DOS executable header.'
    offset = struct.unpack_from('<I', data, 0x3C)[0]
    assert data[offset:offset + 4] == b'PE\0\0', 'Missing PE signature.'
    machine = struct.unpack_from('<H', data, offset + 4)[0]
    magic = struct.unpack_from('<H', data, offset + 24)[0]
    subsystem = struct.unpack_from('<H', data, offset + 24 + 68)[0]
    assert machine == 0x8664 and magic == 0x20B, 'The image is not AMD64 PE32+.'
    return {'machine': 'AMD64', 'subsystem': subsystem}


parser = argparse.ArgumentParser()
parser.add_argument('archive', type=Path)
parser.add_argument('--commit', required=True)
args = parser.parse_args()
with zipfile.ZipFile(args.archive) as package:
    assert package.testzip() is None, 'Archive CRC validation failed.'
    members = {item.filename.replace('\\', '/'): item for item in package.infolist()}
    root = 'AuroraGlass/'
    required = {'Telegram.exe', 'Start Aurora Glass.cmd', 'LICENSE', 'LEGAL',
                'README.md', 'IMPLEMENTATION.md', 'MOTION.md', 'BUILD-INFO.json', 'imports.txt',
                'modules/x64/d3d/d3dcompiler_47.dll'}
    missing = sorted(root + name for name in required if root + name not in members)
    assert not missing, f'Missing package files: {missing}; archive entries: {sorted(members)}'
    for name in members:
        path = Path(name)
        parts = {part.lower() for part in path.parts}
        assert name.startswith(root) and '..' not in parts, 'Unexpected archive path.'
        assert not parts.intersection({'tdata', 'profile', '.git'}), 'Local application data was packaged.'
        assert path.name.lower() not in {'updater.exe', 'cmakecache.txt', 'fork-api.cmake', 'log.txt'}, 'A private or excluded file was packaged.'
        assert path.suffix.lower() not in {'.pdb', '.dmp'}, 'A debug artifact was packaged.'
    executable = package.read(members[root + 'Telegram.exe'])
    executable_hash = hashlib.sha256(executable).hexdigest()
    provenance = json.loads(package.read(members[root + 'BUILD-INFO.json']))
    assert provenance['source_commit'] == args.commit, 'The package source commit does not match the build.'
    assert provenance['executable_sha256'] == executable_hash, 'The executable does not match its build provenance.'
    main_info = pe_info(executable)
    assert main_info['subsystem'] == 2, 'The client is not a Windows GUI executable.'
    brand_count = executable.count('Aurora Glass for Telegram\0'.encode('utf-16le'))
    assert brand_count >= 2, 'Fork product/description resources are not active.'
    launcher = package.read(members[root + 'Start Aurora Glass.cmd']).decode('ascii')
    assert '-workdir "%~dp0profile"' in launcher, 'The launcher does not isolate its profile.'
    assert '"%~dp0Telegram.exe"' in launcher, 'The launcher does not use its own executable.'
    dlls = {}
    for name, item in members.items():
        if name.lower().endswith('.dll'):
            dlls[name] = pe_info(package.read(item))['machine']
    report = {
        'source_commit': args.commit,
        'archive': str(args.archive),
        'sha256': hashlib.sha256(args.archive.read_bytes()).hexdigest(),
        'archive_crc': 'passed',
        'file_count': sum(not item.is_dir() for item in members.values()),
        'client': main_info,
        'executable_sha256': executable_hash,
        'build_provenance': 'passed',
        'fork_brand_resource_count': brand_count,
        'dll_architectures': dlls,
        'isolated_launcher': True,
        'excluded_files_absent': True,
    }
output = args.archive.with_name('verification.json')
output.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
