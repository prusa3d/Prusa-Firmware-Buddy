#pragma once
#include <stdio.h>
#include "gcode_reader.hpp"

int f_gcode_thumb_open(IGcodeReader &reader, FILE *fp);
int f_gcode_thumb_close(FILE *fp);
