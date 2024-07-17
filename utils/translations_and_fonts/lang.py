import sys
import argparse
from pathlib import Path
from dataclasses import dataclass
import polib
import logging

logger = logging.getLogger('lang.py')
pofile_name_tmp = 'Prusa-Firmware-Buddy_{lang}.po'


def load_translation(path: Path):
    """Load .po file at given location."""
    pofile = polib.pofile(str(path.resolve()))
    return pofile


def load_translations(directory: Path):
    """
    Load all translations (.po files) under given directory.

    Expected file structure is as follows:
    <directory>/
        <langcode AB>/
            Prusa-Firmware-Buddy_<langcode AB>.po
        <langcode XY>/
            Prusa-Firmware-Buddy_<langcode XY>.po
        ...
    """
    translations = dict()
    for subdir in directory.iterdir():
        if not subdir.is_dir():
            continue
        if len(subdir.name) != 2:  # not a lang code, skipping
            logger.warning('unexpected subdirectory %s found; skipping',
                           subdir)
        langcode = subdir.name
        pofile_path = subdir / pofile_name_tmp.format(lang=langcode)
        if not pofile_path.exists():
            logger.warning('no %s found within %s; skipping', pofile_path.name,
                           subdir)
            continue
        try:
            pofile = load_translation(pofile_path)
        except OSError as error:
            logger.warning('failed to read %s: %s; skipping', pofile_path,
                           error)
            continue
        translations[langcode] = pofile
    return translations


def hash_djb2(s):
    hsh = 5381
    for x in s.encode('utf-8'):
        hsh = ((hsh << 5) + hsh) + x
    return hsh & 0xFFFFFFFF


@dataclass
class HashTable:
    string_rec: list
    buckets: list


@dataclass
class BucketItem:
    leading_bytes: bytes
    table_index: int


@dataclass
class HashTableEntry:
    table_index: int
    key: str
    value: str


class UnsolvableCollisionError(Exception):
    pass


def generate_hash_table(entries,
                        buckets_cnt,
                        leading_bytes_len=2,
                        hash_fn=hash_djb2) -> HashTable:
    """
    Generate hash table data for given entries.

    :raises: UnsolvableCollisionError if a hash_table with given parameters can't be composed.
    """

    def reduced_hash(data):
        return hash_fn(data) % buckets_cnt

    def get_leading_bytes(entry):
        return entry.key.encode('utf-8')[:leading_bytes_len]

    # stable sort the entries lexicographically
    entries = sorted(entries, key=lambda e: e.key)
    # stable sort the entries by their reduced hash value
    entries = sorted(entries, key=lambda e: reduced_hash(e.key))

    # generate string rec array
    string_rec = [
        BucketItem(leading_bytes=get_leading_bytes(entry),
                   table_index=entry.table_index) for entry in entries
    ]

    # generate hash table
    buckets = [(None, None)] * buckets_cnt
    idx = 0
    while idx < len(entries):
        current_hash = reduced_hash(entries[idx].key)
        # find entries belonging to the bucket
        bucket_end_idx = idx + 1
        while bucket_end_idx < len(entries) and current_hash == reduced_hash(
                entries[bucket_end_idx].key):
            bucket_end_idx += 1
        # check there are no unsolvable collisions
        distinct_leading_bytes = set(
            get_leading_bytes(entries[i]) for i in range(idx, bucket_end_idx))
        bucket_size = bucket_end_idx - idx
        if len(distinct_leading_bytes) != bucket_size:
            raise UnsolvableCollisionError()
        # add the bucket info
        buckets[current_hash] = (idx, bucket_end_idx)
        idx = bucket_end_idx

    return HashTable(buckets=buckets, string_rec=string_rec)


def format_c_array(items, indent=4):
    """Create a C array literal with given items."""
    lines = ['{']
    lines += [' ' * indent + str(item) + ',' for item in items]
    lines += ['};\n']
    return '\n'.join(lines)


def dump_hpp_array(path: Path, decl, items):
    """Write down a file with array definition."""
    with open(path, 'w') as f:
        f.write(decl + ' = ' + format_c_array(items, indent=4))


def dump_ipp_array(path: Path, items):
    """Write down a file with array definition (no decl, to be used inlined)"""
    with open(path, 'w') as f:
        f.write(format_c_array(items, indent=0))


def dump_buckets_count(path: Path, buckets_count):
    """Write down a file composed of constexpr size_t buckets_count = xxxx;"""
    with open(path, 'w') as f:
        f.write('constexpr size_t buckets_count = ' + str(buckets_count) +
                ';\n')


def dump_hash_table(langcode, entries, hash_table: HashTable,
                    output_dir: Path):
    """Generate all the required C++ files for given hash table."""
    # concatenate all the values and remember all the indicies
    utf8_raw = b''
    indicies = []
    for entry in entries:
        indicies.append(len(utf8_raw))
        utf8_raw += entry.value.encode('utf-8') + b'\x00'
    # dump the values itself
    decl = f'const uint8_t StringTable{langcode.upper()}::utf8Raw[]'
    dump_hpp_array(output_dir / f'utf8Raw.{langcode}.hpp', decl,
                   ['0x%02x' % (byte, ) for byte in utf8_raw])
    # dump indicies of the values
    decl = f'const uint32_t StringTable{langcode.upper()}::stringBegins[]'
    dump_hpp_array(output_dir / f'stringBegins.{langcode}.hpp', decl,
                   ['0x%x' % (index, ) for index in indicies])
    # dump hash table's buckets
    dump_ipp_array(output_dir / f'hash_table_buckets.ipp', [
        '{ 0x%xU, 0x%xU }' %
        (start if start is not None else 0xffff, end or 0xffff)
        for start, end in hash_table.buckets
    ])
    # dump string indicies
    dump_ipp_array(output_dir / f'hash_table_string_indices.ipp', [
        '{ 0x%xU, 0x%xU }' %
        (int.from_bytes(item.leading_bytes, 'little'), item.table_index)
        for item in hash_table.string_rec
    ])
    dump_buckets_count(output_dir / f'hash_table_buckets_count.ipp',
                       len(hash_table.buckets))


def cmd_generate_hash_tables(args):
    """Entrypoint of the generate-hash-tables subcommand."""
    translations = load_translations(args.input_dir)
    if not translations:
        logger.error('no translations found')
        return 1
    buckets_count = 128

    for langcode, translation in translations.items():
        # entries we wanna store in the hash table
        entries = [
            HashTableEntry(table_index=idx,
                           key=entry.msgid,
                           value=entry.msgstr)
            for idx, entry in enumerate(translation)
        ]

        # generate the hash table (loop until we find a number of buckets without unsolvable collisions)
        while True:
            try:
                hash_table = generate_hash_table(entries,
                                                 buckets_cnt=buckets_count)
                logger.info(f'hash table %s: using %d buckets', langcode,
                            buckets_count)
                break
            except UnsolvableCollisionError:
                buckets_count += 1
        else:
            logger.error(
                'failed to find number of buckets free of unsolvable collisions; exiting'
            )
            return 1

        # create all the required .hpp and .ipp files with data of the hashtable
        dump_hash_table(langcode, entries, hash_table, args.output_dir)


def cmd_generate_required_chars(args):
    """Entrypoint of the generate-required-chars subcommand."""
    translations = load_translations(args.input_dir)
    if not translations:
        logger.error('no translations found')
        return 1

    required_chars = set(
        ch for translation in translations.values() for entry in translation
        for ch in entry.msgstr
        if ((ord(ch) > 127 and ord(ch) <= 0x30A0) or ord(ch) > 0x30FF)
        and ord(ch) != 0x3001
        and ord(ch) != 0x3002)  # without katakana and japanese , and .

    # All fonts have to have '?'
    required_chars.add('?')

    # characters contained in language names; not contained in *.po files
    required_chars.add('Č')
    required_chars.add('š')
    required_chars.add('ñ')
    required_chars.add('ç')

    # Add all standard ASCII characters (32 - 127)
    # We need nearly all of them and it speeds up character draw process (no lower_bound to find the character index)
    for ch in range(0x20, 0x7F):  # 0x7F is DEL (not used)
        required_chars.add(chr(ch))

    required_chars = sorted(required_chars)
    open(args.output_dir / 'standard-chars.txt', 'w',
         encoding='utf-8').write(' '.join(required_chars))

    required_chars = set(required_chars)

    # characters contained in language name are not contained in *.po files:
    # "Japanese" == ニホンゴ <- these have to be added separately because language name is not translated
    required_chars.add('ニ')
    required_chars.add('ホ')
    required_chars.add('ン')
    required_chars.add('ゴ')

    # Add all characters of katakana (testing)
    for ch in range(0x30A1, 0x30FF + 1):  # Add katakana alphabet for japanese
        required_chars.add(chr(ch))
    required_chars.add('、')
    required_chars.add('。')

    required_chars = sorted(required_chars)
    open(args.output_dir / 'full-chars.txt', 'w',
         encoding='utf-8').write(' '.join(required_chars))
    # open(args.output_dir / 'required-chars.raw',
    #     'bw').write(''.join(required_chars).encode('utf-32-le'))

    # Reduced character set for font LARGE
    digits_chars = set([
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
        ".",
        "%",
        "?",
        " ",
        "-",
        ",",
    ])
    digits_chars = sorted(digits_chars)
    open(args.output_dir / 'digits-chars.txt', 'w',
         encoding='utf-8').write(' '.join(digits_chars))


def cmd_dump_pofiles(args):
    """Entrypoint of the dump-pofiles subcommand."""
    # load all the po files
    translations = load_translations(args.input_dir)
    if not translations:
        logger.warning('no translations found')
        return 1
    # get list of keys
    keys = list(entry.msgid for entry in list(translations.values())[0])
    # output the keys.txt file
    open(args.output_dir / 'keys.txt',
         'w').writelines(k.replace('\n', '\\n') + '\n' for k in keys)
    # output all the [lang].txt files
    for langcode, pofile in translations.items():
        lines = list()
        for key, entry in zip(keys, pofile):
            if key != entry.msgid:
                logger.warning(
                    'unexpected entry %s (%s expected); skipping %s',
                    entry.msgid, key, langcode)
                break
            lines.append(entry.msgstr.replace('\n', '\\n') + '\n')
        open(args.output_dir / f'{langcode}.txt', 'w',
             encoding='utf-8').writelines(lines)


def main():
    # prepare top level argument parser
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='count', default=0)
    subparsers = parser.add_subparsers(title='subcommands', dest='subcommand')
    subparsers.required = True

    # prepare dump-pofiles subparser
    dump_pofiles = subparsers.add_parser('dump-pofiles')
    dump_pofiles.add_argument('input_dir', metavar='input-dir', type=Path)
    dump_pofiles.add_argument('output_dir', metavar='output-dir', type=Path)
    dump_pofiles.set_defaults(func=cmd_dump_pofiles)

    # prepare generate-hash-tables subparser
    generate_hash_tables = subparsers.add_parser('generate-hash-tables')
    generate_hash_tables.add_argument('input_dir',
                                      metavar='input-dir',
                                      type=Path)
    generate_hash_tables.add_argument('output_dir',
                                      metavar='output-dir',
                                      type=Path)
    generate_hash_tables.set_defaults(func=cmd_generate_hash_tables)

    # generate required-chars
    generate_required_chars = subparsers.add_parser('generate-required-chars')
    generate_required_chars.add_argument('input_dir',
                                         metavar='input-dir',
                                         type=Path)
    generate_required_chars.add_argument('output_dir',
                                         metavar='output-dir',
                                         type=Path)
    generate_required_chars.set_defaults(func=cmd_generate_required_chars)

    # parse and run a subcommand
    args = parser.parse_args(sys.argv[1:])
    logging.basicConfig(format='%(levelname)-8s %(message)s',
                        level=logging.WARNING - args.verbose * 10)
    retval = args.func(args)
    sys.exit(retval if retval is not None else 0)


if __name__ == '__main__':
    main()
