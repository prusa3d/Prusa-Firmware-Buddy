/// @file
#include "gui_utils.hpp"

#include <str_utils.hpp>

std::optional<Font> auto_select_font(const AutoSelectFontArgs &args) {
    static constexpr std::array font_list {
#if HAS_LARGE_DISPLAY()
        Font::large,
#endif
            Font::big,
            Font::normal,
            Font::small
    };

    const Font *fnt = font_list.begin();

    // Skip larger fonts than largest
    while (*fnt != args.largest) {
        fnt++;

        if (fnt == font_list.end()) {
            return std::nullopt;
        }
    }

    while (true) {
        StringReaderUtf8 reader(args.text);
        const auto *res = resource_font(*fnt);
        const RectTextLayout layout(reader, args.rect.Width() / res->w, args.rect.Height() / res->h, args.multiline ? is_multiline::yes : is_multiline::no);

        if (!layout.has_text_overflown()) {
            return *fnt;
        }

        if (*fnt == args.smallest) {
            return std::nullopt;
        }

        fnt++;
        if (fnt == font_list.end()) {
            return std::nullopt;
        }
    }
}
