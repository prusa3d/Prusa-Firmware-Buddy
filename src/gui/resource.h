//resource.h
#pragma once
#include <stdint.h>

struct PNG {
    // printer type depended PNGs
    static constexpr const char prusa_printer_logo[] = "/internal/res/png/prusa_printer_logo.png";
    static constexpr const char prusa_printer_splash[] = "/internal/res/png/prusa_printer_splash.png";

    // printer type independent PNGs
    static constexpr const char arrow_right_10x16[] = "/internal/res/png/arrow_right_10x16.png";

    static constexpr const char arrow_up_12x12[] = "/internal/res/png/arrow_up_12x12.png";
    static constexpr const char arrow_up_orange_12x12[] = "/internal/res/png/arrow_up_orange_12x12.png";
    static constexpr const char arrow_down_12x12[] = "/internal/res/png/arrow_down_12x12.png";
    static constexpr const char arrow_down_orange_12x12[] = "/internal/res/png/arrow_down_orange_12x12.png";

    static constexpr const char flag_cs_16x11[] = "/internal/res/png/flag_cs_16x11.png";
    static constexpr const char flag_de_16x11[] = "/internal/res/png/flag_de_16x11.png";
    static constexpr const char flag_en_16x11[] = "/internal/res/png/flag_en_16x11.png";
    static constexpr const char flag_es_16x11[] = "/internal/res/png/flag_es_16x11.png";
    static constexpr const char flag_fr_16x11[] = "/internal/res/png/flag_fr_16x11.png";
    static constexpr const char flag_it_16x11[] = "/internal/res/png/flag_it_16x11.png";
    static constexpr const char flag_pl_16x11[] = "/internal/res/png/flag_pl_16x11.png";

    static constexpr const char sheets_profile_16x14[] = "/internal/res/png/sheets_profile_16x14.png";

    static constexpr const char spinner0_16x16[] = "/internal/res/png/spinner0_16x16.png";
    static constexpr const char spinner1_16x16[] = "/internal/res/png/spinner1_16x16.png";
    static constexpr const char spinner2_16x16[] = "/internal/res/png/spinner2_16x16.png";
    static constexpr const char spinner3_16x16[] = "/internal/res/png/spinner3_16x16.png";
    static constexpr const char home_16x16[] = "/internal/res/png/home_shape_16px.png";
    static constexpr const char print_16x16[] = "/internal/res/png/print_16px.png";
    static constexpr const char info_16x16[] = "/internal/res/png/info_16px.png";
    static constexpr const char warning_16x16[] = "/internal/res/png/warning_16px.png";
    static constexpr const char question_16x16[] = "/internal/res/png/question_16px.png";
    static constexpr const char arrow_up_16x16[] = "/internal/res/png/arrow_up_16px.png";
    static constexpr const char arrow_down_16x16[] = "/internal/res/png/arrow_down_16px.png";
    static constexpr const char arrow_left_16x16[] = "/internal/res/png/arrow_left_16px.png";
    static constexpr const char wifi_16x16[] = "/internal/res/png/wifi_16x16.png";
    static constexpr const char lan_16x16[] = "/internal/res/png/lan_16x16.png";
    static constexpr const char home_full_16x16[] = "/internal/res/png/home_full_16x16.png";
    static constexpr const char folder_up_16x16[] = "/internal/res/png/folder_up_16x16.png";
    static constexpr const char error_16x16[] = "/internal/res/png/error_16x16.png";

    static constexpr const char dash_18x18[] = "/internal/res/png/dash_18x18.png";
    static constexpr const char ok_color_18x18[] = "/internal/res/png/ok_color_18x18.png";
    static constexpr const char nok_color_18x18[] = "/internal/res/png/nok_color_18x18.png";

    static constexpr const char hourglass_26x39[] = "/internal/res/png/hourglass_26x39.png";

    static constexpr const char usb_32x16[] = "/internal/res/png/usb_16px.png";

    static constexpr const char back_32x32[] = "/internal/res/png/back_32x32.png";

    static constexpr const char nozzle_34x32[] = "/internal/res/png/nozzle_34x32.png";

    static constexpr const char pepa_42x64[] = "/internal/res/png/pepa_64px.png";

    static constexpr const char question_48x48[] = "/internal/res/png/question_48x48.png";
    static constexpr const char warning_48x48[] = "/internal/res/png/warning_48px.png";
    static constexpr const char error_white_48x48[] = "/internal/res/png/error_white_48px.png";
    static constexpr const char info_48x48[] = "/internal/res/png/info_48px.png";
    static constexpr const char disconnect_48x48[] = "/internal/res/png/disconnect_48x48.png";
    static constexpr const char resume_48x48[] = "/internal/res/png/resume_48x48.png";
    static constexpr const char reprint_48x48[] = "/internal/res/png/reprint_48x48.png";
    static constexpr const char nozzle_48x48[] = "/internal/res/png/nozzle_shape_48px.png";
    static constexpr const char fan_error_48x48[] = "/internal/res/png/fan_error_48x48.png";
    static constexpr const char exposure_times_48x48[] = "/internal/res/png/exposure_times_48x48.png";
    static constexpr const char usb_error_48x48[] = "/internal/res/png/usb_error_48x48.png";

    static constexpr const char print_58x58[] = "/internal/res/png/print_58x58.png";
    static constexpr const char preheat_58x58[] = "/internal/res/png/preheat_58x58.png";
    static constexpr const char spool_58x58[] = "/internal/res/png/spool_58x58.png";
    static constexpr const char calibrate_58x58[] = "/internal/res/png/calibrate_58x58.png";
    static constexpr const char settings_58x58[] = "/internal/res/png/settings_58x58.png";
    static constexpr const char info_58x58[] = "/internal/res/png/info_58x58.png";
    static constexpr const char pause_58x58[] = "/internal/res/png/pause_58x58.png";
    static constexpr const char stop_58x58[] = "/internal/res/png/stop_58x58.png";
    static constexpr const char home_58x58[] = "/internal/res/png/home_58x58.png";

    static constexpr const char hand_qr_59x72[] = "/internal/res/png/hand_qr.png";

    static constexpr const char wifi_64x64[] = "/internal/res/png/wifi_64x64.png";

    static constexpr const char marlin_logo_76x61[] = "/internal/res/png/marlin_logo_76x61.png";

    static constexpr const char hand_154x65[] = "/internal/res/png/hand_154x65.png";

    static constexpr const char turn_knob_81x55[] = "/internal/res/png/turn_knob.png";

    static constexpr const char pepa_original_90x137[] = "/internal/res/png/pepa_original_140px.png";

    static constexpr const char pepa_92x140[] = "/internal/res/png/pepa_140px.png";

    static constexpr const char nozzle_crash_101x64[] = "/internal/res/png/nozzle_crash_101x64.png";

    static constexpr const char serial_printing_172x138[] = "/internal/res/png/serial_printing.png";
};

constexpr const char *IDR_PNG_nozzle_16px = "png/nozzle_16px";
constexpr const char *IDR_PNG_heatbed_16px = "png/heatbed_16px";
constexpr const char *IDR_PNG_speed_16px = "png/speed_16px";
constexpr const char *IDR_PNG_spool_16px = "png/spool_16px";
constexpr const char *IDR_PNG_z_axis_16px = "png/z_axis_16px";
constexpr const char *IDR_PNG_home_shape_16px = "png/home_shape_16px";
constexpr const char *IDR_PNG_print_16px = "png/print_16px";
constexpr const char *IDR_PNG_x_axis_16x16 = "png/x_axis_16x16";
constexpr const char *IDR_PNG_y_axis_16x16 = "png/y_axis_16x16";
constexpr const char *IDR_PNG_z_axis_16x16 = "png/z_axis_16x16";
constexpr const char *IDR_PNG_turbine_16x16 = "png/turbine_16x16";
constexpr const char *IDR_PNG_fan_16x16 = "png/fan_16x16";
constexpr const char *IDR_PNG_selftest_16x16 = "png/selftest_16x16";
constexpr const char *IDR_PNG_wizard_16x16 = "png/wizard_16x16";
constexpr const char *IDR_PNG_filament_sensor_17x16 = "png/filament_sensor_17x16";
constexpr const char *IDR_PNG_nozzle_empty_16px = "png/nozzle_empty_16px";
constexpr const char *IDR_PNG_folder_full_16px = "png/folder_full_16px";

namespace png {
struct Id {
    const char *ptr;
    static constexpr Id Null() { return { nullptr }; }
    constexpr bool IsNull() { return ptr == nullptr; }
    constexpr bool operator==(Id id) { return ptr == id.ptr; }
    constexpr bool operator!=(Id id) { return !((*this) == id); }
};
}

enum ResourceId : uint8_t {

    //null resource - test
    IDR_NULL = 0x0000,

    //fonts
    IDR_FNT_SMALL,
    IDR_FNT_NORMAL,
    IDR_FNT_BIG,
    IDR_FNT_TERMINAL,
    IDR_FNT_SPECIAL,
};
