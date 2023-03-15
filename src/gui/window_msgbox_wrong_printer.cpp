#include "window_msgbox_wrong_printer.hpp"

MsgBoxInvalidPrinter::Message::Message(window_t *parent, const char *text, GCodeInfo::ValidPrinterSettings::Severity severity, bool valid)
    : icon(parent, {}, (severity == GCodeInfo::ValidPrinterSettings::Severity::Abort) ? &png::nok_16x16 : &png::warning_16x16)
    , text(parent, {}, is_multiline::no, is_closed_on_click_t::no, _(text)) {
    if (valid) {
        icon.Hide();
        this->text.Hide();
    }
}

MsgBoxInvalidPrinter::MsgBoxInvalidPrinter(Rect16 rect, string_view_utf8 tit, const png::Resource *title_icon)
    : MsgBoxTitled(rect, Responses_NONE, 0, nullptr, _(txt_wrong_printer_title), is_multiline::no, tit, title_icon, is_closed_on_click_t::no)
    , valid_printer_settings(GCodeInfo::getInstance().valid_printer_settings)
    , messages { { { this, txt_wrong_nozzle_diameter, valid_printer_settings.wrong_nozzle_diameters[0].get_severity(), valid_printer_settings.nozzle_diameters_valid() },
          { this, txt_wrong_printer_model, valid_printer_settings.wrong_printer_model.get_severity(), valid_printer_settings.wrong_printer_model.is_valid() },
          { this, txt_wrong_firmware_version, valid_printer_settings.wrong_firmware_version.get_severity(), valid_printer_settings.wrong_firmware_version.is_valid() },
          { this, txt_wrong_gcode_level, valid_printer_settings.wrong_gcode_level.get_severity(), valid_printer_settings.wrong_gcode_level.is_valid() } } } {
    Rect16::Width_t icon_margin = GuiDefaults::InvalidPrinterIconMargin;
    Rect16::Height_t line_spacing = GuiDefaults::InvalidPrinterLineSpacing;

    Rect16::Width_t png_w = png::warning_16x16.w;
    Rect16::Height_t png_h = png::warning_16x16.h;

    Rect16::Height_t h = GuiDefaults::Font->h;
    Rect16::Height_t lineh = std::min(h, png_h) + line_spacing;

    Rect16 icon_rect = { getTextRect().TopLeft(), png_w, png_h };
    Rect16 text_rect = getTextRect() = Rect16::Height_t(h);

    text.SetRect(text_rect);
    icon_rect += Rect16::Top_t(lineh);
    text_rect += Rect16::Top_t(lineh);

    text_rect += Rect16::Left_t(png_w + icon_margin);
    text_rect -= Rect16::Width_t(png_w + icon_margin);

    for (auto &m : messages) {
        if (m.text.HasVisibleFlag()) {
            icon_rect += Rect16::Top_t(lineh);
            text_rect += Rect16::Top_t(lineh);
            m.icon.SetRect(icon_rect);
            m.text.SetRect(text_rect);
        }
    }
}
