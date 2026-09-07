/// @file
#include "screen_menu_fan_info.hpp"

#include <common/printer_model.hpp>
#include <fanctl.hpp>
#include <img_resources.hpp>

#if HAS_XL_CAN()
    #include <puppies/xl_can.hpp>
#endif

MI_INFO_PRINT_FAN::MI_INFO_PRINT_FAN()
    : WI_FAN_LABEL_t(_("Print Fan"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = marlin_vars().print_fan_speed,
                       .rpm = marlin_vars().active_hotend().print_fan_rpm,
                   }; } //
    ) {}

MI_INFO_HBR_FAN::MI_INFO_HBR_FAN()
    : WI_FAN_LABEL_t(PRINTER_IS_PRUSA_MK3_5() ? _("Hotend Fan") : _("Heatbreak Fan"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = static_cast<uint8_t>(sensor_data().heatbreak_fan_pwm.load()),
                       .rpm = marlin_vars().active_hotend().heatbreak_fan_rpm,
                   }; } //
    ) {}

#if HAS_BED_FAN()
// translation: menu item showing info about heated bed fan, %d is the fan number
static constexpr const char *bed_fan_label_template = N_("Bed Fan %d");

MI_INFO_BED_FAN1::MI_INFO_BED_FAN1()
    : WI_FAN_LABEL_t {
        string_view_utf8 {},
        [](auto) { return FanPWMAndRPM {
                       .pwm = sensor_data().bed_fan1_pwm.load(),
                       .rpm = sensor_data().bed_fan1_rpm.load(),
                   }; },
    } {
    SetLabel(_(bed_fan_label_template).formatted(label_params_, 1));
}

MI_INFO_BED_FAN2::MI_INFO_BED_FAN2()
    : WI_FAN_LABEL_t {
        string_view_utf8 {},
        [](auto) { return FanPWMAndRPM {
                       .pwm = sensor_data().bed_fan2_pwm.load(),
                       .rpm = sensor_data().bed_fan2_rpm.load(),
                   }; },
    } {
    SetLabel(_(bed_fan_label_template).formatted(label_params_, 2));
}
#endif

#if HAS_PSU_FAN()
MI_INFO_PSU_FAN::MI_INFO_PSU_FAN()
    : WI_FAN_LABEL_t {
        // translation: menu item showing info about power supply unit cooling fan
        _("PSU Fan"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = sensor_data().psu_fan_pwm.load(),
                       .rpm = sensor_data().psu_fan_rpm.load(),
                   }; },
    } {}
#endif

#if HAS_CPU_FAN()
MI_INFO_CPU_FAN::MI_INFO_CPU_FAN()
    // translation: menu item showing info about the XLS main-board CPU cooling fan
    : WI_FAN_LABEL_t(_("CPU Fan"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = Fans::cpu().get_pwm(),
                       .rpm = Fans::cpu().get_actual_rpm(),
                   }; } //
    ) {
    // The CPU fan is physically present only on XLS; hide on plain XL.
    if (PrinterModelInfo::current().model != PrinterModel::xls) {
        set_is_hidden();
    }
}
#endif

#if HAS_XL_CAN()
MI_INFO_BED_MCU_FAN::MI_INFO_BED_MCU_FAN()
    // translation: menu item showing info about the XLS Modular Bed cooling fan
    : WI_FAN_LABEL_t(_("Bed MCU Fan"),
        [](auto) { return FanPWMAndRPM {
                       .pwm = buddy::puppies::xl_can.get_fan_pwm(),
                       .rpm = buddy::puppies::xl_can.get_fan_rpm(),
                   }; } //
    ) {
    // Present only on XLS; hide on plain XL (shared image).
    if (PrinterModelInfo::current().model != PrinterModel::xls) {
        set_is_hidden();
    }
}
#endif

ScreenMenuFanInfo::ScreenMenuFanInfo()
    : ScreenMenuFanInfo_ {
        // translation: Header text of the screen showing information about fans.
        _("FAN INFO"),
        &img::info_16x16,
    } {}
