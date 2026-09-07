#include "text_fit_test.hpp"

#include <catch2/catch_test_macros.hpp>
#include <lang/string_view_utf8.hpp>
#include "translator.hpp"
#include <translation_provider_FILE.hpp>
#include <Rect16.h>
#include <guiconfig/GuiDefaults.hpp>
#include <fonts.hpp>
#include <utils/enum_array.hpp>
#include <text_fit_enums.hpp>
#include <utils/string_builder.hpp>
#include <str_utils.hpp>

struct TextLayout {
    Rect16 title_rect;
    Rect16 desc_rect;
    Font title_font;
    Font desc_font;
};

constexpr std::array<const char *, 9> lang_codes = { "en", "cs", "de", "es", "fr", "it", "ja", "pl", "uk" };

static constexpr Font red_screen_font = GuiDefaults::EnableDialogBigLayout ? GuiDefaults::DefaultFont : Font::small;

static constexpr EnumArray<GuiLayout, TextLayout, 3> layouts {
    { GuiLayout::red_screen, TextLayout { GuiDefaults::RedscreenTitleRect, GuiDefaults::RedscreenDescriptionRect, red_screen_font, red_screen_font } },
    { GuiLayout::warning_dialog, TextLayout { GuiDefaults::RedscreenTitleRect, GuiDefaults::WarningDlgTextRect, GuiDefaults::DefaultFont, GuiDefaults::DefaultFont } },
    { GuiLayout::mmu_dialog, TextLayout { GuiDefaults::MMUNoticeTitleRect, GuiDefaults::MMUNoticeTextRect, GuiDefaults::FontBig, GuiDefaults::DefaultFont } },
};

namespace {
const FILETranslationProvider cs("MO/cs.mo");
ProviderRegistrator csReg("cs", &cs);
const FILETranslationProvider de("MO/de.mo");
ProviderRegistrator deReg("de", &de);
const FILETranslationProvider es("MO/es.mo");
ProviderRegistrator esReg("es", &es);
const FILETranslationProvider fr("MO/fr.mo");
ProviderRegistrator frReg("fr", &fr);
const FILETranslationProvider it("MO/it.mo");
ProviderRegistrator itReg("it", &it);
const FILETranslationProvider ja("MO/ja.mo");
ProviderRegistrator jaReg("ja", &ja);
const FILETranslationProvider pl("MO/pl.mo");
ProviderRegistrator plReg("pl", &pl);
const FILETranslationProvider uk("MO/uk.mo");
ProviderRegistrator ukReg("uk", &uk);
} // namespace

void test_error(const ErrorEntry &error) {
    auto *layout = &layouts[error.layout];

    auto *fnt_title = resource_font(layout->title_font);
    auto *fnt_desc = resource_font(layout->desc_font);

    for (const auto lang : lang_codes) {
        Translations::Instance().ChangeLanguage(Translations::MakeLangCode(lang));

        const string_view_utf8 title_str = Translations::Instance().CurrentProvider()->GetText(error.title);
        StringReaderUtf8 title_reader(title_str);
        const auto title_layout = RectTextLayout(title_reader, layout->title_rect.Width() / fnt_title->w, layout->title_rect.Height() / fnt_title->h, is_multiline::no);

        {
            INFO("Title");
            INFO("Lang = " << lang);
            INFO("Original Text = "
                << "\"" << error.title << "\"\n");
            ArrayStringBuilder<512> str_build; // In case of buffer too small - throws SIGABRT
            str_build.append_string_view(title_str);
            INFO("Translated Text = "
                << "\"" << str_build.str() << "\"\n");
            INFO("Max chars per line = " << layout->title_rect.Width() / fnt_title->w << "; Chars in text = " << title_str.computeNumUtf8Chars());

            CHECK(!title_layout.has_text_overflown());
        }

        const string_view_utf8 desc_str = Translations::Instance().CurrentProvider()->GetText(error.text);
        StringReaderUtf8 desc_reader(desc_str);
        const auto desc_layout = RectTextLayout(desc_reader, layout->desc_rect.Width() / fnt_desc->w, layout->desc_rect.Height() / fnt_desc->h, is_multiline::yes);

        {
            INFO("Description");
            INFO("Lang = " << lang);
            INFO("Original Text = "
                << "\"" << error.text << "\"\n");
            ArrayStringBuilder<500> str_build; // In case of buffer too small - throws SIGABRT
            str_build.append_string_view(desc_str);
            INFO("Translated Text = "
                << "\"" << str_build.str() << "\"\n");
            INFO("Max chars per line = " << layout->desc_rect.Width() / fnt_desc->w);
            INFO("How may rows can fit in the rectangle: " << layout->desc_rect.Height() / fnt_desc->h);
            INFO("Hint: It's too many rows or single word is longer than one row");

            CHECK(!desc_layout.has_text_overflown());
        }
    }
}
