#pragma once

#include "Marlin.h"

#if ENABLED(MODULAR_HEATBED)

class AdvancedModularBed {
public:
    virtual ~AdvancedModularBed() = default;
    virtual uint16_t idx(const uint8_t column, const uint8_t row) = 0;
    virtual void set_target(const uint8_t column, const uint8_t row, float target_temp) = 0;
    virtual float get_target(const uint8_t column, const uint8_t row) = 0;
    virtual float get_temp(const uint8_t column, const uint8_t row) = 0;
    virtual void update_bedlet_temps(uint16_t enabled_mask, float target_temp) = 0;

    inline void set_gradient_cutoff(float value) {
        bedlet_gradient_cutoff = value;
    }

    void set_gradient_exponent(float value) {
        bedlet_gradient_exponent = value;
    }

    void set_expand_to_sides(bool enabled) {
        expand_to_sides_enabled = enabled;
    }

protected:
    float bedlet_gradient_cutoff = HBL_GRADIENT_CUTOFF;
    float bedlet_gradient_exponent = HBL_GRADIENT_EXPONENT;
    bool expand_to_sides_enabled = HBL_EXPAND_TO_SIDES;
};

extern AdvancedModularBed * const advanced_modular_bed;

#endif
