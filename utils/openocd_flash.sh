#!/bin/sh

openocd --file utils/cproject/A3ides-Debug-OpenOCD.cfg --command "program build/mini_debug_noboot/firmware verify reset"
