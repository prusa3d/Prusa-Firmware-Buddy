#include <common/extended_printer_type.hpp>

#include <config_store/store_instance.hpp>
#include <option/has_print_fan_type.h>
#include <option/has_toolchanger.h>
#include <tool_index.hpp>
#include <marlin_server.hpp>
#include <marlin_client.hpp>
#include <config_store/store_c_api.h>
#include <utils/algorithm_extensions.hpp>
#include <option/has_nozzle_cleaner_lite.h>
#include <option/has_15gt_belts.h>

#if HAS_PRINT_FAN_TYPE()
    #include <print_fan_type.hpp>
#endif

#if HAS_TOOLCHANGER()
    #include <puppies/Dwarf.hpp>
#endif

#if HAS_EXTENDED_PRINTER_TYPE()

void change_extended_printer_type(PrinterModel new_model, [[maybe_unused]] ChangeExtendedPrinterTypeMode mode) {
    auto &store = config_store();

    [[maybe_unused]] const auto old_index = store.extended_printer_type.get();
    const auto new_index = stdext::index_of(extended_printer_type_model, new_model);
    if (new_index == extended_printer_type_model.size()) {
        bsod_unreachable();
    }

    [[maybe_unused]] const auto gcode = [](const char *fmt, auto... args) {
        if (marlin_server::is_marlin_server_thread()) {
            marlin_server::enqueue_gcode_printf(fmt, args...);
        } else {
            marlin_client::gcode_printf(fmt, args...);
        }
    };

    store.extended_printer_type.set(new_index);

    #if PRINTER_IS_PRUSA_XL()
    static_assert(extended_printer_type_model == std::array { PrinterModel::xl, PrinterModel::xls });
    const bool is_xls = (new_model == PrinterModel::xls);

    bool needs_settings_reset = false;
    {
        auto transaction = store.get_backend().transaction_guard();

        // Auto-set print fan type based on variant: XLS uses LDO, XL uses Delta (default)
        const auto fan_type = is_xls ? PrintFanType::LDO_D5015G08B05X71 : PrintFanType::DELTA_BFB0505HHA_CWCD;
        for (auto tool : PhysicalToolIndex::all()) {
            set_print_fan_type(tool, fan_type);
        }

        // XLS ships with HF nozzles, XL did not
        store.nozzle_is_high_flow.set(is_xls ? ((1 << PhysicalToolIndex::count) - 1) : 0);

        // XLS ships with the nozzle cleaner as well
        store.nozzle_cleaner_lite_installed.set(is_xls);

        // ...and with the new belts
        needs_settings_reset = store.set_belts_15gt(is_xls);
    }

    if (needs_settings_reset) {
        switch (mode) {

        case ChangeExtendedPrinterTypeMode::standard_with_marlin_client_and_puppies:
            // Reset steps per MM
            gcode("M92 X%f Y%f", (double)get_steps_per_unit_x(), (double)get_steps_per_unit_y());
            break;

        case ChangeExtendedPrinterTypeMode::config_store_init:
            // Marlin has not been initialized yet - no need to reset
            break;
        }
    }

    // Update Dwarf fan_mode register to match the new variant.
    // Dwarfs re-derive this from PrinterModelInfo::current() during their own init.
    if (mode == ChangeExtendedPrinterTypeMode::standard_with_marlin_client_and_puppies) {
        using namespace buddy::puppies;
        const Dwarf::FanMode fan_mode = is_xls ? Dwarf::FanMode::XLS_NATIVE : Dwarf::FanMode::XL_LEGACY;
        for (auto &dwarf : buddy::puppies::dwarfs) {
            dwarf.set_fan_mode(fan_mode);
        }
    }
    #endif

    #if EXTENDED_PRINTER_TYPE_DETERMINES_MOTOR_STEPS()
    // Reset motor configuration if the printer types have different motors
    if (old_index >= extended_printer_type_has_400step_motors.size() || extended_printer_type_has_400step_motors[old_index] != extended_printer_type_has_400step_motors[new_index]) {
        {
            auto transaction = store.get_backend().transaction_guard();
            store.homing_sens_x.set_to_default();
            store.homing_sens_y.set_to_default();

            store.homing_bump_divisor_x.set_to_default();
            store.homing_bump_divisor_y.set_to_default();

        #if HAS_PRECISE_HOMING()
            store.precise_homing_sample_history.set_all_to_default();
            store.precise_homing_sample_history_index.set_all_to_default();
        #endif
        }

        if (mode == ChangeExtendedPrinterTypeMode::standard_with_marlin_client_and_puppies) {
            // Reset XY homing sensitivity
            gcode("M914 X Y");

            // XY motor currents
            gcode("M906 X%u Y%u", get_rms_current_ma_x(), get_rms_current_ma_y());

            // XY motor microsteps
            gcode("M350 X%u Y%u", get_microsteps_x(), get_microsteps_y());
        }
    }
    #endif
}

#endif // HAS_EXTENDED_PRINTER_TYPE()
