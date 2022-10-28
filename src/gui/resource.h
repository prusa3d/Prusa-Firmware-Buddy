//resource.h
#pragma once
#include <stdint.h>

#include "png_resources.hpp"

struct PNG {
    static constexpr const char hand_154x65[] = "/internal/res/png/hand_154x65.png";
    static constexpr const char nozzle_34x32[] = "/internal/res/png/nozzle_34x32.png";
    static constexpr const char usb_32x16[] = "/internal/res/png/usb_16px.png";
    static constexpr const char info_16x16[] = "/internal/res/png/info_16px.png";
    static constexpr const char warning_16x16[] = "/internal/res/png/warning_16px.png";
    static constexpr const char question_16x16[] = "/internal/res/png/question_16px.png";
    static constexpr const char nozzle_48x48[] = "/internal/res/png/nozzle_shape_48px.png";
    static constexpr const char arrow_up_16x16[] = "/internal/res/png/arrow_up_16px.png";
    static constexpr const char arrow_down_16x16[] = "/internal/res/png/arrow_down_16px.png";
    static constexpr const char arrow_left_16x16[] = "/internal/res/png/arrow_left_16px.png";
    static constexpr const char hand_qr_59x72[] = "/internal/res/png/hand_qr.png";
    static constexpr const char turn_knob_81x55[] = "/internal/res/png/turn_knob.png";
    static constexpr const char pepa_42x64[] = "/internal/res/png/pepa_64px.png";
    static constexpr const char pepa_92x140[] = "/internal/res/png/pepa_140px.png";
    static constexpr const char pepa_original_90x137[] = "/internal/res/png/pepa_original_140px.png";
    static constexpr const char serial_printing_172x138[] = "/internal/res/png/png_serial_printing.png";
    static constexpr const char question_48x48[] = "/internal/res/png/question_48px.png";
    static constexpr const char warning_48x48[] = "/internal/res/png/warning_48px.png";
    static constexpr const char error_white_48x48[] = "/internal/res/png/error_white_48px.png";
    static constexpr const char info_48x48[] = "/internal/res/png/info_48px.png";
};

enum ResourceId : uint8_t {

    //null resource - test
    IDR_NULL = 0x0000,

    //fonts
    IDR_FNT_SMALL,
    IDR_FNT_NORMAL,
    IDR_FNT_BIG,
    IDR_FNT_TERMINAL,
    IDR_FNT_SPECIAL,

    //pngs
    IDR_PNG_marlin_logo,
    IDR_PNG_prusa_printer_splash,

    IDR_PNG_nozzle_16px,
    IDR_PNG_heatbed_16px,
    IDR_PNG_speed_16px,
    IDR_PNG_spool_16px,
    IDR_PNG_z_axis_16px,
    IDR_PNG_x_axis_16x16,
    IDR_PNG_y_axis_16x16,
    IDR_PNG_z_axis_16x16,
    IDR_PNG_turbine_16x16,
    IDR_PNG_fan_16x16,
    IDR_PNG_selftest_16x16,
    IDR_PNG_wizard_16x16,
    IDR_PNG_filament_sensor_17x16,

    IDR_PNG_print_58px,
    IDR_PNG_preheat_58px,
    IDR_PNG_spool_58px,
    IDR_PNG_reprint_48px,
    IDR_PNG_calibrate_58px,
    IDR_PNG_settings_58px,
    IDR_PNG_info_58px,
    IDR_PNG_pause_58px,
    IDR_PNG_stop_58px,
    IDR_PNG_resume_48px,
    IDR_PNG_home_58px,

    IDR_PNG_home_full_16px,
    IDR_PNG_folder_full_16px,
    IDR_PNG_folder_up_16px,
    IDR_PNG_folder_shape_16px,
    IDR_PNG_folder_open_16px,

    IDR_PNG_wifi_large_64x64,

    IDR_PNG_dash_18px,
    IDR_PNG_ok_color_18px,
    IDR_PNG_nok_color_18px,
    IDR_PNG_spinner1_16px,
    IDR_PNG_spinner2_16px,
    IDR_PNG_spinner3_16px,
    IDR_PNG_spinner4_16px,
    IDR_PNG_hourglass_39px,

    IDR_PNG_lan_16px,
    IDR_PNG_wifi_16px,
    IDR_PNG_error_16px,
    IDR_PNG_sheets_profile_16px,
    IDR_PNG_nozzle_crash,
    IDR_PNG_nozzle_empty_16px,

    IDR_PNG_arrow_right_16px,
    IDR_PNG_arrow_up_8px,
    IDR_PNG_arrow_down_8px,
    IDR_PNG_arrow_up_orange_8px,
    IDR_PNG_arrow_down_orange_8px,

    IDR_PNG_back_32px,
    IDR_PNG_disconnect_48px,
    IDR_PNG_switch_off_36px,
    IDR_PNG_switch_on_36px,
    IDR_PNG_exposure_times_48px,
    IDR_PNG_usb_error,
    IDR_PNG_fan_error,

    IDR_PNG_flag_cs,
    IDR_PNG_flag_de,
    IDR_PNG_flag_en,
    IDR_PNG_flag_es,
    IDR_PNG_flag_fr,
    IDR_PNG_flag_it,
    IDR_PNG_flag_pl,
};
