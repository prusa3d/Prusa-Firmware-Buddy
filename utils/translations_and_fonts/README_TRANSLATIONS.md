# TRANSLATIONS AND LOCALIZATION

## NOTICE:
 * This tutorial is for linux systems. There is probably a very similair process on Windows (all dependencies should have Windows builds), but using commands straight from this tutorial on Windows is not guaranteed to work.

## Preparation
1. We are using gettext and parallel libraries for translations.
```bash
sudo apt-get install gettext
sudo apt-get install parallel
```
2. For font generation, we need libz and libpng-dev. Also `g++-10` was needed in my case.
```bash
sudo apt-get install libpng-dev
sudo apt-get install libz
```
3. Run commands in this tutorial from project's root directory.

## How to translate in the code
1. When you define the string, use `N_()` macro to mark texts for translations. It does not do anything else with the string (`const char *` is returned from it).
2. To translate, use `_()` macro. It receives `const char *` and it returns `string_view_utf8` which is used in every text-drawing function in GUI.

# Automation scripts
1. Generate POT file:
```bash
./utils/translations_and_fonts/generate_pot.sh
```
2. Export new translations, delete unused strings, replace unsupported characters, generate MO files and integrate PO & MO files into the codebase
```bash
path_to_PO - path to ZIP file OR directory containing translated texts in separate PO files

./utils/translations_and_fonts/new_translations.sh {path_to_PO}
```

# HOW TO UPDATE TRANSLATIONS STEP BY STEP (overview)
1. Generate POT template for strings from the codebase
2. Generate POT template for strings from Prusa-Error-Codes
3. Fill out POT header data
4. Translate texts: POT->PO
5. Delete unused strings
6. Replace unsupported characters
7. Regenerate MO files
8. Regenerate fonts with updated set of characters
9. Check if fonts contain previously unknown unsupported character
10. Run unit tests and make sure they all passed

# HOW TO UPDATE TRANSLATIONS STEP BY STEP
1. POT file contains all texts marked for translation. We use gettext to generate it. If you want to learn, how gettext works, [click here](https://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html).
This step will generate template for all marked strings from the codebase (except error codes!)
```bash
rm -f src/lang/po/Prusa-Firmware-Buddy.pot
touch src/lang/po/Prusa-Firmware-Buddy.pot

find src -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$" \
 | parallel -j1 xgettext --keyword=_ --keyword=N_ --language=C --package-name=Prusa-Firmware-Buddy \
 --package-version=4.1 --msgid-bugs-address=info@prusa3d.com --add-comments -j --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot {}
```

2. Now we need to add error strings, but each printer type has it's own error-codes.hpp generated during compiling. This series of commands will create error-code headers for all Buddy Firmware printers and generate the template into already existing POT file.

```bash
# Create temporary folder for error-code headers
mkdir -p tmp_error_headers

# Generate headers
python lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_mini.hpp MINI 12 --list
python lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_mk4.hpp MK4 13 --list
python lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_mk35.hpp MK3.5 23 --list
python lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_ix.hpp iX 16 --list
python lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_xl.hpp XL 17 --list
python lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/mmu-error-codes.yaml tmp_error_headers/error_list_mmu.hpp MMU 04 --mmu --list

# Generate template for strings from error headers generated separately for each printer
find tmp_error_headers -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$"  | parallel -j1 xgettext --keyword=_ --keyword=N_ --language=C --package-name=Prusa-Firmware-Buddy  --package-version=4.1 --msgid-bugs-address=info@prusa3d.com --add-comments -j --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot {}

# Remove tmp_error_headers
rm -rf tmp_error_headers
```

3. Insert header data
```bash
sed -i "s/SOME DESCRIPTIVE TITLE./Prusa-Firmware-Buddy/g" src/lang/po/Prusa-Firmware-Buddy.pot
sed -i "s/YEAR THE PACKAGE'S COPYRIGHT HOLDER/2024 Prusa Research/g" src/lang/po/Prusa-Firmware-Buddy.pot
sed -i "s/FIRST AUTHOR <EMAIL@ADDRESS>, YEAR./Prusa Research <info@prusa3d.com>, 2024/g" src/lang/po/Prusa-Firmware-Buddy.pot
```

4. Translate texts: POT -> PO. We leave this part of the pipeline to our Content team. To see how PO files are created, visit [gettext manual](https://www.gnu.org/software/gettext/manual/gettext.html).
PO files have to named to comply with the format "Prusa-Firmware-Buddy_XX.po", where XX is the location code e.g. `Prusa-Firmware-Buddy_en.po` or `Prusa-Firmware-Buddy_cs.po` and then place them in their designated directiories in `src/lang/po`.

5. Delete unused strings
Run this for every language code from within its folder (src/lang/po/XX)
```bash
msgattrib --set-obsolete --ignore-file=../Prusa-Firmware-Buddy.pot -o Prusa-Firmware-Buddy_XX.po Prusa-Firmware-Buddy_XX.po     # replace XX with language code
msgattrib --no-obsolete -o Prusa-Firmware-Buddy_XX.po Prusa-Firmware-Buddy_XX.po                                                # replace XX with language code
```

6. Replacing unsupported characters
Sometimes, translators are using unsupported characters in their translations (.po). We have to replace them with supported characters. The [script](replace_unsupported_chars.py) will go through every PO file and replace unsupported characters, that we know of.
```bash
python3 utils/translations_and_fonts/replace_unsupported_chars.py src/lang/po/
```

7. Regenerating MO files. Run this for each language code and overwrite the existing MO files.
```bash
msgfmt src/lang/po/XX/Prusa-Buddy-Firmware_XX.po -o src/lang/po/XX/Prusa-Buddy-Firmware_XX.mo   # replace XX with language code
```

8. Regenerating fonts
To safe space, our fonts have only set of characters, that are used in the translations. Fonts have to be regenerated because new translations could contain an unknown character.
[tutorial](README_FONTS.md)

9. Check if fonts contain previously unknown unsupported character. Go to `src/gui/res/fnt_png` and look for any "empty square" in the font - That is the symptom of unknown unsupported character.
Remember the position in the font (row & column starting from 0) and look at the `src/guiapi/include/fnt-XXXX-indices` (XXXX is "full"/"standard"/"digits" based on the examined font). There you can find the unsupported character by row and column.
Once you remove / replace those characters in PO files, you have to redo updating translations from step 5.

10. Building and running unit tests
```bash
mkdir -p build_tests
cd build_tests

# Configure the unit test project
../.dependencies/cmake-3.28.3/bin/cmake -D CMAKE_EXPORT_COMPILE_COMMANDS:BOOL=YES -D CMAKE_C_FLAGS="-O0 -ggdb3" -D CMAKE_CXX_FLAGS="-O0 -ggdb3 -std=c++20" -D CMAKE_BUILD_TYPE=Debug ..

# Build unit tests
make -j$(nproc) tests VERBOSE=1 # This will build unit tests

# Run unit tests
../.dependencies/cmake-3.28.3/bin/ctest --output-on-failure .
```
