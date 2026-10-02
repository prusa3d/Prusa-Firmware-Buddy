#!/usr/bin/env python3
"""Signature Oak icon parity report.

Signature Oak is a limited-edition Core One variant with a brass UI theme.
Brass icons in src/gui/res/png_brass/ are OVERRIDES of standard icons whose
orange pixels are replaced with brass. Only standard icons that contain
orange pixels need a brass counterpart, and only those actually packed into
the Signature Oak firmware (it is always a COREONE build).

This script lists standard icons that have orange pixels but no brass
override. The output is informational; the script never fails the pipeline.
"""

from pathlib import Path

from PIL import Image

# Prusa orange: RGB(248, 101, 27)
ORANGE_RGB = (248, 101, 27)
COLOR_TOLERANCE = 30

PNG_DIR = Path('src/gui/res/png')
BRASS_DIR = Path('src/gui/res/png_brass')
USED_IMGS = Path('src/gui/res/COREONE_used_imgs.txt')


def is_orange(r, g, b):
    return (abs(r - ORANGE_RGB[0]) < COLOR_TOLERANCE
            and abs(g - ORANGE_RGB[1]) < COLOR_TOLERANCE
            and abs(b - ORANGE_RGB[2]) < COLOR_TOLERANCE)


def png_has_orange(path):
    img = Image.open(path).convert('RGBA')
    for r, g, b, a in img.getdata():
        if a < 10:
            continue
        if is_orange(r, g, b):
            return True
    return False


def main():
    used_icons = sorted(
        set(line.strip()
            for line in USED_IMGS.read_text().splitlines()) - {''})
    brass_icons = set(p.name for p in BRASS_DIR.glob('*.png'))

    missing = [
        icon for icon in used_icons
        if icon not in brass_icons and png_has_orange(PNG_DIR / icon)
    ]

    if missing:
        print(f'\ufeffSignature Oak icon parity: {len(missing)} icon(s) '
              f'with orange pixels but no brass override in png_brass/ 🙏')
        print('Please request a brass variant from the designer for each '
              'icon below:')
        for icon in missing:
            print(f'* `{icon}`')
    else:
        print('\ufeffSignature Oak icon parity: '
              'all orange icons have brass overrides 💪')


if __name__ == '__main__':
    main()
