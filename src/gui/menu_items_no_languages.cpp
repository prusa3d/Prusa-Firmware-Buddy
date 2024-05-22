/**
 * @file menu_items_no_languages.cpp
 * @brief menu language items, must be compiled when languages are disabled
 * there is a second file menu_items_languages.cpp, it must be compiled when languages are enabled
 */

#include "menu_items_languages.hpp"
#include "img_resources.hpp"

/*****************************************************************************/
// MI_LANGUAGE
MI_LANGUAGE::MI_LANGUAGE()
    : IWindowMenuItem(_(label), &img::language_16x16, is_enabled_t::no, is_hidden_t::no, expands_t::yes) {
}

void MI_LANGUAGE::click(IWindowMenu & /*window_menu*/) {
}

/*****************************************************************************/
// MI_LANGUAGUE_USB
MI_LANGUAGUE_USB::MI_LANGUAGUE_USB()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::dev) {}

void MI_LANGUAGUE_USB::click([[maybe_unused]] IWindowMenu &windowMenu) {
}

/*****************************************************************************/
// MI_LOAD_LANG
MI_LOAD_LANG::MI_LOAD_LANG()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::dev) {}

void MI_LOAD_LANG::click([[maybe_unused]] IWindowMenu &windowMenu) {
}

/*****************************************************************************/
// MI_LANGUAGUE_XFLASH
MI_LANGUAGUE_XFLASH::MI_LANGUAGUE_XFLASH()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::no, is_hidden_t::dev) {}

void MI_LANGUAGUE_XFLASH::click([[maybe_unused]] IWindowMenu &windowMenu) {
}
