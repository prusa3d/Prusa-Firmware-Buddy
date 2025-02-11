#pragma once
#include "gcode_info.hpp"
#include "window_msgbox.hpp"
#include <guiconfig/guiconfig.h>
#include <find_error.hpp>

class MsgBoxInvalidPrinter : public MsgBoxTitled {
    struct Message {
        Message(window_t *parent, const string_view_utf8 &text, HWCheckSeverity severity, bool valid);
        Message(window_t *parent, const string_view_utf8 &text, const GCodeInfo::ValidPrinterSettings::Feature &feature);

        window_icon_t icon;
        window_text_t text;
    };
    const GCodeInfo::ValidPrinterSettings &valid_printer_settings;

    // Must be before messages!
    /// Max version len + some margin
    StringViewUtf8Parameters<sizeof(valid_printer_settings.latest_fw_version) + 5> wrong_fw_version_params;

    std::array<Message, hw_check_type_count + 3 + (HAS_MMU2() ? 1 : 0)> messages;

    Message unsupported_features;
    window_text_t unsupported_features_text;

public:
    MsgBoxInvalidPrinter(Rect16 rect, const string_view_utf8 &tit, const img::Resource *title_icon_res);
};
