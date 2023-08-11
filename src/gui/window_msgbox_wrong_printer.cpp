#include "window_msgbox_wrong_printer.hpp"

MsgBoxInvalidPrinter::Message::Message(window_t *parent, const char *text, HWCheckSeverity severity, bool valid)
    : icon(parent, {}, (severity == HWCheckSeverity::Abort) ? &img::nok_16x16 : &img::warning_16x16)
    , text(parent, {}, is_multiline::no, is_closed_on_click_t::no, _(text)) {
    if (valid) {
        icon.Hide();
        this->text.Hide();
    }
}

MsgBoxInvalidPrinter::MsgBoxInvalidPrinter(Rect16 rect, string_view_utf8 tit, const img::Resource *title_icon)
    : MsgBoxTitled(rect, Responses_NONE, 0, nullptr, _(txt_wrong_printer_title), is_multiline::no, tit, title_icon, is_closed_on_click_t::no)
    , valid_printer_settings(GCodeInfo::getInstance().get_valid_printer_settings())
    , messages { { { this, txt_wrong_tools, valid_printer_settings.wrong_tools.get_severity(), valid_printer_settings.wrong_tools.is_valid() },
          { this, txt_wrong_nozzle_diameter, valid_printer_settings.wrong_nozzle_diameter.get_severity(), valid_printer_settings.wrong_nozzle_diameter.is_valid() },
          { this, txt_wrong_printer_model, valid_printer_settings.wrong_printer_model.get_severity(), valid_printer_settings.wrong_printer_model.is_valid() },
          { this, txt_wrong_gcode_level, valid_printer_settings.wrong_gcode_level.get_severity(), valid_printer_settings.wrong_gcode_level.is_valid() },
          { this, txt_compatibility_mode, valid_printer_settings.mk3_compatibility_mode.get_severity(), valid_printer_settings.mk3_compatibility_mode.is_valid() } } }
    , wrong_fw_message(this, txt_old_firmware, valid_printer_settings.wrong_firmware.get_severity(), valid_printer_settings.wrong_firmware.is_valid())
    , wrong_fw_version_text(this, {}, is_multiline::no)
    , unsupported_features(this, txt_unsupported_features, HWCheckSeverity::Abort, !valid_printer_settings.unsupported_features)
    , unsupported_features_text(this, {}, is_multiline::no) {
    Rect16::Width_t icon_margin = GuiDefaults::InvalidPrinterIconMargin;
    Rect16::Height_t line_spacing = GuiDefaults::InvalidPrinterLineSpacing;

    Rect16::Width_t png_w = img::warning_16x16.w;
    Rect16::Height_t png_h = img::warning_16x16.h;

    Rect16::Height_t h = GuiDefaults::Font->h;
    Rect16::Height_t lineh = std::min(h, png_h) + line_spacing;

    Rect16 icon_rect = { getTextRect().TopLeft(), png_w, png_h };
    Rect16 text_rect = getTextRect() = Rect16::Height_t(h);

    text.SetRect(text_rect);

    // Make a separator empty line only if there is room for it
    auto lines = std::count_if(begin(messages), end(messages), [](auto &m) { return m.text.HasVisibleFlag(); }) + (wrong_fw_message.text.HasVisibleFlag() ? 1 : 0) + (unsupported_features.text.HasVisibleFlag() ? 2 : 0);
    if (lines <= 6) {
        icon_rect += Rect16::Top_t(lineh);
        text_rect += Rect16::Top_t(lineh);
    }

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
    if (wrong_fw_message.text.HasVisibleFlag()) {
        icon_rect += Rect16::Top_t(lineh);
        text_rect += Rect16::Top_t(lineh);
        wrong_fw_message.icon.SetRect(icon_rect);

        Rect16 fw_message_rect = text_rect;
        fw_message_rect.LimitSize({ static_cast<uint16_t>(fw_message_rect.Width() * 5 / 7), h }); // Shrink the row for text
        wrong_fw_message.text.SetRect(fw_message_rect);
        wrong_fw_version_text.SetRect(text_rect.RightSubrect(fw_message_rect));                   // Remainder of line for fw version
        wrong_fw_version_text.SetAlignment(Align_t::CenterTop());

        // Print version string
        static char fw_version[] = "v00.00.000";
        snprintf(fw_version, std::size(fw_version), "v%d.%d.%d", valid_printer_settings.gcode_fw_version.major, valid_printer_settings.gcode_fw_version.minor, valid_printer_settings.gcode_fw_version.patch);
        wrong_fw_version_text.SetText(string_view_utf8::MakeRAM(reinterpret_cast<uint8_t *>(fw_version)));
    } else {
        wrong_fw_version_text.Hide();
    }

    // Show unsupported features
    if (unsupported_features.text.HasVisibleFlag()) {
        icon_rect += Rect16::Top_t(lineh);
        text_rect += Rect16::Top_t(lineh);
        unsupported_features.icon.SetRect(icon_rect);
        unsupported_features.text.SetRect(text_rect);
        text_rect += Rect16::Top_t(lineh);
        text_rect += Rect16::Left_t(10);
        unsupported_features_text.SetText(string_view_utf8::MakeRAM((uint8_t *)valid_printer_settings.unsupported_features_text));
        unsupported_features_text.SetRect(text_rect);
    } else {
        unsupported_features_text.Hide();
    }
}
