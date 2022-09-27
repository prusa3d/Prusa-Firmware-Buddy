import pathlib
import sys

assert len(sys.argv) == 3, f'Usage: {sys.argv[0]} <filename> <filesize>'
path = pathlib.Path(sys.argv[1])
size = int(sys.argv[2])

if path.exists():
    sys.exit(0)

with open(path, 'wb') as f:
    f.seek(size - 1)
    f.write(b'\x00')
