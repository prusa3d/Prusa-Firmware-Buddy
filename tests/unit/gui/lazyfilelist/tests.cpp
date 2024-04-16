#include "catch2/catch.hpp"

#include <random>
#include <iostream>
#include <assert.h>

#include "ourPosix.hpp"
#include "lazyfilelist.hpp"
#include "file_sort.hpp"
#include <string.h>

// to be able to compile under Windows
// In the POSIX locale, strcasecmp() and strncasecmp() shall behave as if the strings had been converted to
// lowercase and then a byte comparison performed. The results are unspecified in other locales.
// portable solution consider that round trips the letter (to upper then to lower) to cope with non 1-to-1 mappings:
#ifndef _STRINGS_H
extern "C" __attribute__((weak)) int strcasecmp(const char *a, const char *b) {
    int ca, cb;
    do {
        ca = (unsigned char)*a++;
        cb = (unsigned char)*b++;
        ca = tolower(toupper(ca));
        cb = tolower(toupper(cb));
    } while (ca == cb && ca != '\0');
    return ca - cb;
}
#endif // _STRINGS_H

using namespace std;

template <typename LDV>
bool CheckFilesSeq(const LDV &ldv, std::vector<std::string> expectedSeq) {
    return std::mismatch(ldv.data().begin(), ldv.data().end(), expectedSeq.begin(),
               [](const typename LDV::Entry &e, const std::string &s) { return s == e.lfn; })
        == std::make_pair(ldv.data().end(), expectedSeq.end());
}

static char txt_old[] = "old"; // cannot be const char

TEST_CASE("LazyDirView::Entries test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;
    {
        LDV::Entry e = { .time = LONG_MAX, .type = FileSort::EntryType::DIR, .lfn = "\xff\xff\xff\xff\xff", .sfn = "" };
        LDV::Entry e1 = { .time = 1, .type = FileSort::EntryType::DIR, .lfn = "nold", .sfn = "" };
        dirent f = { DT_DIR, "old", txt_old, 1 };
        LDV::EntryRef dwp(f, "we/dont/care/about/transfer/here");

        CHECK(LDV::less_by_time(e, dwp));
        CHECK(LDV::less_by_time(dwp, e1));

        CHECK(!LDV::less_by_name(e, dwp));
        CHECK(!LDV::less_by_name(dwp, e1));
    }
}

TEST_CASE("LazyDirView::SortByName test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;

    testFiles0 = {
        { "old", UINT_LEAST64_MAX, true },
        { "fw", 3, true },
        { "png-decode", 1, true },
        { "01.g", 1, false },
        { "02.g", 1, false },
        { "03.g", 1, false },
        { "04.g", 1, false },
        { "05.g", 1, false },
        { "06.g", 1, false },
        { "07.g", 8, false },
        { "08.g", 7, false },
        { "09.g", 9, false },
        { "10.g", 10, false },
        { "11.g", 11, false },
        { "12.g", UINT_LEAST64_MAX, false }
    };

    for (size_t i = 0; i < 20; ++i) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(testFiles0.begin(), testFiles0.end(), g);

        LDV ldv;
        ldv.ChangeDirectory("path",
            LDV::SortPolicy::BY_NAME,
            nullptr);
        CHECK(CheckFilesSeq(ldv, { "..", "fw", "old", "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "fw", "old", "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g", "06.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "old", "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g", "06.g", "07.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g", "06.g", "07.g", "08.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "01.g", "02.g", "03.g", "04.g", "05.g", "06.g", "07.g", "08.g", "09.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "02.g", "03.g", "04.g", "05.g", "06.g", "07.g", "08.g", "09.g", "10.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "03.g", "04.g", "05.g", "06.g", "07.g", "08.g", "09.g", "10.g", "11.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "04.g", "05.g", "06.g", "07.g", "08.g", "09.g", "10.g", "11.g", "12.g" }));
        CHECK(ldv.MoveDown() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "04.g", "05.g", "06.g", "07.g", "08.g", "09.g", "10.g", "11.g", "12.g" }));

        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "03.g", "04.g", "05.g", "06.g", "07.g", "08.g", "09.g", "10.g", "11.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "02.g", "03.g", "04.g", "05.g", "06.g", "07.g", "08.g", "09.g", "10.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "01.g", "02.g", "03.g", "04.g", "05.g", "06.g", "07.g", "08.g", "09.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g", "06.g", "07.g", "08.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "old", "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g", "06.g", "07.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "fw", "old", "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g", "06.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "..", "fw", "old", "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g" }));
        CHECK(ldv.MoveUp() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "..", "fw", "old", "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g" }));

        CHECK(ldv.MoveDown(3));
        CHECK(CheckFilesSeq(ldv, { "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g", "06.g", "07.g", "08.g" }));

        CHECK(ldv.MoveUp(3));
        CHECK(CheckFilesSeq(ldv, { "..", "fw", "old", "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g" }));

        CHECK(ldv.MoveDown(7));
        CHECK(CheckFilesSeq(ldv, { "04.g", "05.g", "06.g", "07.g", "08.g", "09.g", "10.g", "11.g", "12.g" }));

        CHECK(ldv.MoveUp(7));
        CHECK(CheckFilesSeq(ldv, { "..", "fw", "old", "png-decode", "01.g", "02.g", "03.g", "04.g", "05.g" }));
    }
}
TEST_CASE("LazyDirView::SortByCrModDateTime test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;

    testFiles0 = {
        { "old", 1, true },
        { "fw", 2, true },
        { "png-decode", UINT_LEAST64_MAX - 2, true },
        { "01.g", 1, false },
        { "02.g", 1, false },
        { "03.g", 1, false },
        { "04.g", 1, false },
        { "05.g", 1, false },
        { "06.g", 1, false },
        { "07.g", 8, false },
        { "08.g", 7, false },
        { "09.g", 9, false },
        { "10.g", 10, false },
        { "11.g", 11, false },
        { "12.g", 10, false }
    };

    for (size_t i = 0; i < 20; ++i) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(testFiles0.begin(), testFiles0.end(), g);
        LDV ldv;
        ldv.ChangeDirectory("path",
            LDV::SortPolicy::BY_CRMOD_DATETIME,
            nullptr);
        CHECK(ldv.MoveUp() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "..", "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g" }));
        CHECK(ldv.MoveDown());
        // mno, tady je otazka, proc tu neni na konci 01.g - jestli by nahodou fajly nemely bejt razeny podle nazvu vzestupne
        CHECK(CheckFilesSeq(ldv, { "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g", "01.g" }));

        CHECK(ldv.MoveDown() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g", "01.g" }));

        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "..", "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g" }));
        CHECK(ldv.MoveUp() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "..", "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g" }));
        CHECK(ldv.MoveUp() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "..", "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g" }));
    }
}

TEST_CASE("LazyDirView::StartWithDir test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;

    testFiles0 = {
        { "old", 1, true },
        { "fw", 2, true },
        { "png-decode", 3, true },
        { "01.g", 1, false },
        { "02.g", 1, false },
        { "03.g", 1, false },
        { "04.g", 1, false },
        { "05.g", 1, false },
        { "06.g", 1, false },
        { "07.g", 8, false },
        { "08.g", 7, false },
        { "09.g", 9, false },
        { "10.g", 10, false },
        { "11.g", 11, false },
        { "12.g", 10, false }
    };

    for (size_t i = 0; i < 20; ++i) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(testFiles0.begin(), testFiles0.end(), g);
        LDV ldv;
        ldv.ChangeDirectory("path", LDV::SortPolicy::BY_CRMOD_DATETIME, "fw");
        CHECK(CheckFilesSeq(ldv, { "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g" }));
        CHECK(ldv.MoveDown());
        CHECK(CheckFilesSeq(ldv, { "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g", "01.g" }));
        CHECK(ldv.MoveDown() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g", "01.g" }));

        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "..", "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g" }));
        CHECK(ldv.MoveUp() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "..", "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g" }));
        CHECK(ldv.MoveUp() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "..", "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g" }));
    }
}

TEST_CASE("LazyDirView::StartWithFile test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;

    testFiles0 = {

        { "old", 1, true },
        { "fw", 2, true },
        { "png-decode", 3, true },
        { "01.g", 1, false },
        { "02.g", 1, false },
        { "03.g", 1, false },
        { "04.g", 1, false },
        { "05.g", 1, false },
        { "06.g", 1, false },
        { "07.g", 8, false },
        { "08.g", 7, false },
        { "09.g", 9, false },
        { "10.g", 10, false },
        { "11.g", 11, false },
        { "12.g", 10, false }
    };
    for (size_t i = 0; i < 20; ++i) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(testFiles0.begin(), testFiles0.end(), g);
        LDV ldv;
        ldv.ChangeDirectory("path", LDV::SortPolicy::BY_CRMOD_DATETIME, "07.g");
        // and this is gonna be interesting
        CHECK(CheckFilesSeq(ldv, { "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g", "01.g", "" }));
        CHECK(ldv.MoveDown() == false);
        CHECK(CheckFilesSeq(ldv, { "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g", "01.g", "" }));

        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g", "01.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g", "02.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g", "03.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g", "04.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g", "05.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g", "06.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g", "08.g" }));
        CHECK(ldv.MoveUp());
        CHECK(CheckFilesSeq(ldv, { "..", "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g" }));
        CHECK(ldv.MoveUp() == false); // no more files
        CHECK(CheckFilesSeq(ldv, { "..", "png-decode", "fw", "old", "11.g", "12.g", "10.g", "09.g", "07.g" }));
    }
}

TEST_CASE("LazyDirView::StartWithFile2 test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;

    testFiles0 = {
        { "3", 1, true },
        { "tr.g", 2, false },
    };

    LDV ldv;
    ldv.ChangeDirectory("path", LDV::SortPolicy::BY_CRMOD_DATETIME, "tr.g");
    // and this is gonna be interesting
    CHECK(CheckFilesSeq(ldv, { "tr.g", "", "", "", "", "", "", "", "" }));
    CHECK(ldv.MoveDown() == false);
    CHECK(CheckFilesSeq(ldv, { "tr.g", "", "", "", "", "", "", "", "" }));

    CHECK(ldv.MoveUp());
    CHECK(CheckFilesSeq(ldv, { "3", "tr.g", "", "", "", "", "", "", "" }));
    CHECK(ldv.MoveUp());
    CHECK(CheckFilesSeq(ldv, { "..", "3", "tr.g", "", "", "", "", "", "" }));
    CHECK(ldv.MoveUp() == false);
    CHECK(CheckFilesSeq(ldv, { "..", "3", "tr.g", "", "", "", "", "", "" }));
}

TEST_CASE("LazyDirView::ExtremelyLongFileName test", "[LazyDirView][!shouldfail]") {
    using LDV = LazyDirView<9>;

    static const char extremeLFN[] = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff - kopie - kopie.gcode";
    char truncatedLFN[FF_LFN_BUF + 1];
    strlcpy(truncatedLFN, extremeLFN, sizeof(truncatedLFN)); // prepare the truncated string
    testFiles0 = {
        { "3", 1, true },
        { extremeLFN, 2, false },
    };

    LDV ldv;
    ldv.ChangeDirectory("path", LDV::SortPolicy::BY_CRMOD_DATETIME, nullptr);
    // And here I don't expect the correct path to be in the file list - it is just too long for that
    // For now this test is expected to fail, to be solved in BFW-1041
    CHECK(CheckFilesSeq(ldv, { "3", truncatedLFN, "", "", "", "", "", "", "" }));
    // the file list may survive this unharmed, but I must also make sure the surrounding code survives as well
    // -> the truncated filename does not exist at all or may even be identical to some other (deliberately chosen) filename
    // and we must make sure the user prints the right one (the chosen one)
}

TEST_CASE("LazyDirView::HoppingAround", "[LazyDirView]") {
    using LDV = LazyDirView<9>;

    testFiles0 = {
        { "V_22.bgcode", 1695309736, false },
        { "W_23.bgcode", 1695309738, false },
        { "X_24.bgcode", 1695309740, false },
        { "Y_25.bgcode", 1695309742, false },
        { "Z_26.bgcode", 1695309744, false },
        { "1_27.bgcode", 1695309746, false },
        { "2_28.bgcode", 1695309748, false },
        { "3_29.bgcode", 1695309780, false },
        { "4_30.bgcode", 1695309782, false },
        { "5_31.bgcode", 1695309784, false },
        { "6_32.bgcode", 1695309786, false },
        { "7_33.bgcode", 1695309788, false },
        { "8_34.bgcode", 1695309790, false },
        { "9_35.bgcode", 1695309792, false },
        { "0_36.bgcode", 1695309794, false },
        { "Á_37.bgcode", 1695309796, false },
        { "Č_38.bgcode", 1695309798, false },
        { "Ď_39.bgcode", 1695309800, false },
        { "É_40.bgcode", 1695309802, false },
        { "Ě_41.bgcode", 1695309804, false },
        { "Í_42.bgcode", 1695309806, false },
        { "Ň_43.bgcode", 1695309808, false },
        { "Ó_44.bgcode", 1695309840, false },
        { "Ř_45.bgcode", 1695309842, false },
        { "Š_46.bgcode", 1695309844, false },
        { "Ť_47.bgcode", 1695309846, false },
        { "Ú_48.bgcode", 1695309848, false },
        { "Ů_49.bgcode", 1695309850, false },
        { "Ý_50.bgcode", 1695309852, false },
        { "Ž_51.bgcode", 1695309854, false },
        { "A_1.bgcode", 1695309663, false },
        { "B_2.bgcode", 1695309665, false },
        { "C_3.bgcode", 1695309667, false },
        { "D_4.bgcode", 1695309669, false },
        { "E_5.bgcode", 1695309671, false },
        { "složka", 1695309672, true },
        { "slozka", 1695309673, true },
        { "F_6.bgcode", 1695309673, false },
        { "G_7.bgcode", 1695309675, false },
        { "H_8.bgcode", 1695309677, false },
        { "I_9.bgcode", 1695309679, false },
        { "J_10.bgcode", 1695309681, false },
        { "K_11.bgcode", 1695309683, false },
        { "L_12.bgcode", 1695309685, false },
        { "M_13.bgcode", 1695309687, false },
        { "N_14.bgcode", 1695309689, false },
        { "O_15.bgcode", 1695309721, false },
        { "P_16.bgcode", 1695309723, false },
        { "Q_17.bgcode", 1695309725, false },
        { "R_18.bgcode", 1695309727, false },
        { "S_19.bgcode", 1695309729, false },
        { "T_20.bgcode", 1695309732, false },
        { "U_21.bgcode", 1695309734, false },
    };

    LDV ldv;
    ldv.ChangeDirectory("path", LDV::SortPolicy::BY_CRMOD_DATETIME, nullptr);
    CHECK(CheckFilesSeq(ldv, { "..", "slozka", "složka", "Ž_51.bgcode", "Ý_50.bgcode", "Ů_49.bgcode", "Ú_48.bgcode", "Ť_47.bgcode", "Š_46.bgcode" }));

    ldv.set_window_offset(44);
    CHECK(CheckFilesSeq(ldv, { "I_9.bgcode", "H_8.bgcode", "G_7.bgcode", "F_6.bgcode", "E_5.bgcode", "D_4.bgcode", "C_3.bgcode", "B_2.bgcode", "A_1.bgcode" }));

    ldv.move_window_by(-8);
    CHECK(CheckFilesSeq(ldv, { "Q_17.bgcode", "P_16.bgcode", "O_15.bgcode", "N_14.bgcode", "M_13.bgcode", "L_12.bgcode", "K_11.bgcode", "J_10.bgcode", "I_9.bgcode" }));
}
