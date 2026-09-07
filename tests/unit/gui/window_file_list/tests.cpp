#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "WindowMenuItems.hpp"
#include "ourPosix.hpp"
#include "window_file_list.hpp"

using StringList = std::vector<std::string>;

constexpr int item_height = IWindowMenu::default_item_height + GuiDefaults::MenuItemDelimeterHeight;

Rect16 rect_for_items(int count) {
    return Rect16(0, 0, GuiDefaults::ScreenWidth, count * item_height);
}

// Two directories and twelve files; sorted by name the listing is
// "..", "boxes", "fw", "01.g" ... "12.g" (15 items)
void set_test_directory() {
    testFiles0 = {
        { "boxes", 2, true }, { "fw", 1, true },
        { "01.g", 1, false }, { "02.g", 2, false }, { "03.g", 3, false },
        { "04.g", 4, false }, { "05.g", 5, false }, { "06.g", 6, false },
        { "07.g", 7, false }, { "08.g", 8, false }, { "09.g", 9, false },
        { "10.g", 10, false }, { "11.g", 11, false }, { "12.g", 12, false }
    };
}

// Walks the focus by single knob steps until it stops, collecting the focused labels
StringList walk_focus(window_file_list_t &fl, int direction) {
    StringList r;
    while (fl.move_focus_by(direction)) {
        r.push_back(fl.CurrentLFN());
    }
    return r;
}

TEST_CASE("window_file_list: empty directory", "[window_file_list]") {
    testFiles0 = {};

    window_file_list_t fl(nullptr, rect_for_items(9));
    fl.Load(WF_SORT_BY_NAME, nullptr, nullptr);

    CHECK(fl.item_count() == 1); // just ".."
    REQUIRE(fl.focused_item_index() == 0);
    CHECK(fl.CurrentLFN() == std::string(".."));

    // ".." in the root is presented as a return item
    CHECK(dynamic_cast<MI_RETURN *>(fl.item_at(0)) != nullptr);

    CHECK(!fl.move_focus_by(1));
    CHECK(!fl.move_focus_by(-1));
}

TEST_CASE("window_file_list: listing and knob navigation", "[window_file_list]") {
    set_test_directory();

    // The whole ldv window fits on the screen
    window_file_list_t fl(nullptr, rect_for_items(window_file_list_t::item_buffer_size));
    REQUIRE(fl.max_items_on_screen_count() == window_file_list_t::item_buffer_size);

    fl.Load(WF_SORT_BY_NAME, nullptr, nullptr);
    CHECK(fl.item_count() == 15);

    // Load focuses the first entry after ".."
    REQUIRE(fl.focused_item_index() == 1);
    CHECK(fl.CurrentLFN() == std::string("boxes"));
    CHECK(fl.scroll_offset() == 0);
    CHECK(fl.TopItemSFN() == std::string(".."));

    // Walk the knob to the very end: directories first, then files by name
    CHECK(walk_focus(fl, 1) == StringList { "fw", "01.g", "02.g", "03.g", "04.g", "05.g", "06.g", "07.g", "08.g", "09.g", "10.g", "11.g", "12.g" });
    CHECK(fl.focused_item_index() == fl.item_count() - 1);
    CHECK(fl.scroll_offset() == fl.max_scroll_offset());
    CHECK(fl.TopItemSFN() == std::string("03.g"));

    // And back to the top
    CHECK(walk_focus(fl, -1) == StringList { "11.g", "10.g", "09.g", "08.g", "07.g", "06.g", "05.g", "04.g", "03.g", "02.g", "01.g", "fw", "boxes", ".." });
    CHECK(fl.focused_item_index() == 0);
    CHECK(fl.scroll_offset() == 0);
}

TEST_CASE("window_file_list: Load focuses the given file", "[window_file_list]") {
    set_test_directory();

    window_file_list_t fl(nullptr, rect_for_items(window_file_list_t::item_buffer_size));

    SECTION("cursor file within the first screen does not scroll") {
        fl.Load(WF_SORT_BY_NAME, "04.g", nullptr);
        CHECK(fl.CurrentSFN() == std::string("04.g"));
        CHECK(fl.scroll_offset() == 0);
    }

    SECTION("cursor file deep in the list scrolls it into view") {
        fl.Load(WF_SORT_BY_NAME, "12.g", nullptr);
        CHECK(fl.CurrentSFN() == std::string("12.g"));
        CHECK(fl.scroll_offset() == fl.max_scroll_offset());
        CHECK(fl.TopItemSFN() == std::string("03.g"));
    }

    SECTION("given top file restores the view") {
        fl.Load(WF_SORT_BY_NAME, "05.g", "02.g");
        CHECK(fl.TopItemSFN() == std::string("02.g"));
        CHECK(fl.CurrentSFN() == std::string("05.g"));
    }
}

TEST_CASE("window_file_list: sort by time", "[window_file_list]") {
    testFiles0 = {
        { "old.g", 1, false },
        { "newest.g", 30, false },
        { "newer.g", 20, false },
    };

    window_file_list_t fl(nullptr, rect_for_items(9));
    fl.Load(WF_SORT_BY_TIME, nullptr, nullptr);

    REQUIRE(fl.focused_item_index() == 1);
    CHECK(fl.CurrentLFN() == std::string("newest.g"));
    CHECK(walk_focus(fl, 1) == StringList { "newer.g", "old.g" });
}

TEST_CASE("window_file_list: screen smaller than the item buffer", "[window_file_list]") {
    set_test_directory();

    // Real printers show one item fewer than the item buffer holds - the buffer
    // is sized for the full screen height while the menu rect is smaller.
    constexpr int items_on_screen = window_file_list_t::item_buffer_size - 1;
    window_file_list_t fl(nullptr, rect_for_items(items_on_screen));
    REQUIRE(fl.max_items_on_screen_count() == items_on_screen);

    fl.Load(WF_SORT_BY_NAME, nullptr, nullptr);

    walk_focus(fl, 1);
    CHECK(fl.focused_item_index() == fl.item_count() - 1);
    CHECK(fl.CurrentLFN() == std::string("12.g"));
    CHECK(fl.scroll_offset() == fl.max_scroll_offset());
    CHECK(fl.TopItemSFN() == std::string("04.g"));

    // And back to the top
    walk_focus(fl, -1);
    CHECK(fl.focused_item_index() == 0);
    CHECK(fl.scroll_offset() == 0);
}
