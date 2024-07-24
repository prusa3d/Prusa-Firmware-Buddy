#include "MItem_hardware.hpp"
#include "ScreenHandler.hpp"
#include "WindowMenuSpin.hpp"
#include <option/has_toolchanger.h>
#include <option/has_side_fsensor.h>
#include <PersistentStorage.h>
#include <common/nozzle_diameter.hpp>
#include <screen_menu_hardware_checks.hpp>
#include <common/printer_model_data.hpp>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
    #if HAS_SIDE_FSENSOR()
        #include <filament_sensors_handler_XL_remap.hpp>
    #endif /*HAS_SIDE_FSENSOR()*/
#endif /*HAS_TOOLCHANGER()*/

MI_HARDWARE_CHECK::MI_HARDWARE_CHECK(HWCheckType check_type)
    : WI_SWITCH_t(static_cast<int>(config_store().visit_hw_check(check_type, [](auto &item) { return item.get(); })), _(hw_check_type_names[check_type]), nullptr, is_enabled_t::yes, is_hidden_t::no,
        _("None"), _("Warn"), _("Strict"))
    , check_type(check_type) //
{}

void MI_HARDWARE_CHECK::OnChange([[maybe_unused]] size_t old_index) {
    config_store().visit_hw_check(check_type, [set = static_cast<HWCheckSeverity>(index)](auto &item) { item.set(set); });
}

MI_HARDWARE_G_CODE_CHECKS::MI_HARDWARE_G_CODE_CHECKS()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_HARDWARE_G_CODE_CHECKS::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenMenuHardwareChecks>);
}

#if HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()
// MI_SIDE_FSENSOR_REMAP
MI_SIDE_FSENSOR_REMAP::MI_SIDE_FSENSOR_REMAP()
    : WI_ICON_SWITCH_OFF_ON_t(side_fsensor_remap::is_remapped(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {};

void MI_SIDE_FSENSOR_REMAP::OnChange([[maybe_unused]] size_t old_index) {
    if (uint8_t mask = side_fsensor_remap::ask_to_remap(); mask != 0) { // Ask user to remap
        Screens::Access()->Get()->Validate(); // Do not redraw this menu yet

        // Change index by what user selected)
        set_value(side_fsensor_remap::is_remapped(), false);

        Validate(); // Do not redraw this switch yet
        marlin_client::test_start_with_data(stmFSensor, static_cast<ToolMask>(mask)); // Start filament sensor calibration for moved tools

    } else {
        // Change index by what user selected)
        set_value(side_fsensor_remap::is_remapped(), false);
    }
}
#endif /*HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()*/

#if HAS_EXTENDED_PRINTER_TYPE()
static constexpr auto extended_printer_type_names = stdext::map_array(extended_printer_type_model, [](PrinterModel model) {
    return PrinterModelInfo::get_constexpr(model).id_str;
});

MI_EXTENDED_PRINTER_TYPE::MI_EXTENDED_PRINTER_TYPE()
    : WiStoreEnumSwitch(_("Printer Type"), extended_printer_type_names, false) {
}

void MI_EXTENDED_PRINTER_TYPE::OnChange(size_t old_index) {
    WiStoreEnumSwitch::OnChange(old_index);

    #if EXTENDED_PRINTER_TYPE_DETERMINES_MOTOR_STEPS()
    // Reset motor configuration if the printer types have different motors
    if (extended_printer_type_has_400step_motors[old_index] != extended_printer_type_has_400step_motors[index]) {
        // This code is copied over from MI_MK4_MK39
        // This line looks absolutely terrible, I agree. It apparently just clears homing data.
        PersistentStorage::erase();

        {
            auto &store = config_store();
            auto transaction = store.get_backend().transaction_guard();
            store.homing_sens_x.set(store.homing_sens_x.default_val);
            store.homing_sens_y.set(store.homing_sens_y.default_val);
            store.homing_bump_divisor_x.set(store.homing_bump_divisor_x.default_val);
            store.homing_bump_divisor_y.set(store.homing_bump_divisor_y.default_val);
        }

        // Reset XY homing sensitivity
        marlin_client::gcode("M914 X Y");

        // XY motor currents
        marlin_client::gcode_printf("M906 X%u Y%u", get_rms_current_ma_x(), get_rms_current_ma_y());

        // XY motor microsteps
        marlin_client::gcode_printf("M350 X%u Y%u", get_microsteps_x(), get_microsteps_y());
    }
    #endif
}
#endif
