#!/bin/sh
BUILD_DIR=./build
TESTS_BUILD_DIR=$BUILD_DIR/tests/CMakeFiles/
COV_INFO=$TESTS_BUILD_DIR/bitsery_coverage.info
lcov --directory $TESTS_BUILD_DIR --capture --output-file $COV_INFO
lcov --extract $COV_INFO '*include/bitsery*' --output-file $COV_INFO.clean
genhtml --output-directory $TESTS_BUILD_DIR/coverage_web $COV_INFO.clean
x-www-browser $TESTS_BUILD_DIR/coverage_web/index.html
