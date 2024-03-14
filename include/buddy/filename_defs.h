#pragma once
#include "file_list_defs.h"

#ifndef MAXDIRNAMELENGTH
    #define MAXPATHNAMELENGTH (1 + (F_MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + FF_MAX_LFN)
#else
    #define MAXPATHNAMELENGTH (1 + (MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + FILENAME_LENGTH) // "/" + N * ("ADIRNAME/") + "filename.ext"
#endif

// FIXME why was there same macros as in file_list_defs.h but with different values?
#if (0)
/*
 * Using limits from marlin:
 * FILENAME_LENGTH				13
 * LONG_FILENAME_LENGTH			13 * 5 + 1
 * MAX_DIR_DEPTH				10
 * MAXDIRNAMELENGTH   			8
 * MAXPATHNAMELENGTH			(1 + (MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + FILENAME_LENGTH)
 * */

    #define MAX_DIR_DEPTH    10 // Maximum folder depth
    #define MAXDIRNAMELENGTH 8 // DOS folder name size
    #define _MAX_LFN         (96 + 1 + 5 + 1) // Maximum LFN length to handle (12 to 255)

    #ifdef _MAX_LFN
        #define F_MAXDIRNAMELENGTH _MAX_LFN + 1
        #define MAXPATHNAMELENGTH  (1 + (F_MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + _MAX_LFN)
    #else
        #define MAXPATHNAMELENGTH (1 + (MAXDIRNAMELENGTH + 1) * (MAX_DIR_DEPTH) + 1 + FILENAME_LENGTH) // "/" + N * ("ADIRNAME/") + "filename.ext"
    #endif

#endif // 0
