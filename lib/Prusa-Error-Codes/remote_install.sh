#!/bin/sh

# This file is part of the SL1 firmware
# Copyright (C) 2020 Prusa Research a.s. - www.prusa3d.com
# SPDX-License-Identifier: GPL-3.0-or-later

# Resolve target
if [ "$#" -ne 1 ]; then
    echo "Please provide target ip as the only argument"
    exit -1
fi
target=${1}
echo "Target is ${target}"

# Print commands being executed
set -o xtrace

# Create temp root
tmp=$(mktemp --directory --tmpdir=/tmp/ prusa_errors.XXXX)
echo "Local temp is ${tmp}"

echo "Running setup"
python3 setup.py sdist --dist-dir=${tmp}

# Create remote temp
target_tmp=$(ssh root@${target} "mktemp --directory --tmpdir=/tmp/ prusa_errors.XXXX")
echo "Remote temp is ${target_tmp}"

echo "Installing on target"
scp -r ${tmp}/* root@${target}:${target_tmp}
ssh root@${target} "\
set -o xtrace; \
cd ${target_tmp}; \
tar xvf prusa_errors*.tar.gz; \
rm prusa_errors*.tar.gz; \
cd prusa_errors-*; \
pip3 install . ; \
"

echo "Removing remote temp"
ssh root@${target} "rm -rf ${target_tmp}"

echo "Removing local temp"
rm -rf ${tmp}
