#!/bin/sh
#

# determine workspace path from $0
WRK_DIR=$(echo "$0" | sed 's/\\/\//g')  # replace '\' with '/' (windows)
WRK_DIR=${WRK_DIR%/*}                   # Res folder
WRK_DIR=${WRK_DIR%/*}                   # Gui folder
WRK_DIR=${WRK_DIR%/*}                   # parent folder == workspace folder
cd $WRK_DIR

PNG_DIR=$WRK_DIR/Gui/Res/png
CC_DIR=$WRK_DIR/Gui/Res/cc

size_total=0

png2cc()
{
	const_name_suffix=$2
	if [ -z "$const_name_suffix" ]; then const_name_suffix=$1; fi
	if [ -f $PNG_DIR/$1.png ]; then
		size=$(stat -c%s $PNG_DIR/$1.png)
		size_total=$((size_total+size))
		printf "%6d %s\n" $size $1.png
		bin/win32/bin2cc $PNG_DIR/$1.png $CC_DIR/png_$1.c png_$const_name_suffix
	else
		echo "file not found: $PNG_DIR/$1.png" >&2
	fi
}

png2cc icon_pepa
png2cc msgbox_icon_error
png2cc msgbox_icon_question
png2cc msgbox_icon_warning
png2cc msgbox_icon_info
png2cc wizard_icon_na
png2cc wizard_icon_ok
png2cc wizard_icon_ng
png2cc wizard_icon_ip0
png2cc wizard_icon_ip1
png2cc wizard_icon_hourglass
png2cc wizard_icon_autohome
png2cc wizard_icon_measure
png2cc wizard_icon_search

png2cc statusscreen_logo_prusamini
png2cc statusscreen_logo_prusamini_debug

png2cc icon_pepa_psod

echo "total size: $size_total bytes"

read x
