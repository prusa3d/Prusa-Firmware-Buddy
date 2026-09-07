#!/usr/bin/env python3
"""Generate the font data out of the translations and the source pngs.

Every font there is is declared here, in FONTS. The result is a single C++ fragment
holding the character sets and the bitmaps, to be included inside namespace font_data.
"""

import argparse
import math
import sys
from dataclasses import dataclass
from pathlib import Path

import polib
from PIL import Image

#
# The characters the fonts have to contain, extracted out of the translations. See
# README.md for what the character sets are.
#

# File names may hold characters no translation uses, so every set spans all of latin
LATIN = range(0x20, 0x7F)

KATAKANA = range(0x30A1, 0x30FF + 1)

# The ukrainian alphabet. The source pngs hold the whole 0x0400 block, this is a choice
# of what to spend the flash on - ukrainian is the only cyrillic language there is.
CYRILLIC = ([0x0404, 0x0406, 0x0407] + list(range(0x0410, 0x042A)) + [0x042C] +
            list(range(0x042E, 0x044A)) + list(range(0x044C, 0x0450)) +
            [0x0454, 0x0456, 0x0457, 0x0490, 0x0491])

# Every font holds these, whatever the translations use: '?' is what a character the
# font does not hold falls back to, '°' spells out degrees celsius.
COMMON = '?°'

# Languages of the latin_and_accents set. Not en - english is in every .po file. Not ja
# and uk - they have a font of their own.
ACCENTED_LANGUAGES = ['cs', 'de', 'es', 'fr', 'it', 'pl']

# Every language the character sets are built out of
LANGUAGES = ACCENTED_LANGUAGES + ['ja', 'uk']


def po_path(po_dir: Path, langcode: str):
    """Location of the translation of a language."""
    return po_dir / langcode / f'Prusa-Firmware-Buddy_{langcode}.po'


def load_translation(po_dir: Path, langcode: str):
    """Load the .po file of a language."""
    return polib.pofile(str(po_path(po_dir, langcode).resolve()))


def translated_chars(translation):
    """Characters of both the original and the translated texts.

    Only the ones a font can draw - the texts hold newlines, and a translator can leave
    a non-breaking space or a soft hyphen behind, none of which has a glyph.
    """
    return set(ch for entry in translation for ch in entry.msgid + entry.msgstr
               if ch.isprintable())


def digits_chars():
    # Reduced character set for font LARGE
    return set("0123456789.%? -,")


def latin_and_accents_chars(po_dir: Path):
    chars = set()
    for langcode in ACCENTED_LANGUAGES:
        chars.update(translated_chars(load_translation(po_dir, langcode)))

    chars.update(COMMON)
    # The language names are not translated, so they are in no .po file
    chars.update('Čeština Español Français')

    chars.update(chr(ch) for ch in LATIN)
    return chars


def latin_and_katakana_chars(po_dir: Path):
    chars = translated_chars(load_translation(po_dir, 'ja'))

    chars.update(COMMON)
    # The language name is not translated, so it is in no .po file
    chars.update('ニホンゴ')
    # Japanese punctuation, sitting outside the katakana block
    chars.update('、。')

    chars.update(chr(ch) for ch in LATIN)
    chars.update(chr(ch) for ch in KATAKANA)
    return chars


def latin_and_cyrillic_chars(po_dir: Path):
    chars = translated_chars(load_translation(po_dir, 'uk'))

    chars.update(COMMON)
    # The language name is not translated, so it is in no .po file
    chars.update('Українська мова')

    chars.update(chr(ch) for ch in LATIN)
    chars.update(chr(ch) for ch in CYRILLIC)
    return chars


def character_sets(po_dir: Path):
    """The character sets, each sorted the way it is laid out in a font bitmap."""
    sets = {
        'digits': digits_chars(),
        'latin_and_accents': latin_and_accents_chars(po_dir),
        'latin_and_cyrillic': latin_and_cyrillic_chars(po_dir),
        'latin_and_katakana': latin_and_katakana_chars(po_dir),
    }
    sets['full'] = set().union(*sets.values())
    return {name: sorted(chars) for name, chars in sorted(sets.items())}


#
# The fonts themselves, built out of the source pngs
#

# The source pngs, and the bitmaps built out of them, are 16 characters wide
CHARS_PER_ROW = 16


@dataclass(frozen=True)
class Font:
    width: int  # character width [pixels]
    height: int  # character height [pixels]
    type: str  # regular, bold, ...
    charset: str  # one of the character sets above

    @property
    def name(self):
        return f'{self.type}_{self.width}x{self.height}_{self.charset}'


# Every font there is. Adding one here is all it takes to have it generated - the source
# pngs are looked up by the naming convention of source_pngs().
FONTS = [
    Font(7, 13, 'regular', 'latin_and_accents'),
    Font(7, 13, 'regular', 'latin_and_cyrillic'),
    Font(7, 13, 'regular', 'latin_and_katakana'),
    Font(9, 16, 'regular', 'full'),
    Font(9, 16, 'regular', 'latin_and_accents'),
    Font(9, 16, 'regular', 'latin_and_cyrillic'),
    Font(9, 16, 'regular', 'latin_and_katakana'),
    Font(11, 18, 'regular', 'latin_and_accents'),
    Font(11, 18, 'regular', 'latin_and_cyrillic'),
    Font(11, 18, 'regular', 'latin_and_katakana'),
    Font(11, 19, 'bold', 'full'),
    Font(13, 22, 'bold', 'full'),
    Font(30, 53, 'bold', 'digits'),
]


def find_png(png_dir: Path, pattern: str):
    """The single source png matching the pattern."""
    matches = sorted(png_dir.glob(pattern))
    if len(matches) != 1:
        raise RuntimeError(
            f'expected a single png matching {pattern}, found: {matches}')
    return matches[0]


def source_pngs(font: Font, png_dir: Path):
    """The latin, katakana and cyrillic source pngs of a font."""
    size = f'{font.width}x{font.height}'
    return (find_png(png_dir, f'*{font.type}_{size}.png'),
            find_png(png_dir, f'{size}px*_katakana.png'),
            find_png(png_dir, f'{size}px*_cyrillic.png'))


def source_position(char: str):
    """Where a character sits in its source png, as (png index, column, row)."""
    code = ord(char)

    if 0x0400 <= code <= 0x04FF:
        row, column = divmod(code - 0x0400, CHARS_PER_ROW)
        return 2, column, row

    # Comma and full stop sit at hardcoded coordinates of the katakana source png,
    # they are not part of the katakana block
    if char == '、':
        return 1, 0, 6
    if char == '。':
        return 1, 1, 6

    if 0x30A0 <= code <= 0x30FF:
        row, column = divmod(code - 0x30A0, CHARS_PER_ROW)
        return 1, column, row

    # The latin source png starts at the first printable character
    row, column = divmod(code - 0x20, CHARS_PER_ROW)
    return 0, column, row


def compose(font: Font, chars, pngs):
    """Lay the characters of a font out into a single image, 16 characters per row."""
    rows = math.ceil(len(chars) / CHARS_PER_ROW)
    image = Image.new('RGB', (CHARS_PER_ROW * font.width, rows * font.height),
                      color='white')

    unsupported = []
    for index, char in enumerate(chars):
        png_index, src_x, src_y = source_position(char)
        src = pngs[png_index]
        src_w, src_h = src.size

        # A character below the first one of its source png floors to a negative row
        if src_y < 0 or (src_y + 1) * font.height > src_h or (
                src_x + 1) * font.width > src_w:
            unsupported.append(char)
            continue

        dst_y, dst_x = divmod(index, CHARS_PER_ROW)
        image.paste(
            src.crop((src_x * font.width, src_y * font.height,
                      (src_x + 1) * font.width, (src_y + 1) * font.height)),
            (dst_x * font.width, dst_y * font.height, (dst_x + 1) * font.width,
             (dst_y + 1) * font.height))

    if unsupported:
        raise RuntimeError(
            f'{font.name}: no glyph for {"".join(unsupported)!r} in the source pngs'
        )

    return image


def remove_red_dots(image: Image.Image):
    """Drop the guide dots the source pngs are marked up with."""
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            if pixels[x, y] == (255, 0, 0):
                pixels[x, y] = (255, 255, 255)


def to_bitmap(image: Image.Image, font: Font, char_count: int):
    """Pack the composed image into the font bitmap: the characters in the order they
    were laid out, each of them row by row, 4 bits of opacity per pixel."""
    pixels = image.load()
    bytes_per_character = (font.width * font.height + 1) // 2
    bitmap = bytearray(bytes_per_character * char_count)

    for index in range(char_count):
        row, column = divmod(index, CHARS_PER_ROW)
        for y in range(font.height):
            for x in range(font.width):
                r, g, b = pixels[column * font.width + x,
                                 row * font.height + y]
                # 0 is fully transparent, 15 fully opaque
                opacity = (255 - (r + g + b) // 3) >> 4

                pixel = y * font.width + x
                offset = index * bytes_per_character + pixel // 2
                # Two pixels per byte, the first one in the high nibble. A character
                # with an odd number of pixels leaves the last nibble unused.
                bitmap[offset] |= opacity << (0 if pixel % 2 else 4)

    return bytes(bitmap)


def emit_array(out, declaration: str, values):
    out.write(f'{declaration} = {{\n')
    for value in values:
        out.write(f'    {value},\n')
    out.write('};\n')


def emit_character_set(out, name: str, chars):
    if ord(chars[-1]) > 0xFFFF:
        raise RuntimeError(f'{name}: {chars[-1]!r} does not fit a uint16_t')

    emit_array(out, f'constexpr uint16_t {name}_set[]',
               (f'0x{ord(ch):04x}' for ch in chars))
    out.write(f'static_assert(std::ranges::is_sorted({name}_set));\n\n')


def emit_font(out, font: Font, bitmap: bytes):
    # The bitmap is private to the translation unit, the font data is what the header
    # declares
    out.write('namespace {\n')
    emit_array(out, f'constexpr uint8_t {font.name}_bitmap[]',
               (f'0x{byte:02x}' for byte in bitmap))
    out.write('} // namespace\n\n')

    out.write(f'static_assert({font.name}.w == {font.width}'
              f' && {font.name}.h == {font.height});\n')
    out.write(f'constinit const FontData {font.name}_data'
              f' {{ {font.charset}_set, {font.name}_bitmap }};\n\n')


def generate(out, po_dir: Path, png_dir: Path, sources: list):
    """Write the font data, collecting what it was built out of into sources."""
    sets = character_sets(po_dir)
    sources.extend(po_path(po_dir, lang) for lang in LANGUAGES)

    out.write('namespace {\n\n')
    for name in sorted({font.charset for font in FONTS}):
        emit_character_set(out, name, sets[name])
    out.write('} // namespace\n\n')

    for font in FONTS:
        paths = source_pngs(font, png_dir)
        sources.extend(paths)

        with Image.open(paths[0]) as latin, Image.open(
                paths[1]) as katakana, Image.open(paths[2]) as cyrillic:
            pngs = (latin, katakana, cyrillic)
            for path, png in zip(paths, pngs):
                if png.mode != 'RGB':
                    raise RuntimeError(
                        f'{path} is {png.mode} instead of the required RGB')

            chars = sets[font.charset]
            image = compose(font, chars, pngs)
            remove_red_dots(image)

        emit_font(out, font, to_bitmap(image, font, len(chars)))


def write_depfile(path: Path, output: Path, sources):
    """Tell the build system what the font data was built out of."""
    deps = ' '.join(str(source).replace(' ', '\\ ') for source in sources)
    path.write_text(f'{output}: {deps}\n', encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--po-dir',
                        type=Path,
                        required=True,
                        help='directory holding the translations')
    parser.add_argument('--png-dir',
                        type=Path,
                        required=True,
                        help='directory holding the source pngs')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--depfile',
                        type=Path,
                        help='where to list what the fonts were built out of')
    args = parser.parse_args()

    sources = []
    try:
        with open(args.output, 'w', encoding='utf-8') as out:
            generate(out, args.po_dir, args.png_dir, sources)
    except RuntimeError as error:
        args.output.unlink(missing_ok=True)
        sys.exit(str(error))

    if args.depfile:
        write_depfile(args.depfile, args.output, sources)


if __name__ == '__main__':
    main()
