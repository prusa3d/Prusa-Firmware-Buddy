#!/bin/bash

# PARAMETERS:
# character width
# character height
# font style (bold / regular) - this is selected base on font's source PNG in src/gui/res/fnt_src/ (Katakana part of the font is always regular for visibility purposes)
# font charset - this is converted into INT
#   Full (standard + katakana)                  == 0
#   Standard (standard)                         == 1
#   Digits (digits + '.' + '%' + '?' + '-')     == 2

./utils/translations_and_fonts/generate_single_font.sh 11 19 bold full
./utils/translations_and_fonts/generate_single_font.sh 13 22 bold full
./utils/translations_and_fonts/generate_single_font.sh 30 53 bold digits
./utils/translations_and_fonts/generate_single_font.sh 7 13 regular standard
./utils/translations_and_fonts/generate_single_font.sh 9 16 regular full
./utils/translations_and_fonts/generate_single_font.sh 11 18 regular standard
