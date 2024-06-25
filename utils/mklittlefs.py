import argparse
from hashlib import sha256
import os
from pathlib import Path
import logging
from contextlib import contextmanager

from littlefs import LittleFS
from littlefs.context import UserContext
from littlefs.lfs import LFSConfig


class FileContext(UserContext):

    def __init__(self, path: Path):
        self.path = path
        self.logger = logging.getLogger('file-context')
        self.logger.setLevel(logging.WARNING)

    @contextmanager
    def opened_file(self, cfg):
        if not self.path.exists():
            with open(self.path, 'wb') as f:
                f.write(b'\xFF' * (cfg.block_size * cfg.block_count))
            self.logger.info('image file at %s created', self.path)
        with open(self.path, 'r+b') as f:
            yield f

    def read(self, cfg: LFSConfig, block: int, off: int,
             size: int) -> bytearray:
        start = block * cfg.block_size + off
        with self.opened_file(cfg) as f:
            f.seek(start)
            data = bytearray(f.read(size))
            self.logger.info('read of file %s at %d', self.path.name, start)
            return data

    def prog(self, cfg: LFSConfig, block: int, off: int, data: bytes) -> int:
        start = block * cfg.block_size + off
        with self.opened_file(cfg) as f:
            f.seek(start)
            f.write(data)
            self.logger.info('write to file %s at %d finished', self.path,
                             start)
        return 0

    def erase(self, cfg: LFSConfig, block: int) -> int:
        start = block * cfg.block_size
        with self.opened_file(cfg) as f:
            f.seek(start)
            f.write(b'\xFF' * cfg.block_size)
        self.logger.info('erase of file %s at %d finished', self.path.name,
                         start)
        return 0

    def sync(self, cfg: LFSConfig) -> int:
        return 0


def make_lfs(args):
    context = FileContext(getattr(args, 'image-file'))
    lfs = LittleFS(context=context,
                   block_size=args.block_size,
                   block_count=args.block_count)
    return lfs


def create_image_cmd(args) -> int:
    image_path = getattr(args, 'image-file')
    if image_path.exists():
        image_path.unlink()
    lfs = make_lfs(args)
    lfs.format()
    return 0


def add_file_cmd(args) -> int:
    lfs = make_lfs(args)
    lfs.makedirs(args.target.parent.as_posix(), exist_ok=True)
    with lfs.open(args.target.as_posix(),
                  'wb') as target, open(str(args.source), 'rb') as source:
        target.write(source.read())
    return 0


class HashContext:

    def __init__(self):
        self.sha = sha256()
        self.counter = 0
        self.logger = logging.getLogger('content-hash')

    def append_mark(self):
        self.sha.update(self.counter.to_bytes(4, 'little'))
        self.logger.info('mark(%i)', self.counter)
        self.counter += 1

    def append_data(self, data):
        self.sha.update(data)
        self.logger.info('data(%i)', len(data))


def update_hash_with_file(lfs: LittleFS, path: Path, hash_ctx: HashContext):
    # mark
    hash_ctx.append_mark()

    # filepath
    logging.getLogger('content-hash').info('path(%s)', str(path))
    hash_ctx.append_data(path.as_posix().encode('ascii'))

    # mark
    hash_ctx.append_mark()

    # filedata
    with lfs.open(path.as_posix(), 'rb') as f:
        hash_ctx.append_data(f.read())

    # mark
    hash_ctx.append_mark()


def update_hash_with_directory(lfs: LittleFS, path: Path,
                               hash_ctx: HashContext):
    # mark
    hash_ctx.append_mark()

    # filepath
    logging.getLogger('content-hash').info('path(%s)', str(path))
    hash_ctx.append_data(path.as_posix().encode('ascii'))

    # mark
    hash_ctx.append_mark()

    # dir content
    for fileinfo in lfs.scandir(path.as_posix()):
        entry_path = path / fileinfo.name
        if fileinfo.type == 2:  # directory
            update_hash_with_directory(lfs, entry_path, hash_ctx)
        elif fileinfo.type == 1:  # file
            update_hash_with_file(lfs, entry_path, hash_ctx)

    # mark
    hash_ctx.append_mark()


def calculate_image_hash(args) -> int:
    lfs = make_lfs(args)
    hash_ctx = HashContext()
    update_hash_with_directory(lfs, Path('/'), hash_ctx)

    output_path: Path = getattr(args, 'output-file')
    output_path.write_bytes(hash_ctx.sha.digest())

    return 0


def main(*args) -> int:
    parser = argparse.ArgumentParser(
        description='Manipulate littlefs stored in a file')
    parser.add_argument('--verbose', '-v', action='count', default=0)
    parser.add_argument('--block-size', type=int, default=4096)
    parser.add_argument('--block-count', type=int, default=2000)
    subparsers = parser.add_subparsers(title='subcommands', dest='subcommand')
    subparsers.required = True

    create_image_parser = subparsers.add_parser(
        'create-image', help='Create a new littlefs image file')
    create_image_parser.add_argument('image-file', type=Path)
    create_image_parser.set_defaults(func=create_image_cmd)

    add_file_parser = subparsers.add_parser('add-file',
                                            help='Add file to existing image')
    add_file_parser.add_argument('image-file', type=Path)
    add_file_parser.add_argument('source', type=Path)
    add_file_parser.add_argument('target', type=Path)
    add_file_parser.set_defaults(func=add_file_cmd)

    get_content_hash = subparsers.add_parser('get-content-hash')
    get_content_hash.add_argument('image-file', type=Path)
    get_content_hash.add_argument('output-file', type=Path)
    get_content_hash.set_defaults(func=calculate_image_hash)

    args = parser.parse_args(sys.argv[1:])
    logging.basicConfig(format='%(levelname)-8s %(message)s',
                        level=logging.WARNING - args.verbose * 10)
    return args.func(args)


if __name__ == '__main__':
    import sys
    exitcode = main(sys.argv)
    sys.exit(exitcode)
