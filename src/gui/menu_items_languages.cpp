/**
 * @file menu_items_languages.cpp
 * @brief menu language items, must be compiled when languages are enabled
 * there is a second file menu_items_no_languages.cpp, it must be compiled when languages are disabled
 */

#include "menu_items_languages.hpp"
#include "translator.hpp"
#include "img_resources.hpp"
#include "screen_menu_languages.hpp"
#include "translation_provider_FILE.hpp"
#include "ScreenHandler.hpp"

/*****************************************************************************/
// MI_LANGUAGE
MI_LANGUAGE::MI_LANGUAGE()
    : WI_LABEL_t(_(label), &img::language_16x16, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_LANGUAGE::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuLanguages>);
}

/*****************************************************************************/
// MI_LANGUAGUE_USB
MI_LANGUAGUE_USB::MI_LANGUAGUE_USB()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_LANGUAGUE_USB::click([[maybe_unused]] IWindowMenu &windowMenu) {
    if (fileProviderUSB.EnsureFile()) {
        Translations::Instance().RegisterProvider(Translations::MakeLangCode("ts"), &fileProviderUSB);
    }
}

/*****************************************************************************/
// MI_LOAD_LANG
MI_LOAD_LANG::MI_LOAD_LANG()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_LOAD_LANG::click([[maybe_unused]] IWindowMenu &windowMenu) {
    const uint8_t buffLen = 16;

    uint8_t buff[buffLen];

    FILE *srcDir = fopen("/usb/lang/ts.mo", "rb");
    FILE *dstDir = fopen("/internal/ts.mo", "wb");
    // copy languague from usb to xflash
    if (dstDir && srcDir) {
        for (size_t readBytes = fread(buff, 1, buffLen, srcDir); readBytes != 0; readBytes = fread(buff, 1, buffLen, srcDir)) {
            fwrite(buff, 1, readBytes, dstDir);
        }
    }
    fclose(dstDir);
    fclose(srcDir);
}

/*****************************************************************************/
// MI_LANGUAGUE_XFLASH
MI_LANGUAGUE_XFLASH::MI_LANGUAGUE_XFLASH()
    : WI_LABEL_t(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev) {}

void MI_LANGUAGUE_XFLASH::click([[maybe_unused]] IWindowMenu &windowMenu) {
    if (fileProviderInternal.EnsureFile()) {
        Translations::Instance().RegisterProvider(Translations::MakeLangCode("ts"), &fileProviderInternal);
    }
}
