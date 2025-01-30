#!/bin/bash

# Reset current Prusa-Firmware-Buddy.pot
rm -f src/lang/po/Prusa-Firmware-Buddy.pot
touch src/lang/po/Prusa-Firmware-Buddy.pot

# Generate template for all marked strings from the codebase (except error codes!)
find src -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$" \
 | parallel -j1 xgettext --keyword=_ --keyword=N_ --language=C --package-name=Prusa-Firmware-Buddy \
 --package-version=4.1 --msgid-bugs-address=info@prusa3d.com --add-comments -j --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot {}


# Generate template for all error code strings
mkdir -p tmp_error_headers

python3 lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_mini.hpp MINI 12 --list
python3 lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_mk4.hpp MK4 13 --list
python3 lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_mk35.hpp MK3.5 23 --list
python3 lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_ix.hpp iX 16 --list
python3 lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list_xl.hpp XL 17 --list # XL_DEV_KIT shares header with XL (see AddPrusaErrorCodes.cmake)
python3 lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/mmu-error-codes.yaml tmp_error_headers/error_list_mmu.hpp MMU 04 --mmu --list

# Generate template for strings from error headers generated separately for each printer
find tmp_error_headers -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$"  | parallel -j1 xgettext --keyword=_ --keyword=N_ --language=C --package-name=Prusa-Firmware-Buddy  --package-version=4.1 --msgid-bugs-address=info@prusa3d.com --add-comments -j --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot {}

# Remove tmp_error_headers
rm -rf tmp_error_headers

# Fill out metadata
curr_year=$(date +'%Y')
sed -i "s/SOME DESCRIPTIVE TITLE./Prusa-Firmware-Buddy/g" src/lang/po/Prusa-Firmware-Buddy.pot
sed -i "s/YEAR THE PACKAGE'S COPYRIGHT HOLDER/${curr_year} Prusa Research/g" src/lang/po/Prusa-Firmware-Buddy.pot
sed -i "s/FIRST AUTHOR <EMAIL@ADDRESS>, YEAR./Prusa Research <info@prusa3d.com>, ${curr_year}/g" src/lang/po/Prusa-Firmware-Buddy.pot

echo "New POT file was successfully generated at 'src/lang/po/Prusa-Firmware-Buddy.pot'"
