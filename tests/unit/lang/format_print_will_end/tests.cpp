#include "catch2/catch.hpp"

#include <iostream>
#include <ctime>
#include <stdio.h>
#include "string_view_utf8.hpp"
#include "format_print_will_end.hpp"
#include "i18n.h"

using namespace std;
using Catch::Matchers::Equals;

TEST_CASE("format_print_will_end::Today", "[format_print_will_end]") {
    char tmp[32];

    int h, m;
    bool h24;
    const char *eq;
    std::tie(h, m, h24, eq) = GENERATE(
        std::make_tuple<int, int, bool, const char *>(12, 34, true, "Today at 12:34"),
        std::make_tuple<int, int, bool, const char *>(8, 1, true, "Today at 08:01"),
        std::make_tuple<int, int, bool, const char *>(6, 12, false, "Today at 06:12 AM"),
        std::make_tuple<int, int, bool, const char *>(6, 1, false, "Today at 06:01 AM"),
        std::make_tuple<int, int, bool, const char *>(17, 56, false, "Today at 05:56 PM"));
    tm t;
    t.tm_hour = h;
    t.tm_min = m;
    FormatMsgPrintWillEnd::Today(tmp, 32, &t, h24);

    CHECK_THAT(tmp, Equals(eq));
}

TEST_CASE("format_print_will_end::DayOfWeek", "[format_print_will_end]") {
    char tmp[32];
    int h, m, wd;
    bool h24;
    const char *eq;
    std::tie(h, m, wd, h24, eq) = GENERATE(
        // I'm really curious what day of week value is zero on the ARM - can be Sun or Mon ... we'll see
        // Hmmm, it looks like the week starts on Sunday... need to adjust the generator and the unit test
        std::make_tuple<int, int, int, bool, const char *>(12, 34, 1, true, "Mon 12:34"),
        std::make_tuple<int, int, int, bool, const char *>(8, 1, 1, true, "Mon 08:01"),
        std::make_tuple<int, int, int, bool, const char *>(6, 12, 1, false, "Mon 06:12 AM"),
        std::make_tuple<int, int, int, bool, const char *>(6, 1, 1, false, "Mon 06:01 AM"),
        std::make_tuple<int, int, int, bool, const char *>(17, 56, 1, false, "Mon 05:56 PM"),

        std::make_tuple<int, int, int, bool, const char *>(12, 34, 2, true, "Tue 12:34"),
        std::make_tuple<int, int, int, bool, const char *>(8, 1, 2, true, "Tue 08:01"),
        std::make_tuple<int, int, int, bool, const char *>(6, 12, 2, false, "Tue 06:12 AM"),
        std::make_tuple<int, int, int, bool, const char *>(6, 1, 2, false, "Tue 06:01 AM"),
        std::make_tuple<int, int, int, bool, const char *>(17, 56, 2, false, "Tue 05:56 PM"),

        std::make_tuple<int, int, int, bool, const char *>(12, 34, 3, true, "Wed 12:34"),
        std::make_tuple<int, int, int, bool, const char *>(12, 34, 4, true, "Thr 12:34"),
        std::make_tuple<int, int, int, bool, const char *>(12, 34, 5, true, "Fri 12:34"),
        std::make_tuple<int, int, int, bool, const char *>(12, 34, 6, true, "Sat 12:34"),
        std::make_tuple<int, int, int, bool, const char *>(12, 34, 0, true, "Sun 12:34"));
    tm t;
    t.tm_hour = h;
    t.tm_min = m;
    t.tm_wday = wd;
    FormatMsgPrintWillEnd::DayOfWeek(tmp, 32, &t, h24);

    CHECK_THAT(tmp, Equals(eq));
}

TEST_CASE("format_print_will_end::Date", "[format_print_will_end]") {
    char tmp[32];

    int h, m, d, mon;
    bool h24;
    const char *eq;
    FormatMsgPrintWillEnd::DateFormat fmt;
    std::tie(h, m, d, mon, fmt, h24, eq) = GENERATE(
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(12, 34, 1, 11, FormatMsgPrintWillEnd::ISO, true, "12-01 12:34"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(8, 1, 1, 11, FormatMsgPrintWillEnd::ISO, true, "12-01 08:01"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(6, 12, 1, 11, FormatMsgPrintWillEnd::ISO, false, "12-01 06:12 AM"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(6, 1, 1, 11, FormatMsgPrintWillEnd::ISO, false, "12-01 06:01 AM"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(17, 56, 1, 11, FormatMsgPrintWillEnd::ISO, false, "12-01 05:56 PM"),

        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(12, 34, 1, 11, FormatMsgPrintWillEnd::MD, true, "12/01 12:34"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(8, 1, 1, 11, FormatMsgPrintWillEnd::MD, true, "12/01 08:01"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(6, 12, 1, 11, FormatMsgPrintWillEnd::MD, false, "12/01 06:12 AM"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(6, 1, 1, 11, FormatMsgPrintWillEnd::MD, false, "12/01 06:01 AM"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(17, 56, 1, 11, FormatMsgPrintWillEnd::MD, false, "12/01 05:56 PM"),

        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(12, 34, 1, 11, FormatMsgPrintWillEnd::DM, true, "01/12 12:34"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(8, 1, 1, 11, FormatMsgPrintWillEnd::DM, true, "01/12 08:01"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(6, 12, 1, 11, FormatMsgPrintWillEnd::DM, false, "01/12 06:12 AM"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(6, 1, 1, 11, FormatMsgPrintWillEnd::DM, false, "01/12 06:01 AM"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(17, 56, 1, 11, FormatMsgPrintWillEnd::DM, false, "01/12 05:56 PM"),

        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(12, 34, 1, 11, FormatMsgPrintWillEnd::CS, true, "1.12. 12:34"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(8, 1, 1, 11, FormatMsgPrintWillEnd::CS, true, "1.12. 08:01"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(6, 12, 1, 11, FormatMsgPrintWillEnd::CS, false, "1.12. 06:12 AM"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(6, 1, 1, 11, FormatMsgPrintWillEnd::CS, false, "1.12. 06:01 AM"),
        std::make_tuple<int, int, int, int, FormatMsgPrintWillEnd::DateFormat, bool, const char *>(17, 56, 1, 11, FormatMsgPrintWillEnd::CS, false, "1.12. 05:56 PM"));

    tm t;
    t.tm_hour = h;
    t.tm_min = m;
    t.tm_wday = 0;
    t.tm_mday = d;
    t.tm_mon = mon;
    FormatMsgPrintWillEnd::Date(tmp, 32, &t, h24, fmt);

    CHECK_THAT(tmp, Equals(eq));
}
