import sys
import argparse
from pathlib import Path
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
            if entry.msgstr == '':
                logger.warning('empty translation for %s', key)

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

    # parse and run a subcommand
    args = parser.parse_args(sys.argv[1:])
    logging.basicConfig(format='%(levelname)-8s %(message)s',
                        level=logging.WARNING - args.verbose * 10)
    retval = args.func(args)
    sys.exit(retval if retval is not None else 0)


if __name__ == '__main__':
    main()
