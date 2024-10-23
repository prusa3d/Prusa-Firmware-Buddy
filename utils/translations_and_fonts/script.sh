#!/bin/bash

w=$1
h=$2
type=$3

png_dst=src/gui/res/fnt_png/
dst_name=font_${type}_${w}x${h}

python3 utils/translations_and_fonts/lang.py generate-nonascii-chars src/lang/po .

python3 utils/translations_and_fonts/font.py non-ascii-chars.txt src/gui/res/fnt_src/*${type}_${w}x${h}.png ${w} ${h} ${png_dst}${dst_name}.png src/guiapi/include/fnt-indices.ipp
rm -rf non-ascii-chars.txt non-ascii-chars.raw

mkdir -p build_tests
cd build_tests
../.dependencies/cmake-3.30.3/bin/cmake -D CMAKE_EXPORT_COMPILE_COMMANDS:BOOL=YES -D CMAKE_C_FLAGS="-O0 -ggdb3" -D CMAKE_CXX_FLAGS="-O0 -ggdb3 -std=c++20" -D CMAKE_BUILD_TYPE=Debug .. -G Ninja -D BOARD=XBUDDY
ninja utils/translations_and_fonts/png2font/png2font
cd ../

./build_tests/utils/translations_and_fonts/png2font/png2font -src=${png_dst}${dst_name}.png -dst=${png_dst}${dst_name}_preview.png -out=${dst_name}.bin -bpp=4 -w=${w} -h=${h} -c=16 -r=11

python3 utils/translations_and_fonts/bin2cc.py ${dst_name}.bin src/gui/res/cc/${dst_name}.hpp ${type} ${w} ${h}
rm -rf ${dst_name}.bin

echo "${dst_name}.hpp generation completed."
