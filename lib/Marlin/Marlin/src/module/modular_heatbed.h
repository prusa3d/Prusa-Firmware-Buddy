#pragma once
#include "Marlin.h"

#if ENABLED(MODULAR_HEATBED)

/**
 * @brief THIS IS A STUB!!! NOT A WELL THOUGHT THROUGH API!!!
 * Consider this a minimal list of functions Marlin needs to make modular heatbed work.
 * Replace this class before proceding with implementation.
 */
class SimpleModularHeatbed {

public:
    virtual ~SimpleModularHeatbed() = default;
    virtual void set_target(float target_temp) = 0;  //Set target temperature of active headbedlets
    virtual float get_temp() = 0;    //Average of active heatbedlets
    virtual int get_pwm() = 0;      //Average controll action of all active beds. TODO: Is this % of max power, or 0-255?
};

extern SimpleModularHeatbed * const simple_modular_bed;

class AdvancedModularBed {
public:
    virtual ~AdvancedModularBed() = default;
    virtual uint16_t idx(const uint8_t column, const uint8_t row) = 0;
    virtual void set_target(const uint8_t column, const uint8_t row, float target_temp) = 0;
    virtual float get_temp(const uint8_t column, const uint8_t row) = 0;
    virtual void update_bedlet_temps(uint16_t enabled_mask, float target_temp) = 0;


    inline void set_gradient_cutoff(float value) {
        bedlet_gradient_cutoff = value;
    }

    void set_gradient_exponent(float value) {
        bedlet_gradient_exponent = value;
    }

protected:
    float bedlet_gradient_cutoff = HBL_GRADIENT_EXPONENT;
    float bedlet_gradient_exponent = HBL_GRADIENT_CUTOFF;
};

extern AdvancedModularBed * const advanced_modular_bed;

#endif
