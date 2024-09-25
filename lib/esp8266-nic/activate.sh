#!/bin/bash

source ./paths.sh

echo "Activating virtual environment"
source "${IDF_PATH}/export.sh"
if [[ "$?" != 0 ]]; then
    echo "Couldn't activate virtual environment." 1>&2
    return 1
fi
