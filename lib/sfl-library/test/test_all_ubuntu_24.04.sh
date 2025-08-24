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
run "make CXX=\"g++     -std=c++14\" SILENT=1 -j$(nproc)"
run "make CXX=\"g++     -std=c++17\" SILENT=1 -j$(nproc)"
run "make CXX=\"g++     -std=c++20\" SILENT=1 -j$(nproc)"
run "make CXX=\"g++     -std=c++23\" SILENT=1 -j$(nproc)"
run "make CXX=\"clang++ -std=c++11\" SILENT=1 -j$(nproc)"
run "make CXX=\"clang++ -std=c++14\" SILENT=1 -j$(nproc)"
run "make CXX=\"clang++ -std=c++17\" SILENT=1 -j$(nproc)"
run "make CXX=\"clang++ -std=c++20\" SILENT=1 -j$(nproc)"
run "make CXX=\"clang++ -std=c++23\" SILENT=1 -j$(nproc)"

echo :::: Finished all tests.
echo :::: THE END.
