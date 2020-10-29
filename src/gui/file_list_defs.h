#pragma once
#include "ffconf.h"

/*
 * Using limits from marlin:
 * FILENAME_LENGTH				13
 * LONG_FILENAME_LENGTH			13 * 5 + 1
 * MAX_DIR_DEPTH				10
 * MAXDIRNAMELENGTH   			8
 * MAXPATHNAMELENGTH			(1 + (MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + FILENAME_LENGTH)
 * */

#ifndef MAX_DIR_DEPTH
    #define MAX_DIR_DEPTH 10 // Maximum folder depth
#endif

#ifndef MAXDIRNAMELENGTH
    #define F_MAXDIRNAMELENGTH FF_MAX_LFN + 1
#endif

#ifndef MAXPATHNAMELENGTH
    #define F_MAXPATHNAMELENGTH (1 + (F_MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + FF_MAX_LFN)
#endif
