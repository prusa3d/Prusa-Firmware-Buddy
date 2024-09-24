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

# HOW TO UPDATE TRANSLATIONS (overview)
1. Checkout the branch upon which you want to generate the translations
2. Generate POT file and give it to Content (Jakub Dolezal)
3. Receive PO files with completed translations from Content
4. Delete unused strings
5. Regenerate fonts with updated set of non-ascii characters
6. Run unit tests until they all passed

# Step-by-step
Here are some details for individual steps.

## Generating POT file
POT file contains all texts marked for translation. We use gettext to generate it. If you want to learn how gettext works [click here](https://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html).
1. Generate it through these steps:
```bash
rm -f src/lang/po/Prusa-Firmware-Buddy.pot
touch src/lang/po/Prusa-Firmware-Buddy.pot

find src -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$" \
 | parallel -j1 xgettext --keyword=_ --keyword=N_ --language=C --package-name=Prusa-Firmware-Buddy \
 --package-version=4.1 --msgid-bugs-address=info@prusa3d.com --add-comments -j --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot {}
# This step will generate template for all marked strings from the codebase (except error codes!)
```
2. Now the tricky part. We need to add error strings, but each printer type has it's own error-codes.hpp generated during building. This means, that we need to clean build each configuration marked to be released with updated translations and add error texts from build directory to POT file. For that run this command for every build configuration, that requires updated translations.

```bash
# DO THIS AFTER BUILDING THE CORRECT PRINTER TYPE, THAT REQUIRE UPDATED TRANSLATIONS (for each of the printer types selected to be released with updated translations)

# I am using vscode to build - if you are not using vscode, update path in the first argument
find build-vscode-buddy/lib/error_codes -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$"  | parallel -j1 xgettext --keyword=_ --keyword=N_ --language=C --package-name=Prusa-Firmware-Buddy  --package-version=4.1 --msgid-bugs-address=info@prusa3d.com --add-comments -j --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot {}
```

3. Insert header data
```bash
sed -i "s/SOME DESCRIPTIVE TITLE./Prusa-Firmware-Buddy/g" src/lang/po/Prusa-Firmware-Buddy.pot
sed -i "s/YEAR THE PACKAGE'S COPYRIGHT HOLDER/2024 Prusa Research/g" src/lang/po/Prusa-Firmware-Buddy.pot # update year
sed -i "s/FIRST AUTHOR <EMAIL@ADDRESS>, YEAR./Prusa Research <info@prusa3d.com>, 2024/g" src/lang/po/Prusa-Firmware-Buddy.pot # update year
```

## Receiving PO files from Content
PO files are usually received from content with bad naming. Rename them to comply with format "Prusa-Firmware-Buddy_XX.po", where XX is the location code e.g. `Prusa-Firmware-Buddy_en.po` or `Prusa-Firmware-Buddy_cs.po` and then place them in their designated directiories in `src/lang/po`.

## Delete unused strings
Run this for every language code from within its folder (src/lang/po/XX)
```bash
msgattrib --set-obsolete --ignore-file=../Prusa-Firmware-Buddy.pot -o Prusa-Firmware-Buddy_XX.po Prusa-Firmware-Buddy_XX.po # replace XX with language code
msgattrib --no-obsolete -o Prusa-Firmware-Buddy_XX.po Prusa-Firmware-Buddy_XX.po # replace XX with language code
```

## Replacing unsupported characters
Sometimes, translators are using unsupported characters in their translations (.po). We have to replace them with supported characters. The script will go through every .po file and replace them unsupported characters that we know of.
```bash
python3 utils/translations_and_fonts/replace_unsupported_chars.py src/lang/po/
```
[script](replace_unsupported_chars.py)

## Regenerating fonts
To safe space, our fonts have only set of characters, that are used in the translations. Fonts have to be regenerated because new translations could contain an unknown character.
[tutorial](README_FONTS.md)

## Building and running unit tests
```bash
mkdir -p build_tests
cd build_tests

# If it says, that -std=c++20 does not exist, your compilator is too old. Install g++-10 and run this command before cmake:
# export CXX=/bin/g++-10

../.dependencies/cmake-3.30.3/bin/cmake -D CMAKE_EXPORT_COMPILE_COMMANDS:BOOL=YES -D CMAKE_C_FLAGS="-O0 -ggdb3" -D CMAKE_CXX_FLAGS="-O0 -ggdb3 -std=c++20" -D CMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) tests VERBOSE=1 # This will build unit tests
../.dependencies/cmake-3.30.3/bin/ctest --output-on-failure . # This will run unit tests
```
