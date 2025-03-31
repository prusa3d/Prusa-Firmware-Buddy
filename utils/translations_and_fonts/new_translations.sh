#!/bin/bash

export_path=$1
lang_source_folder="src/lang/po"
lang_codes=("cs" "de" "es" "fr" "it" "ja" "pl")
work_dir="./tmp_translations"
remove_tmp_dir=0
fail=0

export_po_files() {
    if [ -d "$export_path" ]; then
        work_dir="$export_path"
    elif [ -f "$export_path" ] && [ ${export_path: -4} == ".zip" ]; then
        echo "Exporting PO files to the temporary location..."
        unzip -q "$export_path" -d "$work_dir"
        remove_tmp_dir=1
    else
        echo -e "\nERROR: \"$export_path\" is not a directory or ZIP file"
        exit 1
    fi

    echo "Checking and renaming exported files..."
    for code in "${lang_codes[@]}"
    do
        search_pattern="*${code}.po"
        po_filepath="${work_dir}/Prusa-Firmware-Buddy_${code}.po"
        if [ $(find "$work_dir" -type f -name "$search_pattern" | wc -l) -eq 1 ]; then
            exported_name="$(find "$work_dir" -type f -name "$search_pattern")"
            mv -n "$exported_name" "$po_filepath"
        else
            fail=1
            echo "ERROR: \"$search_pattern\" not found or found more than once"
        fi
    done

    [ $fail -eq 1 ] && { echo -e "\nFAILED: Some PO files in \"${work_dir}\" are missing or do not end with \"{LANG_CODE}.po\"\nAfter you fix it, rerun the script with \"${work_dir}\" in the argument"; exit 1; }
}

remove_unused_strings() {
    echo "Removing unused strings in PO files..."
    for code in "${lang_codes[@]}"
    do
        po_filepath="${work_dir}/Prusa-Firmware-Buddy_${code}.po"
        msgattrib --set-obsolete --ignore-file=${lang_source_folder}/Prusa-Firmware-Buddy.pot -o ${po_filepath} ${po_filepath}
        msgattrib --no-obsolete -o ${po_filepath} ${po_filepath}
        msgmerge --sort-output -o ${po_filepath} ${po_filepath} ${lang_source_folder}/Prusa-Firmware-Buddy.pot
    done
}

generate_mo_files() {
    echo "Generating MO files..."
    for code in "${lang_codes[@]}"
    do
        [ ! $(msgfmt "${work_dir}/Prusa-Firmware-Buddy_${code}.po" -o "${work_dir}/Prusa-Firmware-Buddy_${code}.mo" 2>&1 | tee /dev/tty | wc -l) -eq 0 ] && { fail=1; }
    done

    [ $fail -eq 1 ] && { echo -e "\nFAILED: Fix the errors reported while generating MO files and rerun the script with \"${work_dir}\" in the argument"; exit 1; }
}

move_translations() {
    echo "Moving new translations into source location..."
    for code in "${lang_codes[@]}"
    do
        po_filename="Prusa-Firmware-Buddy_${code}.po"
        mo_filename="Prusa-Firmware-Buddy_${code}.mo"

        rm ${lang_source_folder}/${code}/*

        mv ${work_dir}/${po_filename} ${lang_source_folder}/${code}/${po_filename}
        mv ${work_dir}/${mo_filename} ${lang_source_folder}/${code}/${mo_filename}
    done
}

clean_tmp_work_dir() {
    if [ $remove_tmp_dir -eq 1 ]; then
        echo "Removing temporary work directory..."
        rm -rf "$work_dir"
    else
        echo "Removing export directory SKIPPED..."
    fi
}

# main
export_po_files
remove_unused_strings
python3 utils/translations_and_fonts/replace_unsupported_chars.py ${work_dir}
generate_mo_files
move_translations
clean_tmp_work_dir
echo -e "\nSUCCEED: PO files are succcessfully integrated into the codebase"
exit 0
