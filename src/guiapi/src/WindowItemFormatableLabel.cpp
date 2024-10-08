#include <guiapi/include/WindowItemFormatableLabel.hpp>

WI_LAMBDA_LABEL_t::WI_LAMBDA_LABEL_t(const string_view_utf8 &label, const img::Resource *icon, is_enabled_t enabled, is_hidden_t hidden, const PrintFunction &printAs)
    : IWindowMenuItem(label, icon ? icon_width : GuiDefaults::infoDefaultLen * width(InfoFont), icon, enabled, hidden)
    , printAs(printAs) {}

void WI_LAMBDA_LABEL_t::printExtension(Rect16 extension_rect, [[maybe_unused]] Color color_text, Color color_back, [[maybe_unused]] ropfn raster_op) const {
    std::array<char, GuiDefaults::infoDefaultLen> text;
    printAs(text);
    render_text_align(extension_rect, string_view_utf8::MakeRAM(text.data()), InfoFont, color_back,
        (IsFocused() && IsEnabled()) ? COLOR_DARK_GRAY : COLOR_SILVER, GuiDefaults::MenuPaddingItems, Align_t::RightCenter(), false);
}
