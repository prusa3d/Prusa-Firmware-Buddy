/// @file tmc2130.h
#pragma once
#include "../config/config.h"
#include "../hal/gpio.h"
#include "../hal/shr16.h"
#include "../hal/spi.h"

namespace hal {

/// TMC2130 interface
/// There are multiple TMC2130 on our board, so there will be multiple
/// instances of this class
namespace tmc2130 {

enum MotorMode : uint8_t {
    Stealth,
    Normal
};

struct MotorParams {
    const hal::spi::SPI_TypeDef *const spi;
    uint8_t idx; ///< SHR16 index
    bool dirOn; ///< forward direction
    gpio::GPIO_pin csPin; ///< CS pin
    gpio::GPIO_pin stepPin; ///< step pin
    gpio::GPIO_pin sgPin; ///< StallGuard pin
    config::MRes mRes; ///< microstep resolution
    int8_t sg_thrs;
    uint8_t axis;
};

struct MotorCurrents {
    bool vSense; ///< VSense current scaling
    uint8_t iRun; ///< Running current
    uint8_t iHold; ///< Holding current

    constexpr MotorCurrents(uint8_t ir, uint8_t ih)
        : vSense((ir < 32) ? 1 : 0)
        , iRun((ir < 32) ? ir : (ir >> 1))
        , iHold((ir < 32) ? ih : (ih >> 1)) {}
};

struct __attribute__((packed)) ErrorFlags {
    uint8_t reset_flag : 1; ///< driver restarted
    uint8_t uv_cp : 1; ///< undervoltage on charge pump
    uint8_t s2g : 1; ///< short to ground
    uint8_t otpw : 1; ///< over temperature pre-warning
    uint8_t ot : 1; ///< over temperature hard
    inline constexpr ErrorFlags()
        : reset_flag(0)
        , uv_cp(0)
        , s2g(0)
        , otpw(0)
        , ot(0) {}
    inline bool Good() const { return reset_flag == 0 && uv_cp == 0 && s2g == 0 && otpw == 0 && ot == 0; }
};

/// TMC2130 interface - instances of this class are hidden in modules::motion::Motion::AxisData
class TMC2130 {
public:
    /// TMC2130 register addresses
    enum class Registers : uint8_t {
        /// General Configuration Registers
        GCONF = 0x00,
        GSTAT = 0x01,
        IOIN = 0x04,

        /// Velocity Dependent Driver Feature Control Register Set
        IHOLD_IRUN = 0x10,
        TPOWERDOWN = 0x11,
        TSTEP = 0x12,
        TPWMTHRS = 0x13,
        TCOOLTHRS = 0x14,
        THIGH = 0x15,

        /// Motor Driver Registers
        MSCNT = 0x6A,
        CHOPCONF = 0x6C,
        COOLCONF = 0x6D,
        DRV_STATUS = 0x6F,
        PWMCONF = 0x70,
    };

    /// Constructor
    TMC2130() = default;

    /// (re)initialization of the chip - please note this is necessary due to some HW flaws in the original MMU boards.
    /// And yes, the TMC may not get correctly initialized.
    /// @returns true if the TMC2130 was inited correctly
    bool Init(const MotorParams &params,
        const MotorCurrents &currents,
        MotorMode mode);

    /// Set the current motor mode
    void SetMode(const MotorParams &params, MotorMode mode);

    /// Disables the output by setting or clearing CHOPCONF's TOFF.
    void SetBridgeOutput(const MotorParams &params, bool on);

    /// Set the current motor currents
    void SetCurrents(const MotorParams &params, const MotorCurrents &currents);

    /// Set StallGuard threshold
    /// New SGTHRS must be already set in @p params
    /// Beware - there are no checks, new value is written into the TMC register immediately.
    /// It is advised to prefer setting the SGTHRS via the Init() method.
    void SetSGTHRS(const MotorParams &params);

    /// Return enabled state
    const bool Enabled() const {
        return enabled;
    }

    /// Enable/Disable the motor
    void SetEnabled(const MotorParams &params, bool enabled);

    /// Set direction
    static inline void SetDir(const MotorParams &params, bool dir) {
        hal::shr16::shr16.SetTMCDir(params.idx, dir ^ params.dirOn);
    }

    /// Set direction, raw value (i.e. ignore Params).
    static inline void SetRawDir(const MotorParams &params, bool dir) {
        // Also cancels the inversion in SetTMCDir
        hal::shr16::shr16.SetTMCDir(params.idx, !dir);
    }

    /// Step the motor
    static inline void Step(const MotorParams &params) {
        gpio::TogglePin(params.stepPin); // assumes DEDGE
    }

    /// Set step to an explicit state
    static inline void SetStep(const MotorParams &params, bool state) {
        gpio::WritePin(params.stepPin, (state ? gpio::Level::high : gpio::Level::low));
    }

    /// Return SG state
    static inline bool SampleDiag(const MotorParams &params) {
        return gpio::ReadPin(params.sgPin) == gpio::Level::low;
    }

    inline bool Stalled() const {
        return sg_filter_counter == 0;
    }

    inline void ClearStallguard() {
        sg_filter_counter = sg_filter_threshold;
    }

    /// Should be called periodically. Maybe not all the time. Once every 10 ms is probably enough
    bool CheckForErrors(const MotorParams &params);

    inline ErrorFlags GetErrorFlags() const {
        return errorFlags;
    }

#ifdef UNITTEST
    /// A very brutal way of fiddling with the error flags from the outside of this class
    /// Intended only for the UNIT TESTS!
    inline void SetErrorFlags(ErrorFlags ef) {
        errorFlags = ef;
    }
#endif

    /// Reads a driver register and updates the status flags
    uint32_t ReadRegister(const MotorParams &params, Registers reg);

    /// Writes a driver register and updates the status flags
    void WriteRegister(const MotorParams &params, Registers reg, uint32_t data);

    /// Used for polling the DIAG pin. Should be called from the stepper isr periodically when moving.
    void Isr(const MotorParams &params);

private:
    void _spi_tx_rx(const MotorParams &params, uint8_t (&pData)[5]);
    void _handle_spi_status(const MotorParams &params, uint8_t status);

    ErrorFlags errorFlags;
    bool initialized = false;
    bool enabled = false;
    uint8_t sg_filter_threshold = 0;
    uint8_t sg_filter_counter = 0;
};

} // namespace tmc2130
} // namespace hal
