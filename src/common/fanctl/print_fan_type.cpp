#include <print_fan_type.hpp>

#include <config_store/store_definition.hpp>

PrintFanType get_print_fan_type(size_t extruder_nr) {
    return config_store().print_fan_type.get(extruder_nr);
}

void set_print_fan_type(size_t extruder_nr, PrintFanType pft) {
    return config_store().print_fan_type.set(extruder_nr, pft);
}

template <typename T>
static inline T linear_interpolation(const T x0, const T y0, const T x1, const T y1, const T x) {
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

#if PRINTER_IS_PRUSA_XL()

uint16_t print_fan_remap_pwm(PrintFanType pft, uint16_t original_pwm) {
    switch (pft) {
    case PrintFanType::DELTA_BFB0505HHA_CWCD: {
        return original_pwm;
    }
    case PrintFanType::GOM_VD_2620: {
        if (original_pwm == 0) {
            return 0;
        }
        // interpolate PWM values
        // Delta fan has at 20% PWM the same RPM as GOM at 47% PWM
        // Delta fan has at 70% PWM the same airflow as GOM at 100% PWM
        auto remapped_pwm = linear_interpolation<uint32_t>(20 * 255 / 100, 47 * 255 / 100, 70 * 255 / 100, 100 * 255 / 100, original_pwm);

        // clamp at minimum of 20% of the original PWM value, but remapped
        return std::clamp<uint16_t>(remapped_pwm, 47 * 255 / 100, 255);
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
