#pragma once
#include <stdio.h>
#include "gcode_reader_interface.hpp"

int f_gcode_thumb_open(IGcodeReader &reader, FILE *fp);
int f_gcode_thumb_close(FILE *fp);
