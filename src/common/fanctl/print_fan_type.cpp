#include <print_fan_type.hpp>

#include <config_store/store_definition.hpp>
#include <algorithm_scale.hpp>

PrintFanType get_print_fan_type(size_t extruder_nr) {
    return config_store().print_fan_type.get(extruder_nr);
}

void set_print_fan_type(size_t extruder_nr, PrintFanType pft) {
    return config_store().print_fan_type.set(extruder_nr, pft);
}

#if PRINTER_IS_PRUSA_XL()

uint16_t print_fan_remap_pwm(PrintFanType pft, uint16_t original_pwm) {
    switch (pft) {
    case PrintFanType::DELTA_BFB0505HHA_CWCD: {
        return original_pwm;
    }
    case PrintFanType::GOM_VD_3706: {
        if (original_pwm == 0) {
            return 0;
        }
        // Interpolate PWM values
        // Delta fan has at 20% PWM the same airflow as GOM at 40% PWM
        // Delta fan has at 100% PWM lower airflow as GOM at 100% PWM, we keep full power for GOM with higher airflow
        // Clamping applied outside of 20% - 100% of the original value
        auto remapped_pwm = scale<uint32_t>(original_pwm, 20 * 255 / 100, 100 * 255 / 100, 40 * 255 / 100, 100 * 255 / 100);

        return remapped_pwm;
    }
    case PrintFanType::_cnt: {
        break;
    }
    }
    std::unreachable();
}

#else // PRINTER_IS_PRUSA_XL()
    #error "When HAS_PRINT_FAN_TYPE is enabled for more printers, remapping function should be implemented here"
#endif
