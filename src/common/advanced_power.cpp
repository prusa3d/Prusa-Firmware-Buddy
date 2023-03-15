#include "advanced_power.hpp"
#include "metric.h"
#include "adc.hpp"
#include "bsod.h"
#include "hwio_pindef.h"

#if !(BOARD_IS_DWARF)
    #include "bsod_gui.hpp"
#endif

using namespace buddy::hw;

AdvancedPower advancedpower;

AdvancedPower::AdvancedPower()
    : isInitialized(false) {
}

#if !(BOARD_IS_DWARF)
bool AdvancedPower::HSUSBOvercurentFaultDetected() const {
    return (hsUSBOvercurrent.read() == Pin::State::low);
}

bool AdvancedPower::FSUSBOvercurentFaultDetected() const {
    return (fsUSBOvercurrent.read() == Pin::State::low);
}
#endif

void AdvancedPower::ResetOvercurrentFault() {
#if BOARD_IS_XBUDDY
    faultMemoryReset.write(Pin::State::low);
    HAL_Delay(2);
    faultMemoryReset.write(Pin::State::high);
    HAL_Delay(2);
#endif
    isInitialized = true;
}

#if BOARD_IS_BUDDY || BOARD_IS_XBUDDY

bool AdvancedPower::HeaterOvercurentFaultDetected() const {
    return (heaterCurrentFault.read() == Pin::State::high);
}

bool AdvancedPower::OvercurrentFaultDetected() const {
    return (inputCurrentFault.read() == Pin::State::high);
}

#endif

#if !(BOARD_IS_DWARF)
void AdvancedPower::Update() {
    if (!isInitialized)
        return;
    #if BOARD_IS_XBUDDY
    if (HeaterOvercurentFaultDetected()) {
        fatal_error(ErrCode::ERR_ELECTRO_NOZZLE_OVERCURRENT);
    } else if (OvercurrentFaultDetected()) {
        fatal_error(ErrCode::ERR_ELECTRO_INPUT_OVERCURRENT);
        #if HAS_MMU2
    } else if (MMUOvercurentFaultDetected()) {
        fatal_error(ErrCode::ERR_ELECTRO_MMU_OVERCURRENT);
        #endif
    } else if (HSUSBOvercurentFaultDetected()) {
        hsUSBEnable.write(Pin::State::high);
        fatal_error(ErrCode::ERR_ELECTRO_USB_HOST_OVERCURRENT);
    } else if (FSUSBOvercurentFaultDetected()) {
        fsUSBPwrEnable.write(Pin::State::high);
        fatal_error(ErrCode::ERR_ELECTRO_USB_DEVICE_OVERCURRENT);
    }

    #else
    if (HSUSBOvercurentFaultDetected()) {
        hsUSBEnable.write(Pin::State::high);
        fatal_error(ErrCode::ERR_ELECTRO_USB_HOST_OVERCURRENT);
    } else if (FSUSBOvercurentFaultDetected()) {
        fsUSBPwrEnable.write(Pin::State::high);
        fatal_error(ErrCode::ERR_ELECTRO_USB_DEVICE_OVERCURRENT);
    }
    #endif
}
#endif

#if HAS_MMU2
bool AdvancedPower::MMUOvercurentFaultDetected() const {
    return (MMUFault.read() == Pin::State::high);
}
#endif
