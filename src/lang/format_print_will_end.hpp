#pragma once

#include <time.h>

class FormatMsgPrintWillEnd {
    static size_t AppendStrftime(char *ptr, size_t maxsize, const tm *timeptr, bool hours24);

public:
    enum DateFormat {
        ISO,
        CS,
        MD,
        DM
    };

    static size_t Today(char *ptr, size_t maxsize, const tm *timeptr, bool hours24);
    static size_t DayOfWeek(char *ptr, size_t maxsize, const tm *timeptr, bool hours24);
    static size_t Date(char *ptr, size_t maxsize, const tm *timeptr, bool hours24, DateFormat dateFormat);
};
