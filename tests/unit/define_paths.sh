#!/bin/bash
function fullPath {
	echo "$(cd "$(dirname "$1")"; pwd -P)/$(basename "$1")"
}

function addPath {
	fp=`fullPath $1`
	# echo $fp
	export PATH="$fp:$PATH"
}

Catch2_SOURCE_DIR="../../../Catch2"
CMAKE_SOURCE_DIR="../../../../"

addPath "../../.dependencies/cmake-3.15.5/bin"
addPath "../../.dependencies/ninja-1.9.0/"
# path to compiler (g++)
addPath "/c/Programs/MinGW/bin/"
