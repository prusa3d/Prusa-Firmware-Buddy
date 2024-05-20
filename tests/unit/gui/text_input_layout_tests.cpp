#include <catch2/catch.hpp>
#include <dialog_text_input_layout.in.cpp>

using namespace dialog_text_input;

#if HAS_LARGE_DISPLAY()
    #define TESTNAME "gui::text_input_layout::big_display"
#else
    #define TESTNAME "gui::text_input_layout::mini_display"
#endif

TEST_CASE(TESTNAME) {
    constexpr int special_buttons_cnt = static_cast<int>(SpecialButton::_cnt);

    struct Stats {
        /// How many times each special button has been used
        std::array<int, special_buttons_cnt> special_buttons = { 0 };

        /// How many times each character has been used
        std::array<int, 256> characters = { 0 };
    };

    const auto compute_layout_stats = [](const ButtonsLayout &layout) {
        Stats stats;

        for (const auto &row : layout) {
            for (const ButtonRec &button : row) {
                if (button.is_special()) {
                    stats.special_buttons[static_cast<int>(button.to_special_button())]++;

                } else {
                    for (char ch : button) {
                        stats.characters[ch]++;
                    }
                }
            }
        }

        // Reset '\0' character, we don't care about it
        stats.characters[0] = 0;

        return stats;
    };

    const auto special_button_cnt = [&](const Stats &stats, SpecialButton btn) {
        return stats.special_buttons[static_cast<size_t>(btn)];
    };

    Stats total_stats;

    const auto test_layout = [&](const ButtonsLayout &layout) {
        Stats layout_stats = compute_layout_stats(layout);

        // Accumulate total stats
        for (size_t i = 0; i < total_stats.characters.size(); i++) {
            INFO("Character '" << char(i) << "' should not repeat within a layout");
            CHECK(layout_stats.characters[i] < 2);
            total_stats.characters[i] += layout_stats.characters[i];
        }
        for (size_t i = 0; i < special_buttons_cnt; i++) {
            INFO("Special button " << i << " should not repeat within a layout");
            CHECK(layout_stats.special_buttons[i] < 2);
            total_stats.special_buttons[i] += layout_stats.special_buttons[i];
        }

        // Each layout must contain these special buttons
        CHECK(special_button_cnt(layout_stats, SpecialButton::backspace) == 1);
        CHECK(special_button_cnt(layout_stats, SpecialButton::ok) == 1);
        CHECK(special_button_cnt(layout_stats, SpecialButton::cancel) == 1);
        CHECK(special_button_cnt(layout_stats, SpecialButton::clear) == 1);

        // Check that we're not repeating a character in the same layout
        for (size_t ch = 0; ch < layout_stats.characters.size(); ch++) {
            INFO("Repeating character '" << char(ch) << "'");
            CHECK(layout_stats.characters[ch] < 2);
        }

        return layout_stats;
    };

    // Check that a given layout contains specified characters
    const auto check_characters = [](const char *chars, Stats stats, bool exactly_once = true, bool disallow_other_characters = false) {
        char ch;
        while (ch = *(chars++)) {
            if (exactly_once) {
                INFO("Character '" << char(ch) << "' should appear exactly once");
                CHECK(stats.characters[ch] == 1);
            } else {
                INFO("Character '" << char(ch) << "' should appear at least once");
                CHECK(stats.characters[ch] > 0);
            }

            stats.characters[ch] = 0;
        }

        if (disallow_other_characters) {
            for (size_t ch = 0; ch < stats.characters.size(); ch++) {
                INFO("Unexpected character '" << char(ch) << "'");
                CHECK(stats.characters[ch] == 0);
            }
        }
    };

    {
        INFO("layout_text_lowercase");
        const Stats stats = test_layout(layout_text_lowercase);
        check_characters("abcdefghijklmnopqrstuvwxyz", stats);
        CHECK(special_button_cnt(stats, SpecialButton::uppercase) == 1);
        CHECK(special_button_cnt(stats, SpecialButton::space) == 1);
    }

    {
        INFO("layout_text_uppercase");
        const Stats stats = test_layout(layout_text_uppercase);
        check_characters("ABCDEFGHIJKLMNOPQRSTUVWXYZ", stats);
        CHECK(special_button_cnt(stats, SpecialButton::lowercase) == 1);
        CHECK(special_button_cnt(stats, SpecialButton::space) == 1);
    }

    {
        INFO("layout_symbols");
        const Stats stats = test_layout(layout_symbols);
        CHECK(special_button_cnt(stats, SpecialButton::lowercase) == 1);
    }

#if HAS_NUMBERS_LAYOUT()
    {
        INFO("layout_numbers");
        const Stats stats = test_layout(layout_numbers);
        check_characters("0123456789-", stats);
        CHECK(special_button_cnt(stats, SpecialButton::lowercase) == 1);
    }
#endif

    {
        INFO("Total stats");

        // These special buttons must be used at least once
        CHECK(special_button_cnt(total_stats, SpecialButton::uppercase) > 0);
        CHECK(special_button_cnt(total_stats, SpecialButton::lowercase) > 0);
        CHECK(special_button_cnt(total_stats, SpecialButton::symbols) > 0);

#if HAS_NUMBERS_LAYOUT()
        CHECK(special_button_cnt(total_stats, SpecialButton::numbers) > 0);
#endif

        // Check that the alphabet is exactly once in the layout
        check_characters("abcdefghijklmnopqrstuvwxyz", total_stats, true);
        check_characters("ABCDEFGHIJKLMNOPQRSTUVWXYZ", total_stats, true);

        // Check that we have all characters that we want to
        check_characters(
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789"
            "+-*/"
            "|&#$@%"
            "()[]{}<>"
            ".,?!:;"
            "_~="
            "\"'",
            total_stats, false, true);
    }
}
