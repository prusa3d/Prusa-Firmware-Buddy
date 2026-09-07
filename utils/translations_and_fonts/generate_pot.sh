#!/bin/bash

set -e

# Generate template for all error code strings
mkdir -p tmp_error_headers

python3 lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml tmp_error_headers/error_list.hpp XL 17 --list --ignore_printer_id
python3 lib/Prusa-Error-Codes/generate_buddy_headers.py lib/Prusa-Error-Codes/yaml/mmu-error-codes.yaml tmp_error_headers/error_list_mmu.hpp MMU 04 --mmu --list --ignore_printer_id

# Generate template for strings from firmware and error headers
find src include tmp_error_headers lib -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$" > tmp_filelist
xgettext --keyword=_ --keyword=N_ --language=C++ --from-code=UTF-8 --package-name=Prusa-Firmware-Buddy  --package-version=4.1 --msgid-bugs-address=info@prusa3d.com --add-comments --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot --files-from=tmp_filelist

# Remove temporary files
rm -rf tmp_error_headers tmp_filelist

# Fill out metadata
curr_year=$(date +'%Y')
sed -i "s/SOME DESCRIPTIVE TITLE./Prusa-Firmware-Buddy/g" src/lang/po/Prusa-Firmware-Buddy.pot
sed -i "s/YEAR THE PACKAGE'S COPYRIGHT HOLDER/${curr_year} Prusa Research/g" src/lang/po/Prusa-Firmware-Buddy.pot
sed -i "s/FIRST AUTHOR <EMAIL@ADDRESS>, YEAR./Prusa Research <info@prusa3d.com>, ${curr_year}/g" src/lang/po/Prusa-Firmware-Buddy.pot

echo "New POT file was successfully generated at 'src/lang/po/Prusa-Firmware-Buddy.pot'"
