#!/bin/bash

mkdir palette


PNG_DIR=./png
PAL_DIR=./palette

png2indexed(){

read input

for image in $input
do
	echo "$PNG_DIR/$image.png to $PAL_DIR/$image.png"
	#uses image magick tool to convert png from normal to indexed

	$MAGICK_PATH "$PNG_DIR/$image.png" -type PaletteAlpha  -define PNG:bit-depth=8 "$PAL_DIR/$image.png"

done

for image in $input
do
	optipng -fix -o7 -zc6 -zw32k   -q -strip all "$PAL_DIR/$image.png"
done

}

stripFileType()
{
	ls ./"$PNG_DIR" -1 | sed 's/.png//' | tr '\n' ' '
}

stripFileType | png2indexed
