#pragma once
#include "gui.hpp"
#include "gcode_info.hpp"
#include "img_resources.hpp"
#include <guiconfig/guiconfig.h>
#include <find_error.hpp>

class MsgBoxInvalidPrinter : public MsgBoxTitled {
    static constexpr const char *txt_wrong_printer_title = find_error(ErrCode::CONNECT_PRINT_PREVIEW_WRONG_PRINTER).err_text;

    static constexpr const char *txt_wrong_tools = N_("printer doesn't have enough tools");
    static constexpr const char *txt_wrong_nozzle_diameter = N_("nozzle diameter doesn't match");
    static constexpr const char *txt_wrong_printer_model = N_("printer model doesn't match");
    static constexpr const char *txt_compatibility_mode = N_("it will run in MK3-compatibility mode");
    static constexpr const char *txt_wrong_gcode_level = N_("G-code version doesn't match");
#if defined(USE_ILI9488)
    static constexpr const char txt_old_firmware[] = N_("newer firmware is required: %s");
    static constexpr const char *txt_unsupported_features = N_("following features are required:");
#elif defined(USE_ST7789)
    static constexpr const char *txt_old_firmware = N_("Newer FW req.: %s");
    static constexpr const char *txt_unsupported_features = N_("Features required:");
#endif

    struct Message {
        Message(window_t *parent, string_view_utf8 text, HWCheckSeverity severity, bool valid);

        window_icon_t icon;
        window_text_t text;
    };
    const GCodeInfo::ValidPrinterSettings &valid_printer_settings;
    std::array<Message, 6> messages;
    char wrong_fw_version_buff[sizeof(txt_old_firmware) + // Original text
        sizeof(valid_printer_settings.latest_fw_version) + // Max version len
        20]; // Some margin for long translation

    Message unsupported_features;
    window_text_t unsupported_features_text;

public:
    MsgBoxInvalidPrinter(Rect16 rect, string_view_utf8 tit, const img::Resource *title_icon_res);
};
