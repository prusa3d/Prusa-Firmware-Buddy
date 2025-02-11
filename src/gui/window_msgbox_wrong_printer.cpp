#include "window_msgbox_wrong_printer.hpp"
#include "img_resources.hpp"

#include <guiconfig/guiconfig.h>

MsgBoxInvalidPrinter::Message::Message(window_t *parent, const string_view_utf8 &text, HWCheckSeverity severity, bool valid)
    : icon(parent, {}, (severity == HWCheckSeverity::Abort) ? &img::nok_16x16 : &img::warning_16x16)
    , text(parent, {}, is_multiline::yes, is_closed_on_click_t::no, text) {
    if (valid) {
        icon.Hide();
        this->text.Hide();
    }
}

MsgBoxInvalidPrinter::Message::Message(window_t *parent, const string_view_utf8 &text, const GCodeInfo::ValidPrinterSettings::Feature &feature)
    : Message(parent, text, feature.get_severity(), feature.is_valid()) {}

MsgBoxInvalidPrinter::MsgBoxInvalidPrinter(Rect16 rect, const string_view_utf8 &tit, const img::Resource *title_icon)
    : MsgBoxTitled(rect, Responses_NONE, 0, nullptr, _(find_error(ErrCode::CONNECT_PRINT_PREVIEW_WRONG_PRINTER).err_text), is_multiline::yes, tit, title_icon, is_closed_on_click_t::no)
    , valid_printer_settings(GCodeInfo::getInstance().get_valid_printer_settings())
    , messages({
        { this, _("Printer doesn't have enough tools"), valid_printer_settings.wrong_tools },
            { this, _("Nozzle diameter doesn't match"), valid_printer_settings.wrong_nozzle_diameter },
            { this, _("Nozzle is not hardened"), valid_printer_settings.nozzle_not_hardened },
            { this, _("Nozzle is not high-flow"), valid_printer_settings.nozzle_not_high_flow },
            { this, _("Printer model doesn't match"), valid_printer_settings.wrong_printer_model },
            { this, _("G-code version doesn't match"), valid_printer_settings.wrong_gcode_level },
#if ENABLED(GCODE_COMPATIBILITY_MK3)
            { this, _("MK3 compatibility mode"), valid_printer_settings.gcode_compatibility_mode },
#endif
#if ENABLED(FAN_COMPATIBILITY_MK4_MK3)
            { this, _("Fan speeds will be adjusted"), valid_printer_settings.fan_compatibility_mode },
#endif
            { this,
                (HAS_LARGE_DISPLAY() ? _("Newer firmware is required: %s") : _("Newer FW req.: %s"))
                    .formatted(wrong_fw_version_params, valid_printer_settings.latest_fw_version),
                valid_printer_settings.wrong_firmware },
#if HAS_MMU2()
            { this, _("Nozzle flow rate doesn't match"), valid_printer_settings.nozzle_flow_mismatch },
#endif
    })
    , unsupported_features(this,
          (HAS_LARGE_DISPLAY() ? _("Following features are required:") : _("Features required:")),
          HWCheckSeverity::Abort, !valid_printer_settings.unsupported_features)
    , unsupported_features_text(this, {}, is_multiline::no) {

    static constexpr const Rect16::Width_t icon_margin = GuiDefaults::InvalidPrinterIconMargin;
    static constexpr const Rect16::Height_t line_spacing = GuiDefaults::InvalidPrinterLineSpacing;
    static constexpr const Rect16::Width_t img_w = img::warning_16x16.w;
    static constexpr const Rect16::Height_t img_h = img::warning_16x16.h;

    Rect16::Height_t h = height(GuiDefaults::DefaultFont);

    Rect16 icon_rect = { getTextRect().TopLeft(), img_w, img_h };

#if HAS_MINI_DISPLAY()
    Rect16::Height_t item_h = (std::min(h, img_h) + line_spacing) * 2;
    Rect16 text_rect = getTextRect() = Rect16::Height_t(2 * item_h);
#elif HAS_LARGE_DISPLAY()
    Rect16::Height_t item_h = std::min(h, img_h) + line_spacing;
    Rect16 text_rect = getTextRect() = Rect16::Height_t(h);
#endif
    text.SetRect(text_rect);

#if HAS_LARGE_DISPLAY()
    // Make a separator empty line only if there is room for it
    auto lines = std::count_if(begin(messages), end(messages), [](auto &m) { return m.text.HasVisibleFlag(); }) + (unsupported_features.text.HasVisibleFlag() ? 2 : 0);
    if (lines <= 6) {
        icon_rect += Rect16::Top_t(item_h);
        text_rect += Rect16::Top_t(item_h);
    }
#endif
    text_rect += Rect16::Left_t(img_w + icon_margin);
    text_rect -= Rect16::Width_t(img_w + icon_margin);

    for (auto &m : messages) {
        if (m.text.HasVisibleFlag()) {
            icon_rect += Rect16::Top_t(item_h);
            text_rect += Rect16::Top_t(item_h);
            m.icon.SetRect(icon_rect);
            m.text.SetRect(text_rect);
        }
    }

    // Show unsupported features
    if (unsupported_features.text.HasVisibleFlag()) {
        icon_rect += Rect16::Top_t(item_h);
        text_rect += Rect16::Top_t(item_h);
        unsupported_features.icon.SetRect(icon_rect);
        unsupported_features.text.SetRect(text_rect);
        text_rect += Rect16::Top_t(item_h);
        text_rect += Rect16::Left_t(10);
        unsupported_features_text.SetText(string_view_utf8::MakeRAM((uint8_t *)valid_printer_settings.unsupported_features_text));
        unsupported_features_text.SetRect(text_rect);
    } else {
        unsupported_features_text.Hide();
    }
}
