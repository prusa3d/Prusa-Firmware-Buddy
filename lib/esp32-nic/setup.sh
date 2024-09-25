#!/bin/bash

echo "Downloading SDKs"
git clone -b v5.2.2 --depth 1 --recurse-submodules https://github.com/espressif/esp-idf.git .esp32-sdk

source ./paths.sh

echo "Installing the sdk"
${IDF_PATH}/install.sh
if [[ "$?" != 0 ]]; then
    echo "SDK Installation failed" 1>&2
    exit 1
fi

echo "Activating virtual environment"
source "${IDF_PATH}/export.sh"
if [[ "$?" != 0 ]]; then
    echo "Couldn't activate virtual environment." 1>&2
    exit 1
fi
