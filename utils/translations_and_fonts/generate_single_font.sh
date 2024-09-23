#!/bin/bash

w=$1
h=$2
type=$3
charset_option=$4

png_dst=src/gui/res/fnt_png/
dst_name=font_${type}_${w}x${h}_${charset_option}

# Generate required characters for every font charset_option
python3 utils/translations_and_fonts/lang.py generate-required-chars src/lang/po .

# Calculate character count
full_character_count=$(cat "full-chars.txt" | tr -d ' \n' | wc -m)
standard_character_count=$(cat "standard-chars.txt" | tr -d ' \n' | wc -m)
digits_character_count=$(cat "digits-chars.txt" | tr -d ' \n' | wc -m)

# Caluclate rows of current font
if [[ "$charset_option" == "full" ]]; then
    char_rows=$(awk "BEGIN {print int(\"$full_character_count\" / 16 + 0.999999)}")
    required_chars=full-chars.txt
    ipp_path=src/guiapi/include/fnt-full-indices.ipp
elif [[ "$charset_option" == "standard" ]]; then
    char_rows=$(awk "BEGIN {print int(\"$standard_character_count\" / 16 + 0.999999)}")
    required_chars=standard-chars.txt
    ipp_path=src/guiapi/include/fnt-standard-indices.ipp
elif [[ "$charset_option" == "digits" ]]; then
    char_rows=$(awk "BEGIN {print int(\"$digits_character_count\" / 16 + 0.999999)}")
    required_chars=digits-chars.txt
    ipp_path=src/guiapi/include/fnt-digits-indices.ipp
else
    echo "Argument 'charset_option' ( digits / standard / full ) contains an unexpected value"
    rm -rf full-chars.txt standard-chards.txt digits-chars.txt
    exit 1
fi

# Generate font PNGs and index files (based on charset)
# Having all possible source (standard and katakana) for each font IS MANDATORY!
python3 utils/translations_and_fonts/font.py src/gui/res/fnt_src/*${type}_${w}x${h}.png src/gui/res/fnt_src/${w}x${h}px*_katakana.png ${charset_option} ${required_chars} ${w} ${h} ${png_dst}${dst_name}.png ${ipp_path}

# Check the return value of font.py
if [ $? -eq 0 ]; then
    echo "font.py script executed successfully"
else
    echo "font.py script failed"
    rm -rf full-chars.txt standard-chars.txt digits-chars.txt
    exit 1
fi

rm -rf full-chars.txt standard-chars.txt digits-chars.txt

# Build png2font binary
mkdir -p build_tests
cd build_tests
../.dependencies/cmake-3.30.3/bin/cmake -D CMAKE_EXPORT_COMPILE_COMMANDS:BOOL=YES -D CMAKE_C_FLAGS="-O0 -ggdb3" -D CMAKE_CXX_FLAGS="-O0 -ggdb3 -std=c++20" -D CMAKE_BUILD_TYPE=Debug .. -G Ninja
ninja utils/translations_and_fonts/png2font/png2font
cd ../

# Generate binary from the font
./build_tests/utils/translations_and_fonts/png2font/png2font -src=${png_dst}${dst_name}.png -dst=${png_dst}${dst_name}_preview.png -out=${dst_name}.bin -bpp=4 -w=${w} -h=${h} -c=16 -r=${char_rows}

# Generate C++ header from binary
python3 utils/translations_and_fonts/bin2cc.py ${dst_name}.bin src/gui/res/cc/${dst_name}.hpp ${type} ${w} ${h} FontCharacterSet::${charset_option}
rm -rf ${dst_name}.bin

echo "${dst_name}.hpp generation completed."
