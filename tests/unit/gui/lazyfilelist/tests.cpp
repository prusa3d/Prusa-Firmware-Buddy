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
    return std::mismatch(ldv.files.begin(), ldv.files.end(), expectedSeq.begin(),
               [](const typename LDV::Entry &e, const std::string &s) { return s == e.lfn; })
        == std::make_pair(ldv.files.end(), expectedSeq.end());
}

static char txt_old[] = "old"; // cannot be const char

TEST_CASE("LazyDirView::Entries test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;
    {
        LDV::Entry e = { false, "\xff\xff\xff\xff\xff", "", LONG_MAX };
        LDV::Entry e1 = { false, "nold", "", 1 };
        dirent f = { DT_DIR, "old", txt_old, 1 };
        MutablePath p { "we/dont/care/about/transfer/here" };
        FileSort::DirentWPath dwp { f, p };

        CHECK(LDV::LessByTimeEF(e, dwp));
        CHECK(LDV::LessByTimeFE(dwp, e1));

        CHECK(!LDV::LessByFNameEF(e, dwp));
        CHECK(!LDV::LessByFNameFE(dwp, e1));
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
