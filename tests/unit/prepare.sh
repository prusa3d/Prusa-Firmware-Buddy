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


#addPath "/c/Programs/cygwin64/bin/"
addPath "/c/Programs/MinGW/bin/"


# cd ../..
# mkdir build_tests
# cd build_tests
# cmake .. -G Ninja

# # build all the unit tests
# ninja tests

# # run the unit tests
# ctest .

#export PATH=cat `fp "../../.dependencies/cmake-3.15.5/bin"`:$PATH"
#export PATH="../../.dependencies/ninja-1.9.0/:$PATH"

#export CMAKE_C_COMPILER="/c/Programs/MinGW/bin/gcc.exe"
#export CMAKE_CXX_COMPILER="/c/Programs/MinGW/bin/g++.exe"
