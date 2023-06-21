#pragma once
#include "gui.hpp"
#include "gcode_info.hpp"
#include "png_resources.hpp"

class MsgBoxInvalidPrinter : public MsgBoxTitled {
    static constexpr const char *txt_wrong_printer_title = N_("The G-code isn't fully compatible");

    static constexpr const char *txt_wrong_tools = N_("printer doesn't have enough tools");
    static constexpr const char *txt_wrong_nozzle_diameter = N_("nozzle diameter doesn't match");
    static constexpr const char *txt_wrong_printer_model = N_("printer model doesn't match");
    static constexpr const char *txt_compatibility_mode = N_("it will run in MK3-compatibility mode");
    static constexpr const char *txt_wrong_gcode_level = N_("G-code version doesn't match");
    static constexpr const char *txt_old_firmware = N_("newer firmware is required");
    static constexpr const char *txt_unsupported_features = N_("following features are required:");

    struct Message {
        Message(window_t *parent, const char *text, HWCheckSeverity severity, bool valid);

        window_icon_t icon;
        window_text_t text;
    };
    const GCodeInfo::ValidPrinterSettings &valid_printer_settings;
    std::array<Message, 5> messages;
    Message wrong_fw_message;            ///< Message with new firmware
    window_text_t wrong_fw_version_text; ///< Display which firmware version
    Message unsupported_features;
    window_text_t unsupported_features_text;

public:
    MsgBoxInvalidPrinter(Rect16 rect, string_view_utf8 tit, const png::Resource *title_icon_res);
};
