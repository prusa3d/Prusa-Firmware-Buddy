#!/bin/sh

openocd --file utils/cproject/Buddy-Debug-OpenOCD.cfg --command "program build/mini_debug_noboot/firmware verify reset"
