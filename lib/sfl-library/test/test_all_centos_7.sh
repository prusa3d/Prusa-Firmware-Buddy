#!/bin/bash

run()
{
    # Print command
    echo :::: Starting test \[$1\]

    # Execute command
    eval $1

    if [ $? -ne 0 ] ; then
        echo :::: ERROR. Test failed \[$1\]
        echo :::: ALL STOP.
        exit 1
    fi

    echo :::: Finished test \[$1\]
}

run "make CXX=\"g++     -std=c++11\" SILENT=1 -j$(nproc)"
run "make CXX=\"clang++ -std=c++11\" SILENT=1 -j$(nproc)"

run "make CXX=\"g++     -std=c++11 -fno-exceptions -DSFL_NO_EXCEPTIONS\" SILENT=1 -j$(nproc)"
run "make CXX=\"clang++ -std=c++11 -fno-exceptions -DSFL_NO_EXCEPTIONS\" SILENT=1 -j$(nproc)"

if command -v /opt/rh/devtoolset-7/root/bin/g++ > /dev/null 2>&1; then

run "make CXX=\"/opt/rh/devtoolset-7/root/bin/g++ -std=c++11\" SILENT=1 -j$(nproc)"
run "make CXX=\"/opt/rh/devtoolset-7/root/bin/g++ -std=c++14\" SILENT=1 -j$(nproc)"
run "make CXX=\"/opt/rh/devtoolset-7/root/bin/g++ -std=c++17\" SILENT=1 -j$(nproc)"

fi

if command -v /opt/rh/llvm-toolset-7/root/bin/clang++ > /dev/null 2>&1; then

run "make CXX=\"/opt/rh/llvm-toolset-7/root/bin/clang++ -std=c++11\" SILENT=1 -j$(nproc)"
run "make CXX=\"/opt/rh/llvm-toolset-7/root/bin/clang++ -std=c++14\" SILENT=1 -j$(nproc)"
run "make CXX=\"/opt/rh/llvm-toolset-7/root/bin/clang++ -std=c++17\" SILENT=1 -j$(nproc)"

fi

echo :::: Finished all tests.
echo :::: THE END.
