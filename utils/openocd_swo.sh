#!/bin/sh

openocd --file utils/cproject/A3ides-SWO-OpenOCD.cfg -c "tpiu config internal /dev/stdout uart off 168000000 1680000; init"
