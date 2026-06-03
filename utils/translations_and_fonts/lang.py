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
    # Sort for reproducible builds: iterdir() yields filesystem-dependent order,
    # and translations are later emitted in this dict's insertion order, so an
    # unsorted walk makes the generated translation/font data host-dependent.
    for subdir in sorted(directory.iterdir(), key=lambda p: p.name):
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


# Usage in a condition
def is_cyrillic_char(char):
    return char >= 0x0400 and char <= 0x04FF


def generate_cyrillic_standard_chars():
    yield 0x0404
    yield 0x0406
    yield 0x0407

    for code in range(0x0410, 0x042A):  # 0x0429 inclusive
        yield code

    yield 0x042C

    for code in range(0x042E, 0x044A):  # 0x0449 inclusive
        yield code

    for code in range(0x044C, 0x0450):  # 0x044F inclusive
        yield code

    yield 0x0456
    yield 0x0457
    yield 0x0490
    yield 0x0491


def is_japanese_char(char):
    return (char <= 0x30FF
            and char >= 0x30A0) or char == 0x3001 or char == 0x3002


def generate_required_digit_chars():
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
    return digits_chars


def generate_required_latin_chars(translations):
    # standard latin is range(0x20, 0x7F)
    translation_list = list()
    translation_list.append(translations["cs"])
    translation_list.append(translations["it"])
    translation_list.append(translations["fr"])
    translation_list.append(translations["de"])
    translation_list.append(translations["es"])
    translation_list.append(translations["pl"])
    # not en (english is contained within each PO file)
    # not ja (separate font)
    # not uk (separate font)

    required_chars_latin = set(ch for translation in translation_list
                               for entry in translation for ch in entry.msgstr)

    # add english characters
    required_chars_latin.update(
        {ch
         for entry in translations["cs"]
         for ch in entry.msgid})

    # All fonts have to have '?'
    # characters contained in language names; not contained in *.po files
    required_chars_latin.update({'?', 'Č', 'š', 'ñ', 'ç', '°'})

    # Include all standard latin characters (file names may include characters that are not in translations)
    for ch in range(0x20, 0x7F):
        required_chars_latin.add(chr(ch))

    return required_chars_latin


def generate_required_latin_and_katakana_chars(translations):
    # katakana is range(0x30A1, 0x30FF + 1)
    required_chars_latin_and_katakana = set(ch for entry in translations["ja"]
                                            for ch in entry.msgstr)

    # add english characters
    required_chars_latin_and_katakana.update(
        {ch
         for entry in translations["ja"]
         for ch in entry.msgid})

    # All fonts have to have '?'
    # characters contained in language name are not contained in *.po files:
    # "Japanese" == ニホンゴ <- these have to be added separately because language name is not translated
    required_chars_latin_and_katakana.update(
        {'?', 'ニ', 'ホ', 'ン', 'ゴ', '、', '。', '°'})

    # Add standart latin characters
    for ch in range(0x20, 0x7F):
        required_chars_latin_and_katakana.add(chr(ch))

    # Include all katakana characters (file names may include characters that are not in translations)
    for ch in range(0x30A1, 0x30FF + 1):
        required_chars_latin_and_katakana.add(chr(ch))

    return required_chars_latin_and_katakana


def generate_required_latin_and_cyrillic_chars(translations):
    # cyrillic is range(0x0400, 0x04FF + 1)
    required_chars_latin_and_cyrillic = set(ch for entry in translations["uk"]
                                            for ch in entry.msgstr)

    # add english characters
    required_chars_latin_and_cyrillic.update(
        {ch
         for entry in translations["uk"]
         for ch in entry.msgid})

    # All fonts have to have '?'
    # Add "Ukrainian" because it's not in .po files
    required_chars_latin_and_cyrillic.update({
        '?', 'У', 'к', 'р', 'а', 'ї', 'н', 'с', 'ь', 'м', 'о', 'в', 'ґ', 'Ґ',
        '°'
    })

    # Add standart latin characters
    for ch in range(0x20, 0x7F):
        required_chars_latin_and_cyrillic.add(chr(ch))

    # Include ukrainian standard characters (file names may include characters that are not in translations)
    for ch in generate_cyrillic_standard_chars():
        required_chars_latin_and_cyrillic.add(chr(ch))

    return required_chars_latin_and_cyrillic


def generate_character_set_file(charset, filepath: Path):
    charset_list = sorted(charset)
    open(filepath, 'w', encoding='utf-8').write(' '.join(charset_list))


def cmd_generate_required_chars(args):
    """Entrypoint of the generate-required-chars subcommand."""
    translations = load_translations(args.input_dir)
    if not translations:
        logger.error('no translations found')
        return 1

    # Generate DIGITS
    digits = generate_required_digit_chars()
    generate_character_set_file(digits, args.output_dir / 'digits-chars.txt')

    # Generate LATIN
    latin = generate_required_latin_chars(translations)
    generate_character_set_file(latin, args.output_dir / 'latin-chars.txt')

    # Generate LATIN+KATAKANA
    latin_and_katakana = generate_required_latin_and_katakana_chars(
        translations)
    generate_character_set_file(
        latin_and_katakana, args.output_dir / 'latin-and-katakana-chars.txt')

    # Generate LATIN+CYRILLIC
    latin_and_cyrillic = generate_required_latin_and_cyrillic_chars(
        translations)
    generate_character_set_file(
        latin_and_cyrillic, args.output_dir / 'latin-and-cyrillic-chars.txt')

    # Generate FULL FONT (all combined)
    full_font = set()
    full_font.update(digits)
    full_font.update(latin)
    full_font.update(latin_and_katakana)
    full_font.update(latin_and_cyrillic)

    generate_character_set_file(full_font, args.output_dir / 'full-chars.txt')


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
