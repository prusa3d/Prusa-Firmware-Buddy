#!/bin/bash

# PARAMETERS:
# character width
# character height
# font style (bold / regular) - this is selected base on font's source PNG in src/gui/res/fnt_src/ (Katakana part of the font is always regular for visibility purposes)
# font charset - this is converted into INT
#   Full (latin + katakana + cyrillic)       == 0
#   Latin (standard)                         == 1
#   Digits (digits + '.' + '%' + '?' + '-')  == 2
#   Latin and Katakana                       == 3
#   Latin and Cyrillic                       == 4

fail_check() {
    [ ! $? -eq 0 ] && { echo "FAILED: Unsuccessfull font generation"; exit 1; }
}

# Fonts for BIG layout displays
./utils/translations_and_fonts/generate_single_font.sh 11 19 bold full
fail_check
./utils/translations_and_fonts/generate_single_font.sh 13 22 bold full
fail_check
./utils/translations_and_fonts/generate_single_font.sh 9 16 regular full
fail_check
./utils/translations_and_fonts/generate_single_font.sh 30 53 bold digits
fail_check

# Fonts for SMALL layout displays (with japanese)
./utils/translations_and_fonts/generate_single_font.sh 7 13 regular latin_and_katakana
fail_check
./utils/translations_and_fonts/generate_single_font.sh 11 18 regular latin_and_katakana
fail_check
./utils/translations_and_fonts/generate_single_font.sh 9 16 regular latin_and_katakana
fail_check

# Fonts for SMALL layout displays (with cyrillic)
./utils/translations_and_fonts/generate_single_font.sh 7 13 regular latin_and_cyrillic
fail_check
./utils/translations_and_fonts/generate_single_font.sh 11 18 regular latin_and_cyrillic
fail_check
./utils/translations_and_fonts/generate_single_font.sh 9 16 regular latin_and_cyrillic
fail_check

# Fonts for SMALL layout displays (latin only)
./utils/translations_and_fonts/generate_single_font.sh 7 13 regular latin
fail_check
./utils/translations_and_fonts/generate_single_font.sh 11 18 regular latin
fail_check
./utils/translations_and_fonts/generate_single_font.sh 9 16 regular latin
fail_check

echo -e "\nSUCCEEDED: All fonts were generated. You can check them in \"src/gui/res/fnt_png\""
