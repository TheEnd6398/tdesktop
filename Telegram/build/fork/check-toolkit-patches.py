import hashlib
from pathlib import Path
import subprocess
import tempfile


root = Path(__file__).resolve().parents[3]
toolkit = root / 'Telegram/lib_ui'
patches = sorted((root / 'fork-patches').glob('*.patch'))
stats = subprocess.check_output(['git', 'apply', '--numstat', *map(str, patches)], cwd=toolkit, text=True)
paths = sorted({line.split('\t', 2)[2] for line in stats.splitlines()})
index = Path(subprocess.check_output(['git', 'rev-parse', '--git-path', 'index'], cwd=toolkit, text=True).strip())
if not index.is_absolute():
    index = toolkit / index


def snapshot():
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None
            for p in [index, *(toolkit / name for name in paths)]}


before = snapshot()
with tempfile.TemporaryDirectory(prefix='aurora-patch-check-') as directory:
    probe = Path(directory)
    source = (root / 'Telegram').as_posix()
    script = (root / 'Telegram/cmake/fork_patches.cmake').as_posix()
    (probe / 'CMakeLists.txt').write_text(
        'cmake_minimum_required(VERSION 3.25)\n'
        'project(AuroraPatchCheck NONE)\n'
        f'set(CMAKE_CURRENT_SOURCE_DIR "{source}")\n'
        f'include("{script}")\n', encoding='utf-8')
    for attempt in range(2):
        subprocess.run(['cmake', '-S', str(probe), '-B', str(probe / 'build'), '-G', 'Ninja'], check=True)
        assert snapshot() == before, 'Patch-stack reconfiguration changed source files or the real Git index.'
print('Applied toolkit stack reconfigures without changing source files or the real Git index.')
