/**
 * @file menu_opener.hpp
 * @brief some tools to simplify screen open in menu
 */

#pragma once

#include "i_window_menu_item.hpp"
#include "ScreenFactory.hpp"
#include "screen.hpp"
#include <algorithm>

/**
 * @brief macros to convert "String" to 'S','t','r','i','n','g','\0' ... '\0'
 * TODO reduce number of '\0'
 */
#define STRING_TO_LETTERS_1(str, i) \
    (sizeof(str) > (i) ? str[(i)] : 0)

#define STRING_TO_LETTERS_4(str, i)      \
    STRING_TO_LETTERS_1(str, i + 0),     \
        STRING_TO_LETTERS_1(str, i + 1), \
        STRING_TO_LETTERS_1(str, i + 2), \
        STRING_TO_LETTERS_1(str, i + 3)

#define STRING_TO_LETTERS_16(str, i)     \
    STRING_TO_LETTERS_4(str, i + 0),     \
        STRING_TO_LETTERS_4(str, i + 4), \
        STRING_TO_LETTERS_4(str, i + 8), \
        STRING_TO_LETTERS_4(str, i + 12)

#define STRING_TO_LETTERS_STR(str) STRING_TO_LETTERS_16(str, 0), 0 // guard for longer strings

// cannot use ScreenFactory::Screen because of cyclical dependency
// here is workaround
using ScreenUniquePtr = static_unique_ptr<screen_t>;
using ScreenCreator = static_unique_ptr<screen_t> (*)(); // function pointer definition
void open_screen(const ScreenCreator open_fn);

/**
 * @brief menu item template to open a screen via factory method
 * do not use directly use GENERATE_SCREEN_FN_ITEM or GENERATE_SCREEN_FN_ITEM_DEV
 *
 * @tparam HIDDEN hidden items are not transalted and have green color
 * @tparam OPEN_FN factory method
 * @tparam LETTERS string convertedt to letters by STRING_TO_LETTERS_STR
 */

template <is_hidden_t HIDDEN, const ScreenCreator OPEN_FN, const char... LETTERS>
class MI_SCREEN_FN : public IWindowMenuItem {
    static const char *make_Str() {
        static const char arr[] = { LETTERS... };
        return arr;
    }

public:
    MI_SCREEN_FN()
        : IWindowMenuItem(HIDDEN == is_hidden_t::dev ? string_view_utf8::MakeCPUFLASH((uint8_t *)make_Str()) : _(make_Str()), nullptr, is_enabled_t::yes, HIDDEN, expands_t::yes) {
    }

protected:
    virtual void click(IWindowMenu &window_menu) override {
        open_screen(OPEN_FN);
    }
};

/**
 * @brief menu item template to open a screen
 * do not use directly use GENERATE_SCREEN_ITEM or GENERATE_SCREEN_ITEM_DEV
 *
 * @tparam HIDDEN hidden items are not transalted and have green color
 * @tparam SCREEN screen type to open
 * @tparam LETTERS string convertedt to letters by STRING_TO_LETTERS_STR
 */
template <is_hidden_t HIDDEN, class SCREEN, const char... LETTERS>
class MI_SCREEN : public MI_SCREEN_FN<HIDDEN, ScreenFactory::Screen<SCREEN>, LETTERS...> {
};

#define GENERATE_SCREEN_ITEM(SCREEN, NAME) MI_SCREEN<is_hidden_t::no, SCREEN, STRING_TO_LETTERS_STR(NAME)>
#define GENERATE_SCREEN_FN_ITEM(FN, NAME)  MI_SCREEN_FN<is_hidden_t::no, FN, STRING_TO_LETTERS_STR(NAME)>

// dev version is green and does not translate texts
#define GENERATE_SCREEN_ITEM_DEV(SCREEN, NAME) MI_SCREEN<is_hidden_t::dev, SCREEN, STRING_TO_LETTERS_STR(NAME)>
#define GENERATE_SCREEN_FN_ITEM_DEV(FN, NAME)  MI_SCREEN_FN<is_hidden_t::dev, FN, STRING_TO_LETTERS_STR(NAME)>
