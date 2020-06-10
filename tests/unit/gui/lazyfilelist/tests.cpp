#include "catch2/catch.hpp"

#include <iostream>
#include <assert.h>

#include "lazyfilelist.h"
#include "fatfs.h"

using namespace std;

template <typename LDV>
bool CheckFilesSeq(const LDV &ldv, std::vector<std::string> expectedSeq) {
    return std::mismatch(ldv.files.begin(), ldv.files.end(), expectedSeq.begin(),
               [](const typename LDV::Entry &e, const std::string &s) { return s == e.lfn; })
        == std::make_pair(ldv.files.end(), expectedSeq.end());
}

TEST_CASE("LazyDirView::Entries test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;
    {
        LDV::Entry e = { false, "\xff\xff\xff\xff\xff", "", 0xffff, 0xffff };
        LDV::Entry e1 = { false, "nold", "", 1, 1 };
        FILINFO f = { 0, 1, 1, AM_DIR, "old", "old" };

        CHECK(LDV::LessByTimeEF(e, f));
        CHECK(LDV::LessByTimeFE(f, e1));

        CHECK(!LDV::LessByFNameEF(e, f));
        CHECK(!LDV::LessByFNameFE(f, e1));
    }
}

TEST_CASE("LazyDirView::SortByName test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;

    testFiles0 = {
        { "old", 2, 0, true },
        { "fw", 3, 0, true },
        { "png-decode", 1, 0, true },
        { "01.g", 1, 0, false },
        { "02.g", 1, 0, false },
        { "03.g", 1, 0, false },
        { "04.g", 1, 0, false },
        { "05.g", 1, 0, false },
        { "06.g", 1, 0, false },
        { "07.g", 8, 0, false },
        { "08.g", 7, 0, false },
        { "09.g", 9, 0, false },
        { "10.g", 10, 0, false },
        { "11.g", 11, 0, false },
        { "12.g", 10, 1, false }
    };

    for (size_t i = 0; i < 20; ++i) {
        std::random_shuffle(testFiles0.begin(), testFiles0.end());

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
        { "old", 1, 0, true },
        { "fw", 2, 0, true },
        { "png-decode", 3, 0, true },
        { "01.g", 1, 0, false },
        { "02.g", 1, 0, false },
        { "03.g", 1, 0, false },
        { "04.g", 1, 0, false },
        { "05.g", 1, 0, false },
        { "06.g", 1, 0, false },
        { "07.g", 8, 0, false },
        { "08.g", 7, 0, false },
        { "09.g", 9, 0, false },
        { "10.g", 10, 0, false },
        { "11.g", 11, 0, false },
        { "12.g", 10, 1, false }
    };

    for (size_t i = 0; i < 20; ++i) {
        std::random_shuffle(testFiles0.begin(), testFiles0.end());
        LDV ldv;
        ldv.ChangeDirectory("path",
            LDV::SortPolicy::BY_CRMOD_DATETIME,
            nullptr);
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
    }
}

TEST_CASE("LazyDirView::StartWithDir test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;

    testFiles0 = {
        { "old", 1, 0, true },
        { "fw", 2, 0, true },
        { "png-decode", 3, 0, true },
        { "01.g", 1, 0, false },
        { "02.g", 1, 0, false },
        { "03.g", 1, 0, false },
        { "04.g", 1, 0, false },
        { "05.g", 1, 0, false },
        { "06.g", 1, 0, false },
        { "07.g", 8, 0, false },
        { "08.g", 7, 0, false },
        { "09.g", 9, 0, false },
        { "10.g", 10, 0, false },
        { "11.g", 11, 0, false },
        { "12.g", 10, 1, false }
    };

    for (size_t i = 0; i < 20; ++i) {
        std::random_shuffle(testFiles0.begin(), testFiles0.end());
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
    }
}

TEST_CASE("LazyDirView::StartWithFile test", "[LazyDirView]") {
    using LDV = LazyDirView<9>;

    testFiles0 = {
        { "old", 1, 0, true },
        { "fw", 2, 0, true },
        { "png-decode", 3, 0, true },
        { "01.g", 1, 0, false },
        { "02.g", 1, 0, false },
        { "03.g", 1, 0, false },
        { "04.g", 1, 0, false },
        { "05.g", 1, 0, false },
        { "06.g", 1, 0, false },
        { "07.g", 8, 0, false },
        { "08.g", 7, 0, false },
        { "09.g", 9, 0, false },
        { "10.g", 10, 0, false },
        { "11.g", 11, 0, false },
        { "12.g", 10, 1, false }
    };

    for (size_t i = 0; i < 20; ++i) {
        std::random_shuffle(testFiles0.begin(), testFiles0.end());
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
        { "3", 1, 0, true },
        { "tr.g", 2, 0, false },
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
