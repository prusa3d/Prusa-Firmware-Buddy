#!/bin/bash

echo "Downloading SDKs"
git clone -b v3.4 --depth 1 --recurse-submodules  https://github.com/espressif/ESP8266_RTOS_SDK.git .esp8266-sdk

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
