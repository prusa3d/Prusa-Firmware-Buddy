#pragma once
#include "gui.hpp"
#include "gcode_info.hpp"
#include "png_resources.hpp"

class MsgBoxInvalidPrinter : public MsgBoxTitled {
    static constexpr const char *txt_wrong_printer_title = N_("This G-code was set up for another HW.");

    static constexpr const char *txt_wrong_nozzle_diameter = N_("wrong nozzle diameter");
    static constexpr const char *txt_wrong_printer_model = N_("wrong printer model");
    static constexpr const char *txt_wrong_firmware_version = N_("wrong firmware version");
    static constexpr const char *txt_wrong_gcode_level = N_("wrong gcode level");

    struct Message {
        Message(window_t *parent, const char *text, GCodeInfo::ValidPrinterSettings::Severity severity, bool valid);

        window_icon_t icon;
        window_text_t text;
    };
    GCodeInfo::ValidPrinterSettings valid_printer_settings;
    std::array<Message, 4> messages;

public:
    MsgBoxInvalidPrinter(Rect16 rect, string_view_utf8 tit, const png::Resource *title_icon_res);
};
