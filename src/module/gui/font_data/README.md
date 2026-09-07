# `font_data` module

Fonts are stored as source PNGs inside the repository. This module is
responsible for transforming font data to a form usable by the renderer.

This module provides all the font data. If you don't need some of them,
just don't reference them and linker will happily drop the unused data.

## Character sets
To save FLASH, module provides multiple copies of the same font.
The copies differ in characters included:
 * `digits` - digits + punctuation
 * `full` - printable ASCII + accents + katakana + cyrillic + punctuation
 * `latin_and_accents` - printable ASCII + accents + punctuation
 * `latin_and_cyrillic` - printable ASCII + cyrillic + punctuation
 * `latin_and_katakana` - printable ASCII + katakana + punctuation
