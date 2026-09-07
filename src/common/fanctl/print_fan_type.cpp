#include <print_fan_type.hpp>

#include <config_store/store_definition.hpp>
#include <algorithm_scale.hpp>
#include <bsod/bsod.h>

PrintFanType get_print_fan_type(PhysicalToolIndex tool) {
    return config_store().print_fan_type.get(tool.to_raw());
}

void set_print_fan_type(PhysicalToolIndex tool, PrintFanType pft) {
    return config_store().print_fan_type.set(tool.to_raw(), pft);
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
    case PrintFanType::LDO_D5015G08B05X71: {
        // LDO blower for XLS - no need for remapping, XLS has its own print profiles
        return original_pwm;
    }
    case PrintFanType::_cnt: {
        break;
    }
    }
    bsod_unreachable();
}

#else // PRINTER_IS_PRUSA_XL()
    #error "When HAS_PRINT_FAN_TYPE is enabled for more printers, remapping function should be implemented here"
#endif
