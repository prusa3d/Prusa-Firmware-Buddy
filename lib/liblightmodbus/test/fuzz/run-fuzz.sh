#!/bin/sh
cd "$(dirname "$0")"
tar xvf slave_samples.tar
AFL_USE_ASAN=1 afl-clang++ -Wall -I../../include slavefuzz.cpp -m32 -g -O3 -DFUZZ -o slavefuzz
AFL_SKIP_CPUFREQ=1 afl-fuzz -i slave_samples -o out -M master -m 1024 ./slavefuzz