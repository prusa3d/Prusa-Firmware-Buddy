#pragma once
#include "gcode_info.hpp"
#include "window_msgbox.hpp"
#include <guiconfig/guiconfig.h>
#include <find_error.hpp>

class MsgBoxInvalidPrinter : public MsgBoxTitled {
    static constexpr const char *txt_wrong_printer_title = find_error(ErrCode::CONNECT_PRINT_PREVIEW_WRONG_PRINTER).err_text;

    static constexpr const char *txt_wrong_tools = N_("printer doesn't have enough tools");
    static constexpr const char *txt_wrong_nozzle_diameter = N_("nozzle diameter doesn't match");
    static constexpr const char *txt_wrong_printer_model = N_("printer model doesn't match");
    static constexpr const char *txt_gcode_compatibility_mode = N_("it will run in MK3-compatibility mode");
    static constexpr const char *txt_fan_compatibility_mode = N_("fan speed will be reduced");
    static constexpr const char *txt_wrong_gcode_level = N_("G-code version doesn't match");
#if HAS_LARGE_DISPLAY()
    static constexpr const char txt_old_firmware[] = N_("newer firmware is required: %s");
    static constexpr const char *txt_unsupported_features = N_("following features are required:");
#elif HAS_MINI_DISPLAY()
    static constexpr const char *txt_old_firmware = N_("Newer FW req.: %s");
    static constexpr const char *txt_unsupported_features = N_("Features required:");
#endif

    struct Message {
        Message(window_t *parent, const string_view_utf8 &text, HWCheckSeverity severity, bool valid);
        Message(window_t *parent, const string_view_utf8 &text, const GCodeInfo::ValidPrinterSettings::Feature &feature);

        window_icon_t icon;
        window_text_t text;
    };
    const GCodeInfo::ValidPrinterSettings &valid_printer_settings;
    std::array<Message, hw_check_type_count + 1> messages;

    StringViewUtf8Parameters<sizeof(valid_printer_settings.latest_fw_version) + 5> wrong_fw_version_params; // Max version len + some margin

    Message unsupported_features;
    window_text_t unsupported_features_text;

public:
    MsgBoxInvalidPrinter(Rect16 rect, const string_view_utf8 &tit, const img::Resource *title_icon_res);
};
