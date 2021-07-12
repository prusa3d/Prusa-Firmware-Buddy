#!/bin/sh
#

# determine workspace path from $0
WRK_DIR=$(echo "$0" | sed 's/\\/\//g')  # replace '\' with '/' (windows)
WRK_DIR=${WRK_DIR%/*}                   # Res folder
WRK_DIR=${WRK_DIR%/*}                   # Gui folder
WRK_DIR=${WRK_DIR%/*}                   # parent folder == workspace folder
cd $WRK_DIR

PNG_DIR=$WRK_DIR/palette
PAL_DIR=./palette
CC_DIR=$WRK_DIR/cc

#uses image magick tool to convert png from normal to indexed 
#insert your own path to magick
# MAGICK_PATH=

png2cc()
{
	const_name_suffix=$2
	if [ -z "$const_name_suffix" ]; then const_name_suffix=$1; fi
	if [ -f $PNG_DIR/$1.png ]; then
		size=$(stat -c%s $PNG_DIR/$1.png)
		size_total=$((size_total+size))
		printf "%6d %s\n" $size $1.png
		../../../utils/bin2cc/bin2cc $PNG_DIR/$1.png $CC_DIR/png_$1.c png_$const_name_suffix
	else
		echo "file not found: $PNG_DIR/$1.png" >&2
	fi
}


stripFileType()
{
	ls ./"$PNG_DIR" -1 | sed 's/.png//' | tr '\n' ' ' 
}

convertToCC(){

size_total=0

	read input
	for image in $input
	do
		png2cc "$image"
	done

echo "total size: $size_total bytes"
}

png2indexed(){

read input

for image in $input 
do
	echo "$PNG_DIR/$image.png to $PAL_DIR/$image.png"
	$MAGICK_PATH "$PNG_DIR/$image.png" -type palette "$PAL_DIR/$image.png"
done

for image in $input
do
	optipng -fix -o7 -zc6 -zw32k -nb -nc  -strip all "$PAL_DIR/$image.png"
done

}

stripFileType | png2indexed
stripFileType | convertToCC

rm -rf $PAL_DIR
