#include <print_fan_type.hpp>

#include <config_store/store_definition.hpp>

PrintFanType get_print_fan_type(size_t extruder_nr) {
    return config_store().print_fan_type.get(extruder_nr);
}

void set_print_fan_type(size_t extruder_nr, PrintFanType pft) {
    return config_store().print_fan_type.set(extruder_nr, pft);
}

#if PRINTER_IS_PRUSA_XL()

uint16_t print_fan_remap_pwm(PrintFanType pft, uint16_t original_pwm) {
    switch (pft) {
    case PrintFanType::DELTA_BFB0505HHA_CWCD:
        return original_pwm;
    case PrintFanType::GOM_VD_2620:
        if (original_pwm == 0) {
            return 0;
        }
        return std::clamp<uint16_t>(original_pwm * 1.43f, 1.43f * 0.2f * 255, 255);
    case PrintFanType::_cnt:
        std::unreachable();
    }
    std::unreachable();
}

#else // PRINTER_IS_PRUSA_XL()
    #error "When HAS_PRINT_FAN_TYPE is enabled for more printers, remapping function should be implemented here"
#endif
