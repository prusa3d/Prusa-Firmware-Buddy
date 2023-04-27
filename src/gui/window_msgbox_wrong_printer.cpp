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
          { this, txt_wrong_gcode_level, valid_printer_settings.wrong_gcode_level.get_severity(), valid_printer_settings.wrong_gcode_level.is_valid() },
          { this, txt_compatibility_mode, valid_printer_settings.mk3_compatibility_mode.get_severity(), valid_printer_settings.mk3_compatibility_mode.is_valid() } } }
    , new_fw_message(this, txt_old_firmware, valid_printer_settings.wrong_firmware.get_severity(), valid_printer_settings.wrong_firmware.is_valid())
    , fw_version_text(this, {}, is_multiline::no) {
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

    // Show new firmware available
    if (new_fw_message.text.HasVisibleFlag()) {
        icon_rect += Rect16::Top_t(lineh);
        text_rect += Rect16::Top_t(lineh);
        new_fw_message.icon.SetRect(icon_rect);

        Rect16 fw_message_rect = text_rect;
        fw_message_rect.LimitSize({ static_cast<uint16_t>(fw_message_rect.Width() * 5 / 7), h }); // Shrink the row for text
        new_fw_message.text.SetRect(fw_message_rect);
        fw_version_text.SetRect(text_rect.RightSubrect(fw_message_rect)); // Remainder of line for fw version
        fw_version_text.SetAlignment(Align_t::CenterTop());

        // Print version string
        static char fw_version[] = "v00.00.000";
        snprintf(fw_version, std::size(fw_version), "v%d.%d.%d", valid_printer_settings.gcode_fw_version.major, valid_printer_settings.gcode_fw_version.minor, valid_printer_settings.gcode_fw_version.patch);
        fw_version_text.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(fw_version)));
    } else {
        fw_version_text.Hide();
    }
}
