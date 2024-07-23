#pragma once

#include "adc.hpp"
#include "stdlib.h"
#include <option/has_mmu2.h>

#if BOARD_IS_XLBUDDY()
    #include "hw_configuration.hpp"
    #include "puppies/Dwarf.hpp"
#endif

#include <option/has_modularbed.h>

class AdvancedPower {
public:
    AdvancedPower();

#if BOARD_IS_XBUDDY()
    inline int GetBedVoltageRaw() const {
        return AdcGet::inputVoltage();
    }

    inline float GetBedVoltage() const {
        return beforeVoltageDivider11(RawValueToVoltage(GetBedVoltageRaw()));
    }

    inline int GetHeaterVoltageRaw() const {
        return AdcGet::heaterVoltage();
    }

    inline float GetHeaterVoltage() const {
        return beforeVoltageDivider11(RawValueToVoltage(GetHeaterVoltageRaw()));
    }

    inline int GetHeaterCurrentRaw() const {
        return AdcGet::heaterCurrent();
    }

    inline float GetHeaterCurrent() const {
        return ((RawValueToVoltage(GetHeaterCurrentRaw()) / 1.95f) * 2.00f);
    }

    inline int GetInputCurrentRaw() const {
        return AdcGet::inputCurrent();
    }

    /**
     * @brief Get the Input Current
     * @return float current [mA]
     */
    float GetInputCurrent() const;

    bool HeaterOvercurentFaultDetected() const;

    bool OvercurrentFaultDetected() const;

#elif BOARD_IS_XLBUDDY()
    inline float GetDwarfSplitter5VCurrent() const {
        return ((RawValueToVoltage(AdcGet::dwarfsCurrent()) / CurrentSenseGain) / RSense);
    }

    inline float Get24VVoltage() const {
        return beforeVoltageDivider11(RawValueToVoltage(AdcGet::inputVoltage24V()));
    }

    inline float Get5VVoltage() const {
        return beforeVoltageDividerSandwich5V(RawValueToVoltage(AdcGet::inputVoltage5V()));
    }

    inline float GetDwarfSandwitch5VCurrent() const {
        return ((beforeVoltageDividerCurrent(RawValueToVoltage(AdcGet::sandwichCurrent5V())) / CurrentSenseGain) / RSense);
    }

    inline float GetXLBuddy5VCurrent() const {
        return ((beforeVoltageDividerCurrent(RawValueToVoltage(AdcGet::xlbuddyCurrent5V())) / CurrentSenseGain) / RSense);
    }

    // Get nozzle heater current [A]
    inline float get_nozzle_current(uint8_t index) {
        assert(index >= 0 && index < buddy::puppies::dwarfs.size());
        return buddy::puppies::dwarfs[index].get_heater_current();
    }

    // Get nozzle supply voltage [V]
    inline float get_nozzle_voltage(uint8_t index) {
        assert(index >= 0 && index < buddy::puppies::dwarfs.size());
        return buddy::puppies::dwarfs[index].get_24V();
    }

    // Get nozzle heater PWM
    inline int get_nozzle_pwm(uint8_t index) {
        assert(index >= 0 && index < buddy::puppies::dwarfs.size());
        return buddy::puppies::dwarfs[index].get_heater_pwm();
    }
#elif BOARD_IS_DWARF()
    inline float GetDwarfNozzleCurrent() const {
        return ((RawValueToVoltage(AdcGet::heaterCurrent()) / CurrentSenseGain) / RSense);
    }

    inline float Get24VVoltage() const {
        return beforeVoltageDivider11(RawValueToVoltage(AdcGet::inputf24V()));
    }
#endif

#if HAS_MODULARBED()
    // Get bed heater current [A]
    // Would be nice to have this inline, but it would drag a modbus includes.
    float get_bed_current();
#endif

#if HAS_MMU2()
    inline int GetMMUInputCurrentRaw() const {
        return AdcGet::MMUCurrent();
    }

    inline float GetMMUInputCurrent() const {
        return ((1.416f * (RawValueToVoltage(GetMMUInputCurrentRaw())) - 0.013f));
    }

    bool MMUOvercurentFaultDetected() const;
#endif

    bool HSUSBOvercurentFaultDetected() const;

    bool FSUSBOvercurentFaultDetected() const;

    void ResetOvercurrentFault();

#if !(BOARD_IS_DWARF())
    void Update();
#endif

private:
    bool isInitialized;
    static constexpr float CurrentSenseGain = 50.00f;
    static constexpr float RSense = 0.022f; // 22mohm

    inline float RawValueToVoltage(int rawValue) const {
        return (rawValue * 3.35f) / 1023.00f;
    }

    float beforeVoltageDivider11(float voltage) const {
        constexpr float R1 = 10.0f * 1000.0f;
        constexpr float R2 = 1.0f * 1000.0f;
        return (voltage * ((R1 + R2) / R2));
    }

#if BOARD_IS_XLBUDDY()
    float beforeVoltageDividerSandwich5V(float voltage) const {
        return voltage * SandwichConfiguration::Instance().divider_5V_coefficient();
    }

    float beforeVoltageDividerCurrent(float voltage) const {
        return voltage * SandwichConfiguration::Instance().divider_current_coefficient();
    }
#endif
};

extern AdvancedPower advancedpower;
