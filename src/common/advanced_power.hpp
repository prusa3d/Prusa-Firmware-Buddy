#pragma once

#include "adc.hpp"
#include "stdlib.h"

class AdvancedPower {
public:
    AdvancedPower();

#if BOARD_IS_XBUDDY
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

    #if (BOARD_IS_XBUDDY && BOARD_VER_EQUAL_TO(0, 3, 4))
    static constexpr float allegro_zero_curr_voltage = 3.35F / 2.F; // 3V3 ref
    #else
    static constexpr float allegro_zero_curr_voltage = 5.F / 2.F; // 5V ref
    #endif // BOARD

    static constexpr float allegro_curr_from_voltage = 1 / 0.09F;
    /**
     * @brief Get the Input Current
     * Allegro ACS711KEXLT-15AB
     * +-15 A, 90mV/A, 0A -> output == Vcc/2
     *
     * XBuddy 0.3.4 3V3 reference
     * XBuddy < 0.3.4 5V reference
     * @return float current [mA]
     */
    inline float GetInputCurrent() const {
        return (RawValueToVoltage(GetInputCurrentRaw()) - allegro_zero_curr_voltage) * allegro_curr_from_voltage;
    }

    bool HeaterOvercurentFaultDetected() const;

    bool OvercurrentFaultDetected() const;

#elif BOARD_IS_XLBUDDY
    inline float GetDwarfSplitter5VCurrent() const {
        return ((RawValueToVoltage(AdcGet::dwarfsCurrent()) / CurrentSenseGain) / RSense);
    }

    inline float Get24VVoltage() const {
        return beforeVoltageDivider11(RawValueToVoltage(AdcGet::inputVoltage24V()));
    }

    inline float Get5VVoltage() const {
        return beforeVoltageDivider11(RawValueToVoltage(AdcGet::inputVoltage5V()));
    }

    inline float GetDwarfSandwitch5VCurrent() const {
        return ((RawValueToVoltage(AdcGet::sandwichCurrent5V()) / CurrentSenseGain) / RSense);
    }

    inline float GetXLBuddy5VCurrent() const {
        return ((RawValueToVoltage(AdcGet::xlbuddyCurrent5V()) / CurrentSenseGain) / RSense);
    }
#elif BOARD_IS_DWARF
    inline float GetDwarfNozzleCurrent() const {
        return ((RawValueToVoltage(AdcGet::heaterCurrent()) / CurrentSenseGain) / RSense);
    }

    inline float Get24VVoltage() const {
        return beforeVoltageDivider11(RawValueToVoltage(AdcGet::inputf24V()));
    }
#endif

#if HAS_MMU2
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

#if !(BOARD_IS_DWARF)
    void Update();
#endif

private:
    bool isInitialized;
    float CurrentSenseGain = 50.00f;
    float RSense = 0.022f; //22mohm

    inline float RawValueToVoltage(int rawValue) const {
        return (rawValue * 3.35f) / 1023.00f;
    }

    float beforeVoltageDivider11(float voltage) const {
        constexpr float R1 = 10.0f * 1000.0f;
        constexpr float R2 = 1.0f * 1000.0f;
        return (voltage * ((R1 + R2) / R2));
    }
};

extern AdvancedPower advancedpower;
